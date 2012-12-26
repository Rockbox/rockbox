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
#include "clkctrl-imx233.h"

#define __CLK_CLKGATE   (1 << 31)
#define __CLK_BUSY      (1 << 29)

void imx233_clkctrl_enable_xtal(enum imx233_xtal_clk_t xtal_clk, bool enable)
{
    if(enable)
        __REG_CLR(HW_CLKCTRL_XTAL) = xtal_clk;
    else
        __REG_SET(HW_CLKCTRL_XTAL) = xtal_clk;
}

bool imx233_clkctrl_is_xtal_enable(enum imx233_xtal_clk_t clk)
{
    return HW_CLKCTRL_XTAL & clk;
}

void imx233_clkctrl_enable_clock(enum imx233_clock_t clk, bool enable)
{
    volatile uint32_t *REG;
    switch(clk)
    {
        case CLK_PIX: REG = &HW_CLKCTRL_PIX; break;
        case CLK_SSP: REG = &HW_CLKCTRL_SSP; break;
        case CLK_PLL:
        {
            if(enable)
            {
                __REG_SET(HW_CLKCTRL_PLLCTRL0) = HW_CLKCTRL_PLLCTRL0__POWER;
                while(!(HW_CLKCTRL_PLLCTRL1 & HW_CLKCTRL_PLLCTRL1__LOCK));
            }
            else
                __REG_CLR(HW_CLKCTRL_PLLCTRL0) = HW_CLKCTRL_PLLCTRL0__POWER;
            return;
        }
        default: return;
    }

    /* warning: some registers like HW_CLKCTRL_PIX don't have a CLR/SET variant ! */
    if(enable)
    {
        *REG = (*REG) & ~__CLK_CLKGATE;
        while((*REG) & __CLK_CLKGATE);
        while((*REG) & __CLK_BUSY);
    }
    else
    {
        *REG |= __CLK_CLKGATE;
        while(!((*REG) & __CLK_CLKGATE));
    }
}

bool imx233_clkctrl_is_clock_enabled(enum imx233_clock_t clk)
{
    volatile uint32_t *REG;
    switch(clk)
    {
        case CLK_PLL: return HW_CLKCTRL_PLLCTRL0 & HW_CLKCTRL_PLLCTRL0__POWER;
        case CLK_PIX: REG = &HW_CLKCTRL_PIX; break;
        case CLK_SSP: REG = &HW_CLKCTRL_SSP; break;
        default: return true;
    }

    return !((*REG) & __CLK_CLKGATE);
}

void imx233_clkctrl_set_clock_divisor(enum imx233_clock_t clk, int div)
{
    /* warning: some registers like HW_CLKCTRL_PIX don't have a CLR/SET variant ! */
    switch(clk)
    {
        case CLK_PIX:
            __FIELD_SET(HW_CLKCTRL_PIX, DIV, div);
            break;
        case CLK_SSP:
            __FIELD_SET(HW_CLKCTRL_SSP, DIV, div);
            break;
        case CLK_CPU:
            __FIELD_SET(HW_CLKCTRL_CPU, DIV_CPU, div);
            break;
        case CLK_EMI:
            __FIELD_SET(HW_CLKCTRL_EMI, DIV_EMI, div);
            break;
        case CLK_HBUS:
            /* disable frac enable at the same time */
            HW_CLKCTRL_HBUS = div << HW_CLKCTRL_HBUS__DIV_BP |
                (HW_CLKCTRL_HBUS & ~(HW_CLKCTRL_HBUS__DIV_FRAC_EN | HW_CLKCTRL_HBUS__DIV_BM));
            break;
        case CLK_XBUS:
            __FIELD_SET(HW_CLKCTRL_XBUS, DIV, div);
            break;
        default: return;
    }
}

int imx233_clkctrl_get_clock_divisor(enum imx233_clock_t clk)
{
    switch(clk)
    {
        case CLK_PIX: return __XTRACT(HW_CLKCTRL_PIX, DIV);
        case CLK_SSP: return __XTRACT(HW_CLKCTRL_SSP, DIV);
        case CLK_CPU: return __XTRACT(HW_CLKCTRL_CPU, DIV_CPU);
        case CLK_EMI: return __XTRACT(HW_CLKCTRL_EMI, DIV_EMI);
        case CLK_HBUS:
            if(HW_CLKCTRL_HBUS & HW_CLKCTRL_HBUS__DIV_FRAC_EN)
                return 0;
            else
                return __XTRACT(HW_CLKCTRL_HBUS, DIV);
        case CLK_XBUS: return __XTRACT(HW_CLKCTRL_XBUS, DIV);
        default: return 0;
    }
}

void imx233_clkctrl_set_fractional_divisor(enum imx233_clock_t clk, int fracdiv)
{
    /* NOTE: HW_CLKCTRL_FRAC only support byte access ! */
    volatile uint8_t *REG;
    switch(clk)
    {
        case CLK_HBUS:
            /* set frac enable at the same time */
            HW_CLKCTRL_HBUS = fracdiv << HW_CLKCTRL_HBUS__DIV_BP | HW_CLKCTRL_HBUS__DIV_FRAC_EN |
                (HW_CLKCTRL_HBUS & ~HW_CLKCTRL_HBUS__DIV_BM);
            return;
        case CLK_PIX: REG = &HW_CLKCTRL_FRAC_PIX; break;
        case CLK_IO: REG = &HW_CLKCTRL_FRAC_IO; break;
        case CLK_CPU: REG = &HW_CLKCTRL_FRAC_CPU; break;
        case CLK_EMI: REG = &HW_CLKCTRL_FRAC_EMI; break;
        default: return;
    }

    if(fracdiv != 0)
        *REG = fracdiv;
    else
        *REG = HW_CLKCTRL_FRAC_XX__CLKGATEXX;
}

int imx233_clkctrl_get_fractional_divisor(enum imx233_clock_t clk)
{
    /* NOTE: HW_CLKCTRL_FRAC only support byte access ! */
    volatile uint8_t *REG;
    switch(clk)
    {
        case CLK_HBUS:
            if(HW_CLKCTRL_HBUS & HW_CLKCTRL_HBUS__DIV_FRAC_EN)
                return __XTRACT(HW_CLKCTRL_HBUS, DIV);
            else
                return 0;
        case CLK_PIX: REG = &HW_CLKCTRL_FRAC_PIX; break;
        case CLK_IO: REG = &HW_CLKCTRL_FRAC_IO; break;
        case CLK_CPU: REG = &HW_CLKCTRL_FRAC_CPU; break;
        case CLK_EMI: REG = &HW_CLKCTRL_FRAC_EMI; break;
        default: return 0;
    }

    if((*REG) & HW_CLKCTRL_FRAC_XX__CLKGATEXX)
        return 0;
    else
        return *REG & ~HW_CLKCTRL_FRAC_XX__XX_STABLE;
}

void imx233_clkctrl_set_bypass_pll(enum imx233_clock_t clk, bool bypass)
{
    uint32_t msk;
    switch(clk)
    {
        case CLK_PIX: msk = HW_CLKCTRL_CLKSEQ__BYPASS_PIX; break;
        case CLK_SSP: msk = HW_CLKCTRL_CLKSEQ__BYPASS_SSP; break;
        case CLK_CPU: msk = HW_CLKCTRL_CLKSEQ__BYPASS_CPU; break;
        case CLK_EMI: msk = HW_CLKCTRL_CLKSEQ__BYPASS_EMI; break;
        default: return;
    }

    if(bypass)
        __REG_SET(HW_CLKCTRL_CLKSEQ) = msk;
    else
        __REG_CLR(HW_CLKCTRL_CLKSEQ) = msk;
}

bool imx233_clkctrl_get_bypass_pll(enum imx233_clock_t clk)
{
    uint32_t msk;
    switch(clk)
    {
        case CLK_PIX: msk = HW_CLKCTRL_CLKSEQ__BYPASS_PIX; break;
        case CLK_SSP: msk = HW_CLKCTRL_CLKSEQ__BYPASS_SSP; break;
        case CLK_CPU: msk = HW_CLKCTRL_CLKSEQ__BYPASS_CPU; break;
        case CLK_EMI: msk = HW_CLKCTRL_CLKSEQ__BYPASS_EMI; break;
        default: return false;
    }

    return HW_CLKCTRL_CLKSEQ & msk;
}

void imx233_clkctrl_enable_usb_pll(bool enable)
{
    if(enable)
        __REG_SET(HW_CLKCTRL_PLLCTRL0) = HW_CLKCTRL_PLLCTRL0__EN_USB_CLKS;
    else
        __REG_CLR(HW_CLKCTRL_PLLCTRL0) = HW_CLKCTRL_PLLCTRL0__EN_USB_CLKS;
}

bool imx233_clkctrl_is_usb_pll_enabled(void)
{
    return HW_CLKCTRL_PLLCTRL0 & HW_CLKCTRL_PLLCTRL0__EN_USB_CLKS;
}

void imx233_clkctrl_set_auto_slow_divisor(enum imx233_as_div_t div)
{
    /* the SLOW_DIV must only be set when auto-slow is disabled */
    bool old_status = imx233_clkctrl_is_auto_slow_enabled();
    imx233_clkctrl_enable_auto_slow(false);
    __FIELD_SET(HW_CLKCTRL_HBUS, SLOW_DIV, div);
    imx233_clkctrl_enable_auto_slow(old_status);
}

enum imx233_as_div_t imx233_clkctrl_get_auto_slow_divisor(void)
{
    return __XTRACT(HW_CLKCTRL_HBUS, SLOW_DIV);
}

void imx233_clkctrl_enable_auto_slow(bool enable)
{
    if(enable)
        __REG_SET(HW_CLKCTRL_HBUS) = HW_CLKCTRL_HBUS__AUTO_SLOW_MODE;
    else
        __REG_CLR(HW_CLKCTRL_HBUS) = HW_CLKCTRL_HBUS__AUTO_SLOW_MODE;
}

bool imx233_clkctrl_is_auto_slow_enabled(void)
{
    return HW_CLKCTRL_HBUS & HW_CLKCTRL_HBUS__AUTO_SLOW_MODE;
}

void imx233_clkctrl_enable_auto_slow_monitor(enum imx233_as_monitor_t monitor, bool enable)
{
    if(enable)
        __REG_SET(HW_CLKCTRL_HBUS) = monitor;
    else
        __REG_CLR(HW_CLKCTRL_HBUS) = monitor;
}

bool imx233_clkctrl_is_auto_slow_monitor_enabled(enum imx233_as_monitor_t monitor)
{
    return HW_CLKCTRL_HBUS & monitor;
}

bool imx233_clkctrl_is_emi_sync_enabled(void)
{
    return !!(HW_CLKCTRL_EMI & HW_CLKCTRL_EMI__SYNC_MODE_EN);
}

unsigned imx233_clkctrl_get_clock_freq(enum imx233_clock_t clk)
{
    switch(clk)
    {
        case CLK_PLL: /* PLL: 480MHz when enable */
            return imx233_clkctrl_is_clock_enabled(CLK_PLL) ? 480000 : 0;
        case CLK_XTAL: /* crystal: 24MHz */
            return 24000;
        case CLK_CPU:
        {
            unsigned ref;
            /* In bypass mode: clk_p derived from clk_xtal via int/binfrac divider
             * otherwise, clk_p derived from clk_cpu via int div and clk_cpu
             * derived from clk_pll fracdiv */
            if(imx233_clkctrl_get_bypass_pll(CLK_CPU))
            {
                ref = imx233_clkctrl_get_clock_freq(CLK_XTAL);
                /* Integer divide mode vs fractional divide mode */
                if(HW_CLKCTRL_CPU & HW_CLKCTRL_CPU__DIV_XTAL_FRAC_EN)

                    return (ref * __XTRACT(HW_CLKCTRL_CPU, DIV_XTAL)) / 32;
                else
                    return ref / imx233_clkctrl_get_clock_divisor(CLK_CPU);
            }
            else
            {
                ref = imx233_clkctrl_get_clock_freq(CLK_PLL);
                /* fractional divider enable ? */
                if(imx233_clkctrl_get_fractional_divisor(CLK_CPU) != 0)
                    ref = (ref * 18) / imx233_clkctrl_get_fractional_divisor(CLK_CPU);
                return ref / imx233_clkctrl_get_clock_divisor(CLK_CPU);
            }
        }
        case CLK_HBUS:
        {
            /* Derived from clk_p via integer/fractional div */
            unsigned ref = imx233_clkctrl_get_clock_freq(CLK_CPU);
            if(imx233_clkctrl_get_fractional_divisor(CLK_HBUS) != 0)
                ref = (ref * imx233_clkctrl_get_fractional_divisor(CLK_HBUS)) / 32;
            if(imx233_clkctrl_get_clock_divisor(CLK_HBUS) != 0)
                ref /= imx233_clkctrl_get_clock_divisor(CLK_HBUS);
            return ref;
        }
        case CLK_IO:
        {
            /* Derived from clk_pll via fracdiv */
            unsigned ref = imx233_clkctrl_get_clock_freq(CLK_PLL);
            if(imx233_clkctrl_get_fractional_divisor(CLK_IO) != 0)
                ref = (ref * 18) / imx233_clkctrl_get_fractional_divisor(CLK_IO);
            return ref;
        }
        case CLK_PIX:
        {
            unsigned ref;
            /* Derived from clk_pll or clk_xtal */
            if(!imx233_clkctrl_is_clock_enabled(CLK_PIX))
                ref = 0;
            else if(imx233_clkctrl_get_bypass_pll(CLK_PIX))
                ref = imx233_clkctrl_get_clock_freq(CLK_XTAL);
            else
            {
                ref = imx233_clkctrl_get_clock_freq(CLK_PLL);
                if(imx233_clkctrl_get_fractional_divisor(CLK_PIX) != 0)
                    ref = (ref * 18) / imx233_clkctrl_get_fractional_divisor(CLK_PIX);
            }
            return ref / imx233_clkctrl_get_clock_divisor(CLK_PIX);
        }
        case CLK_SSP:
        {
            unsigned ref;
            /* Derived from clk_pll or clk_xtal */
            if(!imx233_clkctrl_is_clock_enabled(CLK_SSP))
                ref = 0;
            else if(imx233_clkctrl_get_bypass_pll(CLK_SSP))
                ref = imx233_clkctrl_get_clock_freq(CLK_XTAL);
            else
                ref = imx233_clkctrl_get_clock_freq(CLK_IO);
            return ref / imx233_clkctrl_get_clock_divisor(CLK_SSP);
        }
        case CLK_EMI:
        {
            unsigned ref;
            /* Derived from clk_pll or clk_xtal */
            if(imx233_clkctrl_get_bypass_pll(CLK_EMI))
            {
                ref = imx233_clkctrl_get_clock_freq(CLK_XTAL);
                if(HW_CLKCTRL_EMI & HW_CLKCTRL_EMI__CLKGATE)
                    return 0;
                else
                    return ref / __XTRACT(HW_CLKCTRL_EMI, DIV_XTAL);
            }
            else
            {
                ref = imx233_clkctrl_get_clock_freq(CLK_PLL);
                if(imx233_clkctrl_get_fractional_divisor(CLK_EMI) != 0)
                    ref = (ref * 18) / imx233_clkctrl_get_fractional_divisor(CLK_EMI);
                return ref / imx233_clkctrl_get_clock_divisor(CLK_EMI);
            }
        }
        case CLK_XBUS:
            return imx233_clkctrl_get_clock_freq(CLK_XTAL) /
                    imx233_clkctrl_get_clock_divisor(CLK_XBUS);
        default:
            return 0;
    }
}
