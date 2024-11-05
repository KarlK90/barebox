// SPDX-License-Identifier: GPL-2.0-or-later
/* Driver for CVITEK PHYs */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
// #include <linux/netdevice.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/clk.h>
// #include <linux/cv180x_efuse.h>

#define REG_EPHY_TOP_WRAP 0x03009800
#define REG_EPHY_BASE 0x03009000
#define EPHY_EFUSE_TXECHORC_FLAG 0x00000100 // bit 8
#define EPHY_EFUSE_TXITUNE_FLAG 0x00000200 // bit 9
#define EPHY_EFUSE_TXRXTERM_FLAG 0x00000800 // bit 11

#define CVI_INT_EVENTS \
	(CVI_LNK_STS_CHG_INT_MSK | CVI_MGC_PKT_DET_INT_MSK)

#define EFUSE_BASE (0x03050000)
#define EFUSE_SHADOW_REG (efuse_base + 0x100)
#define EFUSE_SIZE 0x100

static struct clk *efuse_clk;
static void __iomem *efuse_base = IOMEM(EFUSE_BASE);

static int64_t cvi_efuse_read_from_shadow(uint32_t addr)
{
	int64_t ret = -1;

    if (!efuse_clk){
        efuse_clk = clk_get_sys(NULL, "clk_efuse");
        if (IS_ERR(efuse_clk)) {
            pr_err("%s: efuse clock not found %ld\n", __func__,
                   PTR_ERR(efuse_clk));
            return -1;
        }
    }

	if (addr >= EFUSE_SIZE)
		return -EFAULT;

	if (addr % 4 != 0)
		return -EFAULT;

	ret = clk_prepare_enable(efuse_clk);
	if (ret) {
		pr_err("%s: clock failed to prepare+enable: %lld\n", __func__,
		       (long long)ret);
		return ret;
	}

	ret = readl(EFUSE_SHADOW_REG + addr);
	clk_disable_unprepare(efuse_clk);

	return ret;
}

static int cv182xa_phy_config_init(struct phy_device *phydev)
{
	int ret = 0;
	u32 val = 0;
	void __iomem *reg_ephy_top_wrap = IOMEM(REG_EPHY_TOP_WRAP);
	void __iomem *reg_ephy_base = IOMEM(REG_EPHY_BASE);
    pr_debug("%s\n", __func__);

	// set rg_ephy_apb_rw_sel 0x0804@[0]=1/APB by using APB interface
	writel(0x0001, reg_ephy_top_wrap + 4);

	writel(0x0, reg_ephy_base + 0x7c);

// Efuse register
	// Set Double Bias Current
	//Set rg_eth_txitune1  reg_ephy_base + 0x64 [15:8]
	//Set rg_eth_txitune0  reg_ephy_base + 0x64 [7:0]
	if ((cvi_efuse_read_from_shadow(0x20) & EPHY_EFUSE_TXITUNE_FLAG) ==
		EPHY_EFUSE_TXITUNE_FLAG) {
		val = ((cvi_efuse_read_from_shadow(0x24) >> 24) & 0xFF) |
				(((cvi_efuse_read_from_shadow(0x24) >> 16) & 0xFF) << 8);
		writel((readl(reg_ephy_base + 0x64) & ~0xFFFF) | val, reg_ephy_base + 0x64);
	} else
		writel(0x5a5a, reg_ephy_base + 0x64);

// Set Echo_I
	// Set rg_eth_txechoiadj reg_ephy_base + 0x54  [15:8]
	if ((cvi_efuse_read_from_shadow(0x20) & EPHY_EFUSE_TXECHORC_FLAG) ==
		EPHY_EFUSE_TXECHORC_FLAG) {
		writel((readl(reg_ephy_base + 0x54) & ~0xFF00) |
			   (((cvi_efuse_read_from_shadow(0x24) >> 8) & 0xFF) << 8), reg_ephy_base + 0x54);
	} else
		writel(0x0000, reg_ephy_base + 0x54);

//Set TX_Rterm & Echo_RC_Delay
	// Set rg_eth_txrterm_p1  reg_ephy_base + 0x58 [11:8]
	// Set rg_eth_txrterm     reg_ephy_base + 0x58  [7:4]
	// Set rg_eth_txechorcadj reg_ephy_base + 0x58  [3:0]
	if ((cvi_efuse_read_from_shadow(0x20) & EPHY_EFUSE_TXRXTERM_FLAG) ==
		EPHY_EFUSE_TXRXTERM_FLAG) {
		val = (((cvi_efuse_read_from_shadow(0x20) >> 28) & 0xF) << 4) |
				(((cvi_efuse_read_from_shadow(0x20) >> 24) & 0xF) << 8);
		writel((readl(reg_ephy_base + 0x58) & ~0xFF0) | val, reg_ephy_base + 0x58);
	} else
		writel(0x0bb0, reg_ephy_base + 0x58);

    // ETH_100BaseT
	// Set Rise update
	writel(0x0c10, reg_ephy_base + 0x5c);

	// Set Falling phase
	writel(0x0003, reg_ephy_base + 0x68);

	// Set Double TX Bias Current
	writel(0x0000, reg_ephy_base + 0x54);

	// Switch to MII-page16
	writel(0x1000, reg_ephy_base + 0x7c);

	// Set MLT3 Positive phase code, Set MLT3 +0
	writel(0x1000, reg_ephy_base + 0x68);
	writel(0x3020, reg_ephy_base + 0x6c);
	writel(0x5040, reg_ephy_base + 0x70);
	writel(0x7060, reg_ephy_base + 0x74);

	// Set MLT3 +I
	writel(0x1708, reg_ephy_base + 0x58);
	writel(0x3827, reg_ephy_base + 0x5c);
	writel(0x5748, reg_ephy_base + 0x60);
	writel(0x7867, reg_ephy_base + 0x64);

	// Switch to MII-page17
	writel(0x1100, reg_ephy_base + 0x7c);

	// Set MLT3 Negative phase code, Set MLT3 -0
	writel(0x9080, reg_ephy_base + 0x40);
	writel(0xb0a0, reg_ephy_base + 0x44);
	writel(0xd0c0, reg_ephy_base + 0x48);
	writel(0xf0e0, reg_ephy_base + 0x4c);

	// Set MLT3 -I
	writel(0x9788, reg_ephy_base + 0x50);
	writel(0xb8a7, reg_ephy_base + 0x54);
	writel(0xd7c8, reg_ephy_base + 0x58);
	writel(0xf8e7, reg_ephy_base + 0x5c);

	// @Switch to MII-page5
	writel(0x0500, reg_ephy_base + 0x7c);

	// En TX_Rterm
	writel((0x0001 | readl(reg_ephy_base + 0x40)), reg_ephy_base + 0x40);
	//Change rx vcm
	writel((0x820 |readl(reg_ephy_base + 0x4c)), reg_ephy_base + 0x4c);
    //	Link Pulse
	// Switch to MII-page10
	writel(0x0a00, reg_ephy_base + 0x7c);
#if 1
	// Set Link Pulse
	writel(0x3e00, reg_ephy_base + 0x40);
	writel(0x7864, reg_ephy_base + 0x44);
	writel(0x6470, reg_ephy_base + 0x48);
	writel(0x5f62, reg_ephy_base + 0x4c);
	writel(0x5a5a, reg_ephy_base + 0x50);
	writel(0x5458, reg_ephy_base + 0x54);
	writel(0xb23a, reg_ephy_base + 0x58);
	writel(0x94a0, reg_ephy_base + 0x5c);
	writel(0x9092, reg_ephy_base + 0x60);
	writel(0x8a8e, reg_ephy_base + 0x64);
	writel(0x8688, reg_ephy_base + 0x68);
	writel(0x8484, reg_ephy_base + 0x6c);
	writel(0x0082, reg_ephy_base + 0x70);
#else 
	// from sean
	// Fix err: the status is still linkup when removed the network cable.
	writel(0x2000, reg_ephy_base + 0x40);
	writel(0x3832, reg_ephy_base + 0x44);
	writel(0x3132, reg_ephy_base + 0x48);
	writel(0x2d2f, reg_ephy_base + 0x4c);
	writel(0x2c2d, reg_ephy_base + 0x50);
	writel(0x1b2b, reg_ephy_base + 0x54);
	writel(0x94a0, reg_ephy_base + 0x58);
	writel(0x8990, reg_ephy_base + 0x5c);
	writel(0x8788, reg_ephy_base + 0x60);
	writel(0x8485, reg_ephy_base + 0x64);
	writel(0x8283, reg_ephy_base + 0x68);
	writel(0x8182, reg_ephy_base + 0x6c);
	writel(0x0081, reg_ephy_base + 0x70);
#endif
// TP_IDLE
	// Switch to MII-page11
	writel(0x0b00, reg_ephy_base + 0x7c);

// Set TP_IDLE
	writel(0x5252, reg_ephy_base + 0x40);
	writel(0x5252, reg_ephy_base + 0x44);
	writel(0x4B52, reg_ephy_base + 0x48);
	writel(0x3D47, reg_ephy_base + 0x4c);
	writel(0xAA99, reg_ephy_base + 0x50);
	writel(0x989E, reg_ephy_base + 0x54);
	writel(0x9395, reg_ephy_base + 0x58);
	writel(0x9091, reg_ephy_base + 0x5c);
	writel(0x8E8F, reg_ephy_base + 0x60);
	writel(0x8D8E, reg_ephy_base + 0x64);
	writel(0x8C8C, reg_ephy_base + 0x68);
	writel(0x8B8B, reg_ephy_base + 0x6c);
	writel(0x008A, reg_ephy_base + 0x70);

// ETH 10BaseT Data
	// Switch to MII-page13
	writel(0x0d00, reg_ephy_base + 0x7c);

	writel(0x1E0A, reg_ephy_base + 0x40);
	writel(0x3862, reg_ephy_base + 0x44);
	writel(0x1E62, reg_ephy_base + 0x48);
	writel(0x2A08, reg_ephy_base + 0x4c);
	writel(0x244C, reg_ephy_base + 0x50);
	writel(0x1A44, reg_ephy_base + 0x54);
	writel(0x061C, reg_ephy_base + 0x58);

	// Switch to MII-page14
	writel(0x0e00, reg_ephy_base + 0x7c);

	writel(0x2D30, reg_ephy_base + 0x40);
	writel(0x3470, reg_ephy_base + 0x44);
	writel(0x0648, reg_ephy_base + 0x48);
	writel(0x261C, reg_ephy_base + 0x4c);
	writel(0x3160, reg_ephy_base + 0x50);
	writel(0x2D5E, reg_ephy_base + 0x54);

	// Switch to MII-page15
	writel(0x0f00, reg_ephy_base + 0x7c);

	writel(0x2922, reg_ephy_base + 0x40);
	writel(0x366E, reg_ephy_base + 0x44);
	writel(0x0752, reg_ephy_base + 0x48);
	writel(0x2556, reg_ephy_base + 0x4c);
	writel(0x2348, reg_ephy_base + 0x50);
	writel(0x0C30, reg_ephy_base + 0x54);

	// Switch to MII-page16
	writel(0x1000, reg_ephy_base + 0x7c);

	writel(0x1E08, reg_ephy_base + 0x40);
	writel(0x3868, reg_ephy_base + 0x44);
	writel(0x1462, reg_ephy_base + 0x48);
	writel(0x1A0E, reg_ephy_base + 0x4c);
	writel(0x305E, reg_ephy_base + 0x50);
	writel(0x2F62, reg_ephy_base + 0x54);

// LED
	// Switch to MII-page1
	writel(0x0100, reg_ephy_base + 0x7c);

	// select LED_LNK/SPD/DPX out to LED_PAD
	writel((readl(reg_ephy_base + 0x68) & ~0x0f00), reg_ephy_base + 0x68);

	// Switch to MII-page19
	writel(0x1300, reg_ephy_base + 0x7c);
	writel(0x0012, reg_ephy_base + 0x58);
	// set agc max/min swing
	writel(0x6848, reg_ephy_base + 0x5c);

	// Switch to MII-page18
	writel(0x1200, reg_ephy_base + 0x7c);
#if 1 || IS_ENABLED(CONFIG_ARCH_CV181X)
	/* mars LPF(8, 8, 8, 8) HPF(-8, 50(+32), -36, -8) */
	// lpf
	writel(0x0808, reg_ephy_base + 0x48);
	writel(0x0808, reg_ephy_base + 0x4c);
	// hpf
	writel(0x32f8, reg_ephy_base + 0x50);
	writel(0xf8dc, reg_ephy_base + 0x54);
#endif
#if IS_ENABLED(CONFIG_ARCH_CV180X)
	/* phobos LPF:(1 8 23 23 8 1) HPF:(-4,58,-45,8,-5, 0) from sean PPT */
	// lpf
	writel(0x0801, reg_ephy_base + 0x48);
	writel(0x1717, reg_ephy_base + 0x4C);
	writel(0x0108, reg_ephy_base + 0x5C);
	// hpf
	writel(0x3afc, reg_ephy_base + 0x50);
	writel(0x08d3, reg_ephy_base + 0x54);
	writel(0x00fb, reg_ephy_base + 0x60);
#endif

	// Switch to MII-page0
	writel(0x0000, reg_ephy_base + 0x7c);
	// EPHY start auto-neg procedure
	writel(0x090e, reg_ephy_top_wrap);

	// from jinyu.zhao
	/* EPHY is configured as half-duplex after reset, but we need force full-duplex */
	writel((readl(reg_ephy_base) | 0x100), reg_ephy_base);

	// switch to MDIO control by ETH_MAC
	writel(0x0000, reg_ephy_top_wrap + 4);

	return ret;
}

static struct phy_driver cv182xa_phy_driver[] = {
{
	.phy_id		= 0x00435649,
	.phy_id_mask	= 0xffffffff,
	.drv.name		= "CVITEK CV182XA",
    .features	= PHY_BASIC_FEATURES,
	.config_init	= cv182xa_phy_config_init,
} };

MODULE_DESCRIPTION("CV182XA EPHY driver");
MODULE_AUTHOR("Ethan Chen");
MODULE_LICENSE("GPL");

device_phy_drivers(cv182xa_phy_driver);
