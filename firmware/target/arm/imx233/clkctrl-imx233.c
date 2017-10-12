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
#include "string.h"
#include "debug.h"

void imx233_clkctrl_enable(enum imx233_clock_t clk, bool enable)
{
    /* NOTE some registers like HW_CLKCTRL_PIX don't have a CLR/SET variant ! */
    bool gate = !enable;
    switch(clk)
    {
#if IMX233_SUBTARGET >= 3700
        case CLK_PIX: BF_WR(CLKCTRL_PIX, CLKGATE(gate)); break;
#endif
        case CLK_SSP: BF_WR(CLKCTRL_SSP, CLKGATE(gate)); break;
        case CLK_DRI: BF_WR(CLKCTRL_XTAL, DRI_CLK24M_GATE(gate)); break;
        case CLK_PWM: BF_WR(CLKCTRL_XTAL, PWM_CLK24M_GATE(gate)); break;
        case CLK_UART: BF_WR(CLKCTRL_XTAL, UART_CLK_GATE(gate)); break;
        case CLK_FILT: BF_WR(CLKCTRL_XTAL, FILT_CLK24M_GATE(gate)); break;
        case CLK_TIMROT: BF_WR(CLKCTRL_XTAL, TIMROT_CLK32K_GATE(gate)); break;
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
#undef handle_std
#undef handle_xtal
}

bool imx233_clkctrl_is_enabled(enum imx233_clock_t clk)
{
    switch(clk)
    {
        case CLK_PLL: return BF_RD(CLKCTRL_PLLCTRL0, POWER);
#if IMX233_SUBTARGET >= 3700
        case CLK_PIX: return !BF_RD(CLKCTRL_PIX, CLKGATE);
#endif
        case CLK_SSP: return !BF_RD(CLKCTRL_SSP, CLKGATE);
        case CLK_DRI: return !BF_RD(CLKCTRL_XTAL, DRI_CLK24M_GATE);
        case CLK_PWM: return !BF_RD(CLKCTRL_XTAL, PWM_CLK24M_GATE);
        case CLK_UART: return !BF_RD(CLKCTRL_XTAL, UART_CLK_GATE);
        case CLK_FILT: return !BF_RD(CLKCTRL_XTAL, FILT_CLK24M_GATE);
        case CLK_TIMROT: return !BF_RD(CLKCTRL_XTAL, TIMROT_CLK32K_GATE);
        default: return true;
    }
}

void imx233_clkctrl_set_div(enum imx233_clock_t clk, int div)
{
    /* warning: some registers like HW_CLKCTRL_PIX don't have a CLR/SET variant !
     * assume that we always derive emi and cpu from ref_XX */
    switch(clk)
    {
#if IMX233_SUBTARGET >= 3700
        case CLK_PIX: BF_WR(CLKCTRL_PIX, DIV(div)); break;
        case CLK_CPU: BF_WR(CLKCTRL_CPU, DIV_CPU(div)); break;
        case CLK_EMI: BF_WR(CLKCTRL_EMI, DIV_EMI(div)); break;
#else
        case CLK_CPU: BF_WR(CLKCTRL_CPU, DIV(div)); break;
        case CLK_EMI: BF_WR(CLKCTRL_EMI, DIV(div)); break;
#endif
        case CLK_SSP: BF_WR(CLKCTRL_SSP, DIV(div)); break;
        case CLK_HBUS:
            /* make sure to switch to integer divide mode simulteanously */
            BF_WR(CLKCTRL_HBUS, DIV_FRAC_EN(0), DIV(div)); break;
        case CLK_XBUS: BF_WR(CLKCTRL_XBUS, DIV(div)); break;
        default: return;
    }
}

int imx233_clkctrl_get_div(enum imx233_clock_t clk)
{
    /* assume that we always derive emi and cpu from ref_XX */
    switch(clk)
    {
#if IMX233_SUBTARGET >= 3700
        case CLK_PIX: return BF_RD(CLKCTRL_PIX, DIV);
        case CLK_CPU: return BF_RD(CLKCTRL_CPU, DIV_CPU);
        case CLK_EMI: return BF_RD(CLKCTRL_EMI, DIV_EMI);
#else
        case CLK_CPU: return BF_RD(CLKCTRL_CPU, DIV);
        case CLK_EMI: return BF_RD(CLKCTRL_EMI, DIV);
#endif
        case CLK_SSP: return BF_RD(CLKCTRL_SSP, DIV);
        case CLK_HBUS:
            /* since fractional and integer divider share the same field, clain it is disabled in frac mode */
            if(BF_RD(CLKCTRL_HBUS, DIV_FRAC_EN))
                return 0;
            else
                return BF_RD(CLKCTRL_HBUS, DIV);
        case CLK_XBUS: return BF_RD(CLKCTRL_XBUS, DIV);
        default: return 0;
    }
}

#if IMX233_SUBTARGET >= 3700
void imx233_clkctrl_set_frac_div(enum imx233_clock_t clk, int fracdiv)
{
#define handle_frac(dev) \
    case CLK_##dev: \
        if(fracdiv == 0) \
            BF_SET(CLKCTRL_FRAC, CLKGATE##dev); \
        else { \
            BF_WR(CLKCTRL_FRAC, dev##FRAC(fracdiv)); \
            BF_CLR(CLKCTRL_FRAC, CLKGATE##dev); } \
        break;
    switch(clk)
    {
        handle_frac(PIX)
        handle_frac(IO)
        handle_frac(CPU)
        handle_frac(EMI)
        case CLK_HBUS:
            if(fracdiv == 0)
                panicf("Don't set hbus fracdiv to 0!");
            /* value 0 is forbidden because we can't simply disabble the divider, it's always
             * active but either in integer or fractional mode
             * make sure we write both the value and frac_en bit at the same time */
            BF_WR(CLKCTRL_HBUS, DIV_FRAC_EN(1), DIV(fracdiv));
            break;
        default: break;
    }
#undef handle_frac
}

int imx233_clkctrl_get_frac_div(enum imx233_clock_t clk)
{
#define handle_frac(dev) \
    case CLK_##dev:\
        if(BF_RD(CLKCTRL_FRAC, CLKGATE##dev)) \
            return 0; \
        else \
            return BF_RD(CLKCTRL_FRAC, dev##FRAC);
    switch(clk)
    {
#if IMX233_SUBTARGET >= 3700
        handle_frac(PIX)
#endif
        handle_frac(IO)
        handle_frac(CPU)
        handle_frac(EMI)
        case CLK_HBUS:
            if(BF_RD(CLKCTRL_HBUS, DIV_FRAC_EN))
                return BF_RD(CLKCTRL_HBUS, DIV);
            else
                return 0;
        default: return 0;
    }
#undef handle_frac
}

void imx233_clkctrl_set_bypass(enum imx233_clock_t clk, bool bypass)
{
    uint32_t msk;
    switch(clk)
    {
#if IMX233_SUBTARGET >= 3700
        case CLK_PIX: msk = BM_CLKCTRL_CLKSEQ_BYPASS_PIX; break;
#endif
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

bool imx233_clkctrl_get_bypass(enum imx233_clock_t clk)
{
    switch(clk)
    {
#if IMX233_SUBTARGET >= 3700
        case CLK_PIX: return BF_RD(CLKCTRL_CLKSEQ, BYPASS_PIX);
#endif
        case CLK_SSP: return BF_RD(CLKCTRL_CLKSEQ, BYPASS_SSP);
        case CLK_CPU: return BF_RD(CLKCTRL_CLKSEQ, BYPASS_CPU);
        case CLK_EMI: return BF_RD(CLKCTRL_CLKSEQ, BYPASS_EMI);
        default: return false;
    }
}

void imx233_clkctrl_set_cpu_hbus_div(int cpu_idiv, int cpu_fdiv, int hbus_div)
{
    /* disable interrupts to avoid an IRQ being triggered at the point
     * where we are slow/weird speeds, that would result in massive slow-down... */
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);
    /* we need to be very careful here: putting the wrong dividers could blow-up the
     * frequency and result in crash, also the cpu could be running from XTAL or
     * PLL at this point */
    int old_cpu_fdiv = imx233_clkctrl_get_frac_div(CLK_CPU);
    int old_hbus_div = imx233_clkctrl_get_div(CLK_HBUS);
    /* since HBUS is tied to cpu, we first ensure that the HBUS is safe to handle
     * both old and new speed: take maximum of old and new dividers */
    if(hbus_div > old_hbus_div)
        imx233_clkctrl_set_div(CLK_HBUS, hbus_div);
    /* we are about to change cpu speed: we first ensure that the fractional
     * divider is safe to handle both old and new integer divided frequency: take max */
    if(cpu_fdiv > old_cpu_fdiv)
        imx233_clkctrl_set_frac_div(CLK_CPU, cpu_fdiv);
    /* we are safe for major divider change */
    imx233_clkctrl_set_div(CLK_CPU, cpu_idiv);
    /* if the final fractional divider is lower than previous one, it's time to switch */
    if(cpu_fdiv < old_cpu_fdiv)
        imx233_clkctrl_set_frac_div(CLK_CPU, cpu_fdiv);
    /* if we were running from XTAL, switch to PLL */
    imx233_clkctrl_set_bypass(CLK_CPU, false);
    /* finally restore HBUS to its proper value */
    if(hbus_div < old_hbus_div)
        imx233_clkctrl_set_div(CLK_HBUS, hbus_div);
    /* we are free again */
    restore_interrupt(oldstatus);
}
#endif

void imx233_clkctrl_enable_usb(bool enable)
{
    if(enable)
        BF_SET(CLKCTRL_PLLCTRL0, EN_USB_CLKS);
    else
        BF_CLR(CLKCTRL_PLLCTRL0, EN_USB_CLKS);
}

bool imx233_clkctrl_is_usb_enabled(void)
{
    return BF_RD(CLKCTRL_PLLCTRL0, EN_USB_CLKS);
}

void imx233_clkctrl_set_auto_slow_div(unsigned div)
{
    /* the SLOW_DIV must only be set when auto-slow is disabled */
    bool old_status = imx233_clkctrl_is_auto_slow_enabled();
    imx233_clkctrl_enable_auto_slow(false);
    BF_WR(CLKCTRL_HBUS, SLOW_DIV(div));
    imx233_clkctrl_enable_auto_slow(old_status);
}

unsigned imx233_clkctrl_get_auto_slow_div(void)
{
    return BF_RD(CLKCTRL_HBUS, SLOW_DIV);
}

void imx233_clkctrl_enable_auto_slow(bool enable)
{
    /* NOTE: don't use SET/CLR because it doesn't exist on stmp3600 */
    BF_WR(CLKCTRL_HBUS, AUTO_SLOW_MODE(enable));
}

bool imx233_clkctrl_is_auto_slow_enabled(void)
{
    return BF_RD(CLKCTRL_HBUS, AUTO_SLOW_MODE);
}

unsigned imx233_clkctrl_get_freq(enum imx233_clock_t clk)
{
    switch(clk)
    {
        case CLK_PLL: /* PLL: 480MHz when enable */
            return imx233_clkctrl_is_enabled(CLK_PLL) ? 480000 : 0;
        case CLK_XTAL: /* crystal: 24MHz */
            return 24000;
        case CLK_CPU:
        {
#if IMX233_SUBTARGET >= 3700
            unsigned ref;
            /* In bypass mode: clk_p derived from clk_xtal via int/binfrac divider
             * otherwise, clk_p derived from clk_cpu via int div and clk_cpu
             * derived from clk_pll fracdiv */
            if(imx233_clkctrl_get_bypass(CLK_CPU))
            {
                ref = imx233_clkctrl_get_freq(CLK_XTAL);
                /* Integer divide mode vs fractional divide mode */
                if(BF_RD(CLKCTRL_CPU, DIV_XTAL_FRAC_EN))

                    return (ref * BF_RD(CLKCTRL_CPU, DIV_XTAL)) / 32;
                else
                    return ref / imx233_clkctrl_get_div(CLK_CPU);
            }
            else
            {
                ref = imx233_clkctrl_get_freq(CLK_PLL);
                /* fractional divider enable ? */
                if(imx233_clkctrl_get_frac_div(CLK_CPU) != 0)
                    ref = (ref * 18) / imx233_clkctrl_get_frac_div(CLK_CPU);
                return ref / imx233_clkctrl_get_div(CLK_CPU);
            }
#else
            return imx233_clkctrl_get_freq(CLK_PLL) / imx233_clkctrl_get_div(CLK_CPU);
#endif
        }
        case CLK_HBUS:
        {
            /* Derived from clk_p via integer/fractional div */
            unsigned ref = imx233_clkctrl_get_freq(CLK_CPU);
#if IMX233_SUBTARGET >= 3700
            /* if divider is in fractional mode, integer divider does not take effect (in fact it's
             * the same divider but in a different mode ). Also the fractiona value is encoded as
             * a fraction, not a divider */
            if(imx233_clkctrl_get_frac_div(CLK_HBUS) != 0)
                ref = (ref * imx233_clkctrl_get_frac_div(CLK_HBUS)) / 32;
            else
#endif
            if(imx233_clkctrl_get_div(CLK_HBUS) != 0)
                ref /= imx233_clkctrl_get_div(CLK_HBUS);
            return ref;
        }
        case CLK_IO:
        {
            /* Derived from clk_pll via fracdiv */
            unsigned ref = imx233_clkctrl_get_freq(CLK_PLL);
#if IMX233_SUBTARGET >= 3700
            if(imx233_clkctrl_get_frac_div(CLK_IO) != 0)
                ref = (ref * 18) / imx233_clkctrl_get_frac_div(CLK_IO);
#endif
            return ref;
        }
#if IMX233_SUBTARGET >= 3700
        case CLK_PIX:
        {
            unsigned ref;
            /* Derived from clk_pll or clk_xtal */
            if(!imx233_clkctrl_is_enabled(CLK_PIX))
                ref = 0;
            else if(imx233_clkctrl_get_bypass(CLK_PIX))
                ref = imx233_clkctrl_get_freq(CLK_XTAL);
            else
            {
                ref = imx233_clkctrl_get_freq(CLK_PLL);
                if(imx233_clkctrl_get_frac_div(CLK_PIX) != 0)
                    ref = (ref * 18) / imx233_clkctrl_get_frac_div(CLK_PIX);
            }
            return ref / imx233_clkctrl_get_div(CLK_PIX);
        }
#endif
        case CLK_SSP:
        {
            unsigned ref;
            /* Derived from clk_pll or clk_xtal */
            if(!imx233_clkctrl_is_enabled(CLK_SSP))
                ref = 0;
#if IMX233_SUBTARGET >= 3700
            else if(imx233_clkctrl_get_bypass(CLK_SSP))
                ref = imx233_clkctrl_get_freq(CLK_XTAL);
            else
#endif
                ref = imx233_clkctrl_get_freq(CLK_IO);
            return ref / imx233_clkctrl_get_div(CLK_SSP);
        }
        case CLK_EMI:
        {
#if IMX233_SUBTARGET >= 3700
            unsigned ref;
            /* Derived from clk_pll or clk_xtal */
            if(imx233_clkctrl_get_bypass(CLK_EMI))
            {
                ref = imx233_clkctrl_get_freq(CLK_XTAL);
                if(BF_RD(CLKCTRL_EMI, CLKGATE))
                    return 0;
                else
                    return ref / BF_RD(CLKCTRL_EMI, DIV_XTAL);
            }
            else
            {
                ref = imx233_clkctrl_get_freq(CLK_PLL);
                if(imx233_clkctrl_get_frac_div(CLK_EMI) != 0)
                    ref = (ref * 18) / imx233_clkctrl_get_frac_div(CLK_EMI);
                return ref / imx233_clkctrl_get_div(CLK_EMI);
            }
#else
            return imx233_clkctrl_get_freq(CLK_PLL) / imx233_clkctrl_get_div(CLK_EMI);
#endif
        }
        case CLK_XBUS:
            return imx233_clkctrl_get_freq(CLK_XTAL) / imx233_clkctrl_get_div(CLK_XBUS);
        default:
            return 0;
    }
}

void imx233_clkctrl_init(void)
{
    /* set auto-slow monitor to all */
#if IMX233_SUBTARGET >= 3700
    BF_SET(CLKCTRL_HBUS, APBHDMA_AS_ENABLE, TRAFFIC_JAM_AS_ENABLE, TRAFFIC_AS_ENABLE,
        APBXDMA_AS_ENABLE, CPU_INSTR_AS_ENABLE, CPU_DATA_AS_ENABLE);
#else
    BF_WR(CLKCTRL_HBUS, EMI_BUSY_FAST(1), APBHDMA_BUSY_FAST(1), APBXDMA_BUSY_FAST(1),
        TRAFFIC_JAM_FAST(1), TRAFFIC_FAST(1), CPU_DATA_FAST(1), CPU_INSTR_FAST(1));
#endif
#if IMX233_SUBTARGET >= 3780
    BF_SET(CLKCTRL_HBUS, DCP_AS_ENABLE, PXP_AS_ENABLE);
#endif
}
