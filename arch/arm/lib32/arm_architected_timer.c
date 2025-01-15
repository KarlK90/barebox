// SPDX-License-Identifier: GPL-2.0-only

#include <asm/system.h>
#include <clock.h>
#include <common.h>

/* Unlike the ARMv8, the timer is not generic to ARM32 */
void arm_architected_timer_udelay(unsigned long us)
{
	unsigned long long ticks, cntfrq = get_cntfrq();
	unsigned long long start = get_cntpct();

	ticks = DIV_ROUND_DOWN_ULL((us * cntfrq), MSECOND);

	while ((long)(start + ticks - get_cntpct()) > 0)
		;
}

uint64_t get_time_ns(void)
{
	return get_cntpct() * SECOND / get_cntfrq();
}

int is_timeout(uint64_t start, uint64_t time_offset_ns)
{
	if ((int64_t)(start + time_offset_ns - get_time_ns()) < 0) {
		return 1;
	} else {
		return 0;
	}
}
