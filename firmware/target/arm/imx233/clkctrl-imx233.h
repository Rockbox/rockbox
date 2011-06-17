/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2011 by Amaury Pouly
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
#ifndef CLKCTRL_IMX233_H
#define CLKCTRL_IMX233_H

#include "config.h"
#include "system.h"
#include "cpu.h"

#define HW_CLKCTRL_BASE     0x80040000

#define HW_CLKCTRL_XTAL     (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x50))
#define HW_CLKCTRL_XTAL__TIMROT_CLK32K_GATE (1 << 26)

#define HW_CLKCTRL_PIX      (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x60))
#define HW_CLKCTRL_SSP      (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x70))

#define HW_CLKCTRL_CLKSEQ   (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x110))
#define HW_CLKCTRL_CLKSEQ__BYPASS_PIX   (1 << 1)
#define HW_CLKCTRL_CLKSEQ__BYPASS_SSP   (1 << 5)

#define HW_CLKCTRL_FRAC     (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0xf0))
#define HW_CLKCTRL_FRAC_CPU (*(volatile uint8_t *)(HW_CLKCTRL_BASE + 0xf0))
#define HW_CLKCTRL_FRAC_EMI (*(volatile uint8_t *)(HW_CLKCTRL_BASE + 0xf1))
#define HW_CLKCTRL_FRAC_PIX (*(volatile uint8_t *)(HW_CLKCTRL_BASE + 0xf2))
#define HW_CLKCTRL_FRAC_IO  (*(volatile uint8_t *)(HW_CLKCTRL_BASE + 0xf3))
#define HW_CLKCTRL_FRAC_XX__XXDIV_BM    0x3f
#define HW_CLKCTRL_FRAC_XX__XX_STABLE   (1 << 6)
#define HW_CLKCTRL_FRAC_XX__CLKGATEXX   (1 << 7)

#define HW_CLKCTRL_RESET    (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x120))
#define HW_CLKCTRL_RESET_CHIP   0x2
#define HW_CLKCTRL_RESET_DIG    0x1

enum imx233_clock_t
{
    CLK_PIX,
    CLK_SSP,
    CLK_IO,
};

void imx233_enable_timrot_xtal_clk32k(bool enable);
/* only use it for non-fractional clocks (ie not for IO) */
void imx233_enable_clock(enum imx233_clock_t clk, bool enable);
void imx233_set_clock_divisor(enum imx233_clock_t clk, int div);
/* call with fracdiv=0 to disable it */
void imx233_set_fractional_divisor(enum imx233_clock_t clk, int fracdiv);
void imx233_set_bypass_pll(enum imx233_clock_t clk, bool bypass);

#endif /* CLKCTRL_IMX233_H */
