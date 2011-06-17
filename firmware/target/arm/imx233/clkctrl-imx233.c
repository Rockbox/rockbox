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

void imx233_enable_timrot_xtal_clk32k(bool enable)
{
    if(enable)
        __REG_CLR(HW_CLKCTRL_XTAL) = HW_CLKCTRL_XTAL__TIMROT_CLK32K_GATE;
    else
        __REG_SET(HW_CLKCTRL_XTAL) = HW_CLKCTRL_XTAL__TIMROT_CLK32K_GATE;
}

void imx233_enable_clock(enum imx233_clock_t clk, bool enable)
{
    volatile uint32_t *REG;
    switch(clk)
    {
        case CLK_PIX: REG = &HW_CLKCTRL_PIX; break;
        case CLK_SSP: REG = &HW_CLKCTRL_SSP; break;
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

void imx233_set_clock_divisor(enum imx233_clock_t clk, int div)
{
    switch(clk)
    {
        case CLK_PIX:
            __REG_CLR(HW_CLKCTRL_PIX) = (1 << 12) - 1;
            __REG_SET(HW_CLKCTRL_PIX) = div;
            while(HW_CLKCTRL_PIX & __CLK_BUSY);
            break;
        case CLK_SSP:
            __REG_CLR(HW_CLKCTRL_SSP) = (1 << 9) - 1;
            __REG_SET(HW_CLKCTRL_SSP) = div;
            while(HW_CLKCTRL_SSP & __CLK_BUSY);
            break;
        default: return;
    }
}

void imx233_set_fractional_divisor(enum imx233_clock_t clk, int fracdiv)
{
    /* NOTE: HW_CLKCTRL_FRAC only support byte access ! */
    volatile uint8_t *REG;
    switch(clk)
    {
        case CLK_PIX: REG = &HW_CLKCTRL_FRAC_PIX; break;
        case CLK_IO: REG = &HW_CLKCTRL_FRAC_IO; break;
        default: return;
    }

    if(fracdiv != 0)
        *REG = fracdiv;
    else
        *REG = HW_CLKCTRL_FRAC_XX__CLKGATEXX;;
}

void imx233_set_bypass_pll(enum imx233_clock_t clk, bool bypass)
{
    uint32_t msk;
    switch(clk)
    {
        case CLK_PIX: msk = HW_CLKCTRL_CLKSEQ__BYPASS_PIX; break;
        case CLK_SSP: msk = HW_CLKCTRL_CLKSEQ__BYPASS_SSP; break;
        default: return;
    }

    if(bypass)
        __REG_SET(HW_CLKCTRL_CLKSEQ) = msk;
    else
        __REG_CLR(HW_CLKCTRL_CLKSEQ) = msk;
}

