/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * OF helpers for network devices.
 */

#ifndef __LINUX_OF_NET_H
#define __LINUX_OF_NET_H

#include <linux/types.h>
#include <net.h>
#include <of.h>

#ifdef CONFIG_OFTREE
int of_get_mac_addr_nvmem(struct device_node *np, MACaddr_t *addr);
int of_get_mac_address(struct device_node *np, MACaddr_t *addr);
int of_get_phy_mode(struct device_node *np);
#else
static inline int of_get_mac_addr_nvmem(struct device_node *np, MACaddr_t *addr)
{
	return -ENOSYS;
}
static inline int of_get_mac_address(struct device_node *np, MACaddr_t *addr)
{
	return -ENOSYS;
}

static inline int of_get_phy_mode(struct device_node *np)
{
	return -ENOSYS;
}
#endif

#endif /* __LINUX_OF_NET_H */
