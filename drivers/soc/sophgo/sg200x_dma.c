// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2021 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <dma.h>
#include <asm/barrier.h>

#define L1_CACHE_BYTES     64

/*
 * dcache.ipa rs1 (invalidate)
 * | 31 - 25 | 24 - 20 | 19 - 15 | 14 - 12 | 11 - 7 | 6 - 0 |
 *   0000001    01010      rs1       000      00000  0001011
 *
 * dcache.cpa rs1 (clean)
 * | 31 - 25 | 24 - 20 | 19 - 15 | 14 - 12 | 11 - 7 | 6 - 0 |
 *   0000001    01001      rs1       000      00000  0001011
 *
 * dcache.cipa rs1 (clean then invalidate)
 * | 31 - 25 | 24 - 20 | 19 - 15 | 14 - 12 | 11 - 7 | 6 - 0 |
 *   0000001    01011      rs1       000      00000  0001011
 *
 * sync.s
 * | 31 - 25 | 24 - 20 | 19 - 15 | 14 - 12 | 11 - 7 | 6 - 0 |
 *   0000000    11001     00000      000      00000  0001011
 */
#define DCACHE_IPA_A0	".long 0x02a5000b"
#define DCACHE_CPA_A0	".long 0x0295000b"
#define DCACHE_CIPA_A0	".long 0x02b5000b"

#define SYNC_S		".long 0x0190000b"

#define CACHE_OP_RANGE(OP, start, size) \
	register unsigned long i asm("a0") = start & ~(L1_CACHE_BYTES - 1); \
	for (; i < ALIGN(end, L1_CACHE_BYTES); i += L1_CACHE_BYTES) \
		__asm__ __volatile__(OP); \
	 __asm__ __volatile__(SYNC_S)

static void sg200x_flush_range(dma_addr_t start, dma_addr_t end)
{
	CACHE_OP_RANGE(DCACHE_CIPA_A0, start, end);
}

static void sg200x_inv_range(dma_addr_t start, dma_addr_t end)
{
	CACHE_OP_RANGE(DCACHE_IPA_A0, start, end);
}

static inline void *sg200x_alloc_coherent(struct device *dev,
					  size_t size, dma_addr_t *dma_handle)
{
	dma_addr_t cpu_base;
	void *ret;

	ret = xmemalign(L1_CACHE_BYTES, size);

	memset(ret, 0, size);

	cpu_base = (dma_addr_t)ret;

	if (dma_handle)
		*dma_handle = cpu_base;

	sg200x_flush_range(cpu_base, cpu_base + size);

	return ret;

}

static inline void sg200x_free_coherent(struct device *dev,
					void *vaddr, dma_addr_t dma_handle, size_t size)
{
	free((void *)dma_handle);
}

static const struct dma_ops sg200x_dma_ops = {
	.alloc_coherent = sg200x_alloc_coherent,
	.free_coherent = sg200x_free_coherent,
	.flush_range = sg200x_flush_range,
	.inv_range = sg200x_inv_range,
};

static int sg200x_dma_init(void)
{
	/* board drivers can claim the machine compatible, so no driver here */
	if (!of_machine_is_compatible("sophgo,cv1812h"))
		return 0;

	dma_set_ops(&sg200x_dma_ops);

	return 0;
}
mmu_initcall(sg200x_dma_init);
