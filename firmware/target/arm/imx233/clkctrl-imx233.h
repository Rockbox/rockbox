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

#define HW_CLKCTRL_CLKSEQ   (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x110))
#define HW_CLKCTRL_CLKSEQ__BYPASS_PIX   (1 << 1)

#define HW_CLKCTRL_RESET    (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x120))
#define HW_CLKCTRL_RESET_CHIP   0x2
#define HW_CLKCTRL_RESET_DIG    0x1

enum imx233_clock_t
{
    CLK_PIX,
};

void imx233_enable_timrot_xtal_clk32k(bool enable);
void imx233_enable_clock(enum imx233_clock_t clk, bool enable);
void imx233_set_clock_divisor(enum imx233_clock_t clk, int div);
void imx233_set_bypass_pll(enum imx233_clock_t clk, bool bypass);

#endif /* CLKCTRL_IMX233_H */
