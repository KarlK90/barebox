// SPDX-License-Identifier: GPL-2.0-only

#include <asm/system.h>
#include <clock.h>
#include <common.h>

void udelay(unsigned long us)
{
	unsigned long cntfrq = get_cntfrq();
	unsigned long ticks = (us * cntfrq) / MSECOND;
	unsigned long start = get_cntpct();

	while ((long)(start + ticks - get_cntpct()) > 0);
}

void mdelay(unsigned long ms)
{
	udelay(ms * USECOND);
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
