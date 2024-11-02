// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include "barebox-sophgo-riscv-head.h"
#include <asm/barebox-riscv.h>
#include <debug_ll.h>

ENTRY_FUNCTION_SOPHGO(start_milkv_duo_s, a0, a1, a2)
{
	extern char __dtb_z_cv1812h_milkv_duo_s_start[];

	putc_ll('>');

	barebox_riscv_supervisor_entry(0x80000000, SZ_512M, a0, __dtb_z_cv1812h_milkv_duo_s_start + get_runtime_offset());
}
