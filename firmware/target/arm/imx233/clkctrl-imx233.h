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

#define HW_CLKCTRL_PLLCTRL0 (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x0))
#define HW_CLKCTRL_PLLCTRL0__POWER          (1 << 16)
#define HW_CLKCTRL_PLLCTRL0__EN_USB_CLKS    (1 << 18)
#define HW_CLKCTRL_PLLCTRL0__DIV_SEL_BP     20
#define HW_CLKCTRL_PLLCTRL0__DIV_SEL_BM     (3 << 20)

#define HW_CLKCTRL_PLLCTRL1 (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x10))
#define HW_CLKCTRL_PLLCTRL1__LOCK       (1 << 31)

#define HW_CLKCTRL_CPU      (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x20))
#define HW_CLKCTRL_CPU__DIV_CPU_BP  0
#define HW_CLKCTRL_CPU__DIV_CPU_BM  0x3f
#define HW_CLKCTRL_CPU__INTERRUPT_WAIT  (1 << 12)
#define HW_CLKCTRL_CPU__DIV_XTAL_BP 16
#define HW_CLKCTRL_CPU__DIV_XTAL_BM (0x3ff << 16)
#define HW_CLKCTRL_CPU__DIV_XTAL_FRAC_EN    (1 << 26)
#define HW_CLKCTRL_CPU__BUSY_REF_CPU    (1 << 28)

#define HW_CLKCTRL_HBUS     (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x30))
#define HW_CLKCTRL_HBUS__DIV_BP         0
#define HW_CLKCTRL_HBUS__DIV_BM         0x1f
#define HW_CLKCTRL_HBUS__DIV_FRAC_EN    (1 << 5)
#define HW_CLKCTRL_HBUS__SLOW_DIV_BP    16
#define HW_CLKCTRL_HBUS__SLOW_DIV_BM    (0x7 << 16)
#define HW_CLKCTRL_HBUS__AUTO_SLOW_MODE (1 << 20)

/* warning: this register doesn't have a CLR/SET variant ! */
#define HW_CLKCTRL_XBUS     (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x40))
#define HW_CLKCTRL_XBUS__DIV_BP     0
#define HW_CLKCTRL_XBUS__DIV_BM     0x3ff
#define HW_CLKCTRL_XBUS__BUSY       (1 << 31)

#define HW_CLKCTRL_XTAL     (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x50))
#define HW_CLKCTRL_XTAL__TIMROT_CLK32K_GATE (1 << 26)
#define HW_CLKCTRL_XTAL__DRI_CLK24M_GATE    (1 << 28)
#define HW_CLKCTRL_XTAL__FILT_CLK24M_GATE   (1 << 30)

/* warning: this register doesn't have a CLR/SET variant ! */
#define HW_CLKCTRL_PIX      (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x60))
#define HW_CLKCTRL_PIX__DIV_BP  0
#define HW_CLKCTRL_PIX__DIV_BM  0xfff

/* warning: this register doesn't have a CLR/SET variant ! */
#define HW_CLKCTRL_SSP      (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x70))
#define HW_CLKCTRL_SSP__DIV_BP  0
#define HW_CLKCTRL_SSP__DIV_BM  0x1ff

/* warning: this register doesn't have a CLR/SET variant ! */
#define HW_CLKCTRL_EMI      (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0xa0))
#define HW_CLKCTRL_EMI__DIV_EMI_BP  0
#define HW_CLKCTRL_EMI__DIV_EMI_BM  0x3f
#define HW_CLKCTRL_EMI__DIV_XTAL_BP 8
#define HW_CLKCTRL_EMI__DIV_XTAL_BM (0xf << 8)
#define HW_CLKCTRL_EMI__BUSY_REF_EMI    (1 << 28)
#define HW_CLKCTRL_EMI__SYNC_MODE_EN    (1 << 30)
#define HW_CLKCTRL_EMI__CLKGATE     (1 << 31)

#define HW_CLKCTRL_CLKSEQ   (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x110))
#define HW_CLKCTRL_CLKSEQ__BYPASS_PIX   (1 << 1)
#define HW_CLKCTRL_CLKSEQ__BYPASS_SSP   (1 << 5)
#define HW_CLKCTRL_CLKSEQ__BYPASS_EMI   (1 << 6)
#define HW_CLKCTRL_CLKSEQ__BYPASS_CPU   (1 << 7)

#define HW_CLKCTRL_FRAC     (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0xf0))
#define HW_CLKCTRL_FRAC_CPU (*(volatile uint8_t *)(HW_CLKCTRL_BASE + 0xf0))
#define HW_CLKCTRL_FRAC_EMI (*(volatile uint8_t *)(HW_CLKCTRL_BASE + 0xf1))
#define HW_CLKCTRL_FRAC_PIX (*(volatile uint8_t *)(HW_CLKCTRL_BASE + 0xf2))
#define HW_CLKCTRL_FRAC_IO  (*(volatile uint8_t *)(HW_CLKCTRL_BASE + 0xf3))
#define HW_CLKCTRL_FRAC_XX__XXDIV_BM    0x3f
#define HW_CLKCTRL_FRAC_XX__XX_STABLE   (1 << 6)
#define HW_CLKCTRL_FRAC_XX__CLKGATEXX   (1 << 7)

/* warning: this register doesn't have a CLR/SET variant ! */
#define HW_CLKCTRL_RESET    (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x120))
#define HW_CLKCTRL_RESET_CHIP   0x2
#define HW_CLKCTRL_RESET_DIG    0x1

static inline void core_sleep(void)
{
    __REG_SET(HW_CLKCTRL_CPU) = HW_CLKCTRL_CPU__INTERRUPT_WAIT;
    asm volatile (
        "mcr p15, 0, %0, c7, c0, 4 \n" /* Wait for interrupt */
        "nop\n" /* Datasheet unclear: "The lr sent to handler points here after RTI"*/
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
    CLK_EMI, /* freq */
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
    AS_CPU_INSTR = 1 << 21, /* Monitor CPU instruction access to AHB */
    AS_CPU_DATA = 1 << 22, /* Monitor CPU data access to AHB */
    AS_TRAFFIC = 1 << 23, /* Monitor AHB master activity */
    AS_TRAFFIC_JAM = 1 << 24, /* Monitor AHB masters (>=3) activity */
    AS_APBXDMA = 1 << 25, /* Monitor APBX DMA activity */
    AS_APBHDMA = 1 << 26, /* Monitor APBH DMA activity */
    AS_PXP = 1 << 27, /* Monitor PXP activity */
    AS_DCP = 1 << 28, /* Monitor DCP activity */
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

void imx233_clkctrl_set_auto_slow_divisor(enum imx233_as_div_t div);
enum imx233_as_div_t imx233_clkctrl_get_auto_slow_divisor(void);
void imx233_clkctrl_enable_auto_slow(bool enable);
bool imx233_clkctrl_is_auto_slow_enabled(void);
void imx233_clkctrl_enable_auto_slow_monitor(enum imx233_as_monitor_t monitor, bool enable);
bool imx233_clkctrl_is_auto_slow_monitor_enabled(enum imx233_as_monitor_t monitor);

#endif /* CLKCTRL_IMX233_H */
