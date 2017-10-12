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

#include "regs/clkctrl.h"

static inline void core_sleep(void)
{
    BF_WR(CLKCTRL_CPU, INTERRUPT_WAIT(1));
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
    CLK_SSP, /* freq, div, bypass, enable */
    CLK_IO, /* freq, frac (stmp3700+) */
    CLK_CPU, /* freq, div, frac, bypass */
    CLK_HBUS, /* freq, div, frac */
    CLK_PLL, /* freq, enable */
    CLK_XTAL, /* freq */
    CLK_EMI, /* freq, div, frac, bypass */
    CLK_XBUS, /* freq, div */
    CLK_FILT, /* enable */
    CLK_DRI, /* enable */
    CLK_PWM, /* enable */
    CLK_TIMROT,  /* enable */
    CLK_UART, /* enable */
#if IMX233_SUBTARGET >= 3700
    CLK_PIX, /* freq, div, frac, bypass, enable */
#endif
};

void imx233_clkctrl_init(void);
/* only use it for non-fractional clocks (ie not for IO) */
void imx233_clkctrl_enable(enum imx233_clock_t clk, bool enable);
bool imx233_clkctrl_is_enabled(enum imx233_clock_t cl);
void imx233_clkctrl_set_div(enum imx233_clock_t clk, int div);
int imx233_clkctrl_get_div(enum imx233_clock_t clk);
#if IMX233_SUBTARGET >= 3700
/* call with fracdiv=0 to disable it */
void imx233_clkctrl_set_frac_div(enum imx233_clock_t clk, int fracdiv);
/* 0 means fractional dividor disabled */
int imx233_clkctrl_get_frac_div(enum imx233_clock_t clk);
void imx233_clkctrl_set_bypass(enum imx233_clock_t clk, bool bypass);
bool imx233_clkctrl_get_bypass(enum imx233_clock_t clk);
/* all-in-one function which handle all quirks */
void imx233_clkctrl_set_cpu_hbus_div(int cpu_idiv, int cpu_fdiv, int hbus_div);
#endif
void imx233_clkctrl_enable_usb(bool enable);
bool imx233_clkctrl_is_usb_enabled(void);
/* returns frequency in KHz */
unsigned imx233_clkctrl_get_freq(enum imx233_clock_t clk);
/* auto-slow stuff */
void imx233_clkctrl_set_auto_slow_div(unsigned div);
unsigned imx233_clkctrl_get_auto_slow_div(void);
void imx233_clkctrl_enable_auto_slow(bool enable);
bool imx233_clkctrl_is_auto_slow_enabled(void);

#endif /* CLKCTRL_IMX233_H */
