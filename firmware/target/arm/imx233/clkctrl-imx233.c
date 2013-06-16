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
        HW_CLKCTRL_XTAL_CLR = xtal_clk;
    else
        HW_CLKCTRL_XTAL_SET = xtal_clk;
}

bool imx233_clkctrl_is_xtal_enable(enum imx233_xtal_clk_t clk)
{
    return HW_CLKCTRL_XTAL & clk;
}

void imx233_clkctrl_enable_clock(enum imx233_clock_t clk, bool enable)
{
    bool gate = !enable;
    switch(clk)
    {
        case CLK_PIX: BF_WR(CLKCTRL_PIX, CLKGATE, gate); break;
        case CLK_SSP: BF_WR(CLKCTRL_SSP, CLKGATE, gate); break;
        case CLK_PLL:
            /* pll is a special case */
            if(enable)
            {
                BF_SET(CLKCTRL_PLLCTRL0, POWER);
                while(!BF_RD(CLKCTRL_PLLCTRL1, LOCK));
            }
            else
                BF_CLR(CLKCTRL_PLLCTRL0, POWER);
            break;
        default:
            break;
    }
}

bool imx233_clkctrl_is_clock_enabled(enum imx233_clock_t clk)
{
    switch(clk)
    {
        case CLK_PLL: return BF_RD(CLKCTRL_PLLCTRL0, POWER);
        case CLK_PIX: return !BF_RD(CLKCTRL_PIX, CLKGATE);
        case CLK_SSP: return !BF_RD(CLKCTRL_SSP, CLKGATE);
        default: return true;
    }
}

void imx233_clkctrl_set_clock_divisor(enum imx233_clock_t clk, int div)
{
    /* warning: some registers like HW_CLKCTRL_PIX don't have a CLR/SET variant !
     * assume that we always derive emi and cpu from ref_XX */
    switch(clk)
    {
        case CLK_PIX: BF_WR(CLKCTRL_PIX, DIV, div); break;
        case CLK_CPU: BF_WR(CLKCTRL_CPU, DIV_CPU, div); break;
        case CLK_EMI: BF_WR(CLKCTRL_EMI, DIV_EMI, div); break;
        case CLK_SSP: BF_WR(CLKCTRL_SSP, DIV, div); break;
        case CLK_HBUS: BF_WR(CLKCTRL_HBUS, DIV, div); break;
        case CLK_XBUS: BF_WR(CLKCTRL_XBUS, DIV, div); break;
        default: return;
    }
}

int imx233_clkctrl_get_clock_divisor(enum imx233_clock_t clk)
{
    switch(clk)
    {
        case CLK_PIX: return BF_RD(CLKCTRL_PIX, DIV);
        case CLK_CPU: return BF_RD(CLKCTRL_CPU, DIV_CPU);
        case CLK_EMI: return BF_RD(CLKCTRL_EMI, DIV_EMI);
        case CLK_SSP: return BF_RD(CLKCTRL_SSP, DIV);
        case CLK_HBUS: return BF_RD(CLKCTRL_HBUS, DIV);
        case CLK_XBUS: return BF_RD(CLKCTRL_XBUS, DIV);
        default: return 0;
    }
}

void imx233_clkctrl_set_fractional_divisor(enum imx233_clock_t clk, int fracdiv)
{
#define handle_frac(dev) \
    case CLK_##dev: \
        if(fracdiv == 0) \
            BF_SET(CLKCTRL_FRAC, CLKGATE##dev); \
        else { \
            BF_WR(CLKCTRL_FRAC, dev##FRAC, fracdiv); \
            BF_CLR(CLKCTRL_FRAC, CLKGATE##dev); } \
        break;
    switch(clk)
    {
        handle_frac(PIX)
        handle_frac(IO)
        handle_frac(CPU)
        handle_frac(EMI)
        default: break;
    }
#undef handle_frac
}

int imx233_clkctrl_get_fractional_divisor(enum imx233_clock_t clk)
{
#define handle_frac(dev) \
    case CLK_##dev:\
        if(BF_RD(CLKCTRL_FRAC, CLKGATE##dev)) \
            return 0; \
        else \
            return BF_RD(CLKCTRL_FRAC, dev##FRAC);
    switch(clk)
    {
        handle_frac(PIX)
        handle_frac(IO)
        handle_frac(CPU)
        handle_frac(EMI)
        default: return 0;
    }
#undef handle_frac
}

void imx233_clkctrl_set_bypass_pll(enum imx233_clock_t clk, bool bypass)
{
    uint32_t msk;
    switch(clk)
    {
        case CLK_PIX: msk = BM_CLKCTRL_CLKSEQ_BYPASS_PIX; break;
        case CLK_SSP: msk = BM_CLKCTRL_CLKSEQ_BYPASS_SSP; break;
        case CLK_CPU: msk = BM_CLKCTRL_CLKSEQ_BYPASS_CPU; break;
        case CLK_EMI: msk = BM_CLKCTRL_CLKSEQ_BYPASS_EMI; break;
        default: return;
    }

    if(bypass)
        HW_CLKCTRL_CLKSEQ_SET = msk;
    else
        HW_CLKCTRL_CLKSEQ_CLR = msk;
}

bool imx233_clkctrl_get_bypass_pll(enum imx233_clock_t clk)
{
    switch(clk)
    {
        case CLK_PIX: return BF_RD(CLKCTRL_CLKSEQ, BYPASS_PIX);
        case CLK_SSP: return BF_RD(CLKCTRL_CLKSEQ, BYPASS_SSP);
        case CLK_CPU: return BF_RD(CLKCTRL_CLKSEQ, BYPASS_CPU);
        case CLK_EMI: return BF_RD(CLKCTRL_CLKSEQ, BYPASS_EMI);
        default: return false;
    }
}

void imx233_clkctrl_enable_usb_pll(bool enable)
{
    if(enable)
        BF_SET(CLKCTRL_PLLCTRL0, EN_USB_CLKS);
    else
        BF_CLR(CLKCTRL_PLLCTRL0, EN_USB_CLKS);
}

bool imx233_clkctrl_is_usb_pll_enabled(void)
{
    return BF_RD(CLKCTRL_PLLCTRL0, EN_USB_CLKS);
}

void imx233_clkctrl_set_auto_slow_divisor(enum imx233_as_div_t div)
{
    /* the SLOW_DIV must only be set when auto-slow is disabled */
    bool old_status = imx233_clkctrl_is_auto_slow_enabled();
    imx233_clkctrl_enable_auto_slow(false);
    BF_WR(CLKCTRL_HBUS, SLOW_DIV, div);
    imx233_clkctrl_enable_auto_slow(old_status);
}

enum imx233_as_div_t imx233_clkctrl_get_auto_slow_divisor(void)
{
    return BF_RD(CLKCTRL_HBUS, SLOW_DIV);
}

void imx233_clkctrl_enable_auto_slow(bool enable)
{
    BF_WR(CLKCTRL_HBUS, AUTO_SLOW_MODE, enable);
}

bool imx233_clkctrl_is_auto_slow_enabled(void)
{
    return BF_RD(CLKCTRL_HBUS, AUTO_SLOW_MODE);
}

void imx233_clkctrl_enable_auto_slow_monitor(enum imx233_as_monitor_t monitor, bool enable)
{
    if(enable)
        HW_CLKCTRL_HBUS_SET = monitor;
    else
        HW_CLKCTRL_HBUS_CLR = monitor;
}

bool imx233_clkctrl_is_auto_slow_monitor_enabled(enum imx233_as_monitor_t monitor)
{
    return HW_CLKCTRL_HBUS & monitor;
}

bool imx233_clkctrl_is_emi_sync_enabled(void)
{
    return BF_RD(CLKCTRL_EMI, SYNC_MODE_EN);
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
                if(BF_RD(CLKCTRL_CPU, DIV_XTAL_FRAC_EN))

                    return (ref * BF_RD(CLKCTRL_CPU, DIV_XTAL)) / 32;
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
                if(BF_RD(CLKCTRL_EMI, CLKGATE))
                    return 0;
                else
                    return ref / BF_RD(CLKCTRL_EMI, DIV_XTAL);
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
