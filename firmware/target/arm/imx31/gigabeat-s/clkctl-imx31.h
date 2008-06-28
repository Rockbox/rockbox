/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2008 Michael Sevakis
 *
 * Clock control functions for IMX31 processor
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _CLKCTL_IMX31_H_
#define _CLKCTL_IMX31_H_

enum IMX31_CG_LIST
{
    /* CGR0 */
    CG_SD_MMC1 = 0,
    CG_SD_MMC2,
    CG_GPT,
    CG_EPIT1,
    CG_EPIT2,
    CG_IIM,
    CG_ATA,
    CG_SDMA,
    CG_CSPI3,
    CG_RNG,
    CG_UART1,
    CG_UART2,
    CG_SSI1,
    CG_I2C1,
    CG_I2C2,
    CG_I2C3,
    /* CGR1 */
    CG_HANTRO,
    CG_MEMSTICK1,
    CG_MEMSTICK2,
    CG_CSI,
    CG_RTC,
    CG_WDOG,
    CG_PWM,
    CG_SIM,
    CG_ECT,
    CG_USBOTG,
    CG_KPP,
    CG_IPU,
    CG_UART3,
    CG_UART4,
    CG_UART5,
    CG_1_WIRE,
    /* CGR2 */
    CG_SSI2,
    CG_CSPI1,
    CG_CSPI2,
    CG_GACC,
    CG_EMI,
    CG_RTIC,
    CG_FIR,
    CG_NUM_CLOCKS
};

enum IMX31_CG_MODES
{
    CGM_OFF         = 0x0, /* Always off */
    CGM_ON_RUN      = 0x1, /* On in run mode, off in wait and doze */
    CGM_ON_RUN_WAIT = 0x2, /* On in run and wait modes, off in doze */
    CGM_ON_ALL      = 0x3, /* Always on */
};

#define CG_MASK  0x3 /* bitmask */

/* Enable or disable module clocks independently - module must _not_ be
 * active! */
void imx31_clkctl_module_clock_gating(enum IMX31_CG_LIST cg,
                                      enum IMX31_CG_MODES mode);

enum IMX31_PLLS
{
    PLL_MCU = 0,
    PLL_USB,
    PLL_SERIAL,
    NUM_PLLS,
};

#define CONFIG_CLK32_FREQ   32768
#define CONFIG_HCLK_FREQ    27000000

/* Get the PLL reference clock frequency in HZ */
unsigned int imx31_clkctl_get_pll_ref_clk(void);

/* Return PLL frequency in HZ */
unsigned int imx31_clkctl_get_pll(enum IMX31_PLLS pll);

/* Return ipg_clk in HZ */
unsigned int imx31_clkctl_get_ipg_clk(void);

/* Return the ATA frequency in HZ */
unsigned int imx31_clkctl_get_ata_clk(void);

#endif /* _CLKCTL_IMX31_H_ */
