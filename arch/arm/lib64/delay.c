// SPDX-License-Identifier: GPL-2.0-only

#include <asm/system.h>
#include <clock.h>
#include <common.h>
#include <linux/math64.h>

void udelay(unsigned long us)
{
	unsigned long cntfrq = get_cntfrq();
	unsigned long ticks = (us * cntfrq) / 1000000;
	unsigned long start = get_cntpct();

	while ((long)(start + ticks - get_cntpct()) > 0);
}

void mdelay(unsigned long ms)
{
	udelay(ms * 1000);
}

uint64_t get_time_ns(void)
{
	uint64_t cntfrq = get_cntfrq();
	uint64_t cntpct = get_cntpct();

	/* CNTFRQ not programmed: degrade to infinite poll */
	if (!cntfrq)
		return 0;

	return mul_u64_u32_div(cntpct, SECOND, cntfrq);
}

int is_timeout(uint64_t start, uint64_t time_offset_ns)
{
	return (int64_t)(start + time_offset_ns - get_time_ns()) < 0;
}
