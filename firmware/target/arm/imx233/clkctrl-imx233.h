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

#include "regs/regs-clkctrl.h"

static inline void core_sleep(void)
{
    BF_WR(CLKCTRL_CPU, INTERRUPT_WAIT, 1);
    asm volatile (
        "mcr p15, 0, %0, c7, c0, 4 \n" /* Wait for interrupt */
        "nop\n" /* Datasheet unclear: "The lr sent to handler points here after RTI"*/
        "nop\n"
        : : "r"(0)
    );
    enable_irq();
}

enum imx233_clock_t
{
    CLK_PIX, /* freq, div, frac, bypass, enable */
    CLK_SSP, /* freq, div, bypass, enable */
    CLK_IO, /* freq, frac */
    CLK_CPU, /* freq, div, frac, bypass */
    CLK_HBUS, /* freq, div, frac */
    CLK_PLL, /* freq, enable */
    CLK_XTAL, /* freq */
    CLK_EMI, /* freq, div, frac, bypass */
    CLK_XBUS, /* freq, div */
};

enum imx233_xtal_clk_t
{
    XTAL_FILT = 1 << 30,
    XTAL_DRI = 1 << 28,
    XTAL_TIMROT = 1 << 26,
    XTAM_PWM =  1 << 29,
};

/* Auto-Slow monitoring */
enum imx233_as_monitor_t
{
    AS_NONE = 0, /* Do not monitor any activity */
    AS_CPU_INSTR = 1 << 21, /* Monitor CPU instruction access to AHB */
    AS_CPU_DATA = 1 << 22, /* Monitor CPU data access to AHB */
    AS_TRAFFIC = 1 << 23, /* Monitor AHB master activity */
    AS_TRAFFIC_JAM = 1 << 24, /* Monitor AHB masters (>=3) activity */
    AS_APBXDMA = 1 << 25, /* Monitor APBX DMA activity */
    AS_APBHDMA = 1 << 26, /* Monitor APBH DMA activity */
    AS_PXP = 1 << 27, /* Monitor PXP activity */
    AS_DCP = 1 << 28, /* Monitor DCP activity */
    AS_ALL = 0xff << 21, /* Monitor all activity */
};

enum imx233_as_div_t
{
    AS_DIV_1 = 0,
    AS_DIV_2 = 1,
    AS_DIV_4 = 2,
    AS_DIV_8 = 3,
    AS_DIV_16 = 4,
    AS_DIV_32 = 5
};

/* can use a mask of clocks */
void imx233_clkctrl_enable_xtal(enum imx233_xtal_clk_t xtal_clk, bool enable);
void imx233_clkctrl_is_xtal_enabled(enum imx233_xtal_clk_t xtal_clk, bool enable);
/* only use it for non-fractional clocks (ie not for IO) */
void imx233_clkctrl_enable_clock(enum imx233_clock_t clk, bool enable);
bool imx233_clkctrl_is_clock_enabled(enum imx233_clock_t cl);
void imx233_clkctrl_set_clock_divisor(enum imx233_clock_t clk, int div);
int imx233_clkctrl_get_clock_divisor(enum imx233_clock_t clk);
/* call with fracdiv=0 to disable it */
void imx233_clkctrl_set_fractional_divisor(enum imx233_clock_t clk, int fracdiv);
/* 0 means fractional dividor disable */
int imx233_clkctrl_get_fractional_divisor(enum imx233_clock_t clk);
void imx233_clkctrl_set_bypass_pll(enum imx233_clock_t clk, bool bypass);
bool imx233_clkctrl_get_bypass_pll(enum imx233_clock_t clk);
void imx233_clkctrl_enable_usb_pll(bool enable);
bool imx233_clkctrl_is_usb_pll_enabled(void);
unsigned imx233_clkctrl_get_clock_freq(enum imx233_clock_t clk);

bool imx233_clkctrl_is_emi_sync_enabled(void);

void imx233_clkctrl_set_auto_slow_divisor(enum imx233_as_div_t div);
enum imx233_as_div_t imx233_clkctrl_get_auto_slow_divisor(void);
void imx233_clkctrl_enable_auto_slow(bool enable);
bool imx233_clkctrl_is_auto_slow_enabled(void);
/* can use a mask of clocks */
void imx233_clkctrl_enable_auto_slow_monitor(enum imx233_as_monitor_t monitor, bool enable);
bool imx233_clkctrl_is_auto_slow_monitor_enabled(enum imx233_as_monitor_t monitor);

#endif /* CLKCTRL_IMX233_H */
