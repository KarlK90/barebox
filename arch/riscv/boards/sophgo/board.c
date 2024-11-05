// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 Stefan Kerkmann, Pengutronix
 */

#include <common.h>
#include <driver.h>
#include <bbu.h>
#include <envfs.h>
#include "cv181x_reg.h"
#include "cv181x_reg_fmux_gpio.h"
#include "cv181x_pinlist_swconfig.h"

static void mmio_write_32(uintptr_t addr, uint32_t val)
{
	writel(val, IOMEM(addr));
}

static uint32_t mmio_read_32(uintptr_t addr)
{
	return readl(IOMEM(addr));
}

static void mmio_clrsetbits_32(uintptr_t addr, uint32_t clr, uint32_t set)
{
	clrsetbits_le32(addr, clr, set);
}

#define PINMUX_CONFIG(PIN_NAME, FUNC_NAME) \
		mmio_clrsetbits_32(PINMUX_BASE + FMUX_GPIO_FUNCSEL_##PIN_NAME, \
			FMUX_GPIO_FUNCSEL_##PIN_NAME##_MASK << FMUX_GPIO_FUNCSEL_##PIN_NAME##_OFFSET, \
			PIN_NAME##__##FUNC_NAME)

static void bm_eth_reset_phy(void)
{
	void __iomem *ephy_base_reg = IOMEM(0x03009000);
	void __iomem *ephy_top_reg = IOMEM(0x03009800);

	// set rg_ephy_apb_rw_sel 0x0804@[0]=1/APB by using APB interface
	writel(0x0001, ephy_top_reg + 0x4);

	// Release 0x0800[0]=0/shutdown
	writel(0x0900, ephy_top_reg);

	// Release 0x0800[2]=1/dig_rst_n, Let mii_reg can be accessabile
	writel(0x0904, ephy_top_reg);

	mdelay(10);

	// ANA INIT (PD/EN), switch to MII-page5
	writel(0x0500, ephy_base_reg + 0x7c);
	// Release ANA_PD p5.0x10@[13:8] = 6'b001100
	writel(0x0c00, ephy_base_reg + 0x40);
	// Release ANA_EN p5.0x10@[7:0] = 8'b01111110
	writel(0x0c7e, ephy_base_reg + 0x40);

	// Wait PLL_Lock, Lock_Status p5.0x12@[15] = 1
	mdelay(1);

	// Release 0x0800[1] = 1/ana_rst_n
	writel(0x0906, ephy_top_reg);

	// ANA INIT
	// @Switch to MII-page5
	writel(0x0500, ephy_base_reg + 0x7c);
	// PHY_ID
	writel(0x0043, ephy_base_reg + 0x8);
	writel(0x5649, ephy_base_reg + 0xc);
	// switch to MDIO control by ETH_MAC
	writel(0x0, ephy_top_reg + 0x4);
}

static void cv181x_ephy_led_pinmux(void)
{
	// LED PAD MUX
	writel(0x05, 0x030010e0);
	writel(0x05, 0x030010e4);
	//(SD1_CLK selphy)
	writel(0x11111111, 0x050270b0);
	//(SD1_CMD selphy)
	writel(0x11111111, 0x050270b4);
}

static void set_rtc_register_for_power(void)
{
	// Reset Key
	mmio_write_32(0x050260D0, 0x7);
}

static void cvi_board_init(void)
{
	// PINMUX_SDIO1
	/*
			 * Name            Address            SD1  MIPI
			 * reg_sd1_phy_sel REG_0x300_0294[10] 0x0  0x1
			 */
	mmio_write_32(TOP_BASE + 0x294,
		      (mmio_read_32(TOP_BASE + 0x294) & 0xFFFFFBFF));
	PINMUX_CONFIG(SD1_CMD, PWR_SD1_CMD_VO36);
	PINMUX_CONFIG(SD1_CLK, PWR_SD1_CLK_VO37);
	PINMUX_CONFIG(SD1_D0, PWR_SD1_D0_VO35);
	PINMUX_CONFIG(SD1_D1, PWR_SD1_D1_VO34);
	PINMUX_CONFIG(SD1_D2, PWR_SD1_D2_VO33);
	PINMUX_CONFIG(SD1_D3, PWR_SD1_D3_VO32);

	// Camera0
	PINMUX_CONFIG(IIC3_SCL, IIC3_SCL);
	PINMUX_CONFIG(IIC3_SDA, IIC3_SDA);
	PINMUX_CONFIG(CAM_MCLK0, CAM_MCLK0); // Sensor0 MCLK
	PINMUX_CONFIG(CAM_RST0, XGPIOA_2);   // Sensor0 RESET

	// Camera1
	PINMUX_CONFIG(IIC2_SDA, IIC2_SDA);
	PINMUX_CONFIG(IIC2_SCL, IIC2_SCL);
	PINMUX_CONFIG(CAM_MCLK1, CAM_MCLK1); // Sensor1 MCLK
	PINMUX_CONFIG(CAM_PD1, XGPIOA_4);    // Sensor1 RESET

	// LED
	PINMUX_CONFIG(IIC0_SDA, XGPIOA_29);

	// I2C4 for TP
	PINMUX_CONFIG(VIVO_D1, IIC4_SCL);
	PINMUX_CONFIG(VIVO_D0, IIC4_SDA);

	// TP INT
	PINMUX_CONFIG(JTAG_CPU_TCK, XGPIOA_18);
	// TP Reset
	PINMUX_CONFIG(JTAG_CPU_TMS, XGPIOA_19);

	// SPI3
	PINMUX_CONFIG(VIVO_D8, SPI3_SDO);
	PINMUX_CONFIG(VIVO_D7, SPI3_SDI);
	PINMUX_CONFIG(VIVO_D6, SPI3_SCK);
	PINMUX_CONFIG(VIVO_D5, SPI3_CS_X);

	// USB
	PINMUX_CONFIG(USB_VBUS_EN, XGPIOB_5);

	// WIFI/BT
	PINMUX_CONFIG(CLK32K, PWR_GPIO_10);
	PINMUX_CONFIG(UART2_RX, UART4_RX);
	PINMUX_CONFIG(UART2_TX, UART4_TX);
	PINMUX_CONFIG(UART2_CTS, UART4_CTS);
	PINMUX_CONFIG(UART2_RTS, UART4_RTS);

	// GPIOs
	PINMUX_CONFIG(JTAG_CPU_TCK, XGPIOA_18);
	PINMUX_CONFIG(JTAG_CPU_TMS, XGPIOA_19);
	PINMUX_CONFIG(JTAG_CPU_TRST, XGPIOA_20);
	PINMUX_CONFIG(IIC0_SCL, XGPIOA_28);

	// EPHY LEDs
	PINMUX_CONFIG(PWR_WAKEUP0, EPHY_LNK_LED);
	PINMUX_CONFIG(PWR_BUTTON1, EPHY_SPD_LED);

	set_rtc_register_for_power();
}


static int milkv_duo_s_probe(struct device *dev)
{
	barebox_set_hostname("milkv");

	writel(0x30001, 0x01901008);// cortexa53_pwr_iso_en

	bm_eth_reset_phy();
	cv181x_ephy_led_pinmux();
	cvi_board_init();

	return 0;
}

static const struct of_device_id milkv_duo_s_of_match[] = {
	{ .compatible = "milkv,duo-s" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, milkv_duo_s_of_match);

static struct driver milkv_duo_s_board_driver = {
	.name = "board-milkv-s",
	.probe = milkv_duo_s_probe,
	.of_compatible = milkv_duo_s_of_match,
};
device_platform_driver(milkv_duo_s_board_driver);
