// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2010 Juergen Beisert, Pengutronix <kernel@pengutronix.de>
 * Copyright (C) 2011 Marc Kleine-Budde, Pengutronix <mkl@pengutronix.de>
 * Copyright (C) 2011 Wolfram Sang, Pengutronix <w.sang@pengutronix.de>
 * Copyright (C) 2019 Oleksij Rempel, Pengutronix <o.rempel@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <net.h>
#include <mach/mxs/ocotp.h>

static void mx28_evk_get_ethaddr(void)
{
	u8 mac_ocotp[3];
	MACaddr_t mac;
	int ret;

	ret = mxs_ocotp_read(mac_ocotp, 3, 0);
	if (ret != 3) {
		pr_err("Reading MAC from OCOTP failed!\n");
		return;
	}

	mac.addr[0] = 0x00;
	mac.addr[1] = 0x04;
	mac.addr[2] = 0x9f;
	mac.addr[3] = mac_ocotp[2];
	mac.addr[4] = mac_ocotp[1];
	mac.addr[5] = mac_ocotp[0];

	eth_register_ethaddr(0, &mac);
}

static int mx28_evk_devices_init(void)
{
	if (!of_machine_is_compatible("fsl,imx28-evk"))
		return 0;

	mx28_evk_get_ethaddr(); /* must be after registering ocotp */

	return 0;
}
fs_initcall(mx28_evk_devices_init);
