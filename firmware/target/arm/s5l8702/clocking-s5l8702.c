/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:
 *
 * Copyright (C) 2015 by Cástor Muñoz
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
#include <stdbool.h>

#include "config.h"
#include "system.h"  /* udelay() */
#include "s5l8702.h"
#include "clocking-s5l8702.h"

/* returns configured frequency (PLLxFreq, when locked) */
unsigned pll_get_cfg_freq(int pll)
{
    unsigned pdiv, mdiv, sdiv, f_in;
    uint32_t pllpms;

    pllpms = PLLPMS(pll);

    pdiv = (pllpms >> PLLPMS_PDIV_POS) & PLLPMS_PDIV_MSK;
    if (pdiv == 0) return 0;
    mdiv = (pllpms >> PLLPMS_MDIV_POS) & PLLPMS_MDIV_MSK;
    sdiv = (pllpms >> PLLPMS_SDIV_POS) & PLLPMS_SDIV_MSK;

    /* experimental results sugests that the HW is working this way */
    if (mdiv < 2) mdiv += 256;

    if (GET_PMSMOD(pll) == PMSMOD_DIV)
    {
        f_in = (GET_DMOSC(pll))
                    ? ((PLLMOD2 & PLLMOD2_ALTOSC_BIT(pll))
                                            ? S5L8702_ALTOSC1_HZ
                                            : S5L8702_ALTOSC0_HZ)
                    : S5L8702_OSC0_HZ;

        return (f_in * mdiv / pdiv) >> sdiv; /* divide */
    }
    else
    {
        /* XXX: overflows for high f_in, safe for 32768 Hz */
        f_in = S5L8702_OSC1_HZ;
        return (f_in * mdiv * pdiv) >> sdiv; /* multiply */
    }
}

/* returns PLLxClk */
unsigned pll_get_out_freq(int pll)
{
    uint32_t pllmode = PLLMODE;

    if ((pllmode & PLLMODE_PLLOUT_BIT(pll)) == 0)
        return S5L8702_OSC1_HZ; /* slow mode */

    if ((pllmode & PLLMODE_EN_BIT(pll)) == 0)
        return 0;

    return pll_get_cfg_freq(pll);
}

/* returns selected oscillator for CG16_SEL_OSC source */
unsigned soc_get_oscsel_freq(void)
{
    return (PLLMODE & PLLMODE_OSCSEL_BIT) ? S5L8702_OSC1_HZ
                                          : S5L8702_OSC0_HZ;
}

/* returns output frequency */
unsigned cg16_get_freq(volatile uint16_t* cg16)
{
    unsigned sel, freq;
    uint16_t val16 = *cg16;

    if (val16 & CG16_DISABLE_BIT)
        return 0;

    sel = (val16 >> CG16_SEL_POS) & CG16_SEL_MSK;

    if (val16 & CG16_UNKOSC_BIT)
        freq = S5L8702_UNKOSC_HZ;
    else if (sel == CG16_SEL_OSC)
        freq = soc_get_oscsel_freq();
    else
        freq = pll_get_out_freq(--sel);

    freq /= (((val16 >> CG16_DIV1_POS) & CG16_DIV1_MSK) + 1) *
                (((val16 >> CG16_DIV2_POS) & CG16_DIV2_MSK) + 1);

    return freq;
}

void soc_set_system_divs(unsigned cdiv, unsigned hdiv, unsigned hprat)
{
    uint32_t val = 0;
    unsigned pdiv = hdiv * hprat;

    if (cdiv > 1)
        val |= CLKCON1_CDIV_EN_BIT |
                ((((cdiv >> 1) - 1) & CLKCON1_CDIV_MSK) << CLKCON1_CDIV_POS);
    if (hdiv > 1)
        val |= CLKCON1_HDIV_EN_BIT |
                ((((hdiv >> 1) - 1) & CLKCON1_HDIV_MSK) << CLKCON1_HDIV_POS);
    if (pdiv > 1)
        val |= CLKCON1_PDIV_EN_BIT |
                ((((pdiv >> 1) - 1) & CLKCON1_PDIV_MSK) << CLKCON1_PDIV_POS);

    val |= ((hprat - 1) & CLKCON1_HPRAT_MSK) << CLKCON1_HPRAT_POS;

    CLKCON1 = val;

    while ((CLKCON1 >> 8) != (val >> 8));
}

unsigned soc_get_system_divs(unsigned *cdiv, unsigned *hdiv, unsigned *pdiv)
{
    uint32_t val = CLKCON1;

    if (cdiv)
        *cdiv = !(val & CLKCON1_CDIV_EN_BIT) ? 1 :
                (((val >> CLKCON1_CDIV_POS) & CLKCON1_CDIV_MSK) + 1) << 1;
    if (hdiv)
        *hdiv = !(val & CLKCON1_HDIV_EN_BIT) ? 1 :
                (((val >> CLKCON1_HDIV_POS) & CLKCON1_HDIV_MSK) + 1) << 1;
    if (pdiv)
        *pdiv = !(val & CLKCON1_PDIV_EN_BIT) ? 1 :
                (((val >> CLKCON1_PDIV_POS) & CLKCON1_PDIV_MSK) + 1) << 1;

    return cg16_get_freq(&CG16_SYS);  /* FClk */
}

unsigned get_system_freqs(unsigned *cclk, unsigned *hclk, unsigned *pclk)
{
    unsigned fclk, cdiv, hdiv, pdiv;

    fclk = soc_get_system_divs(&cdiv, &hdiv, &pdiv);

    if (cclk) *cclk = fclk / cdiv;
    if (hclk) *hclk = fclk / hdiv;
    if (pclk) *pclk = fclk / pdiv;

    return fclk;
}

void soc_set_hsdiv(int hsdiv)
{
    SM1_DIV = hsdiv - 1;
}

int soc_get_hsdiv(void)
{
    return (SM1_DIV & 0xf) + 1;
}

/* each target/app could define its own clk_modes table */
struct clocking_mode *clk_modes;
int cur_level = -1;

void clocking_init(struct clocking_mode *modes, int level)
{
    /* at this point, CK16_SYS should be already configured
       and enabled by emCORE/bootloader */

    /* initialize global clocking */
    clk_modes = modes;
    cur_level = level;

    /* start initial level */
    struct clocking_mode *m = clk_modes + cur_level;
    soc_set_hsdiv(m->hsdiv);
    soc_set_system_divs(m->cdiv, m->hdiv, m->hprat);
}

void set_clocking_level(int level)
{
    struct clocking_mode *cur, *next;

    int step = (level < cur_level) ? -1 : 1;

    while (cur_level != level)
    {
        cur = clk_modes + cur_level;
        next = cur + step;

        /* step-up */
        if (next->hsdiv > cur->hsdiv)
            soc_set_hsdiv(next->hsdiv);

        /* step up/down */
        soc_set_system_divs(next->cdiv, next->hdiv, next->hprat);

        /* step-down */
        if (next->hsdiv < cur->hsdiv)
            soc_set_hsdiv(next->hsdiv);

        cur_level += step;
    }

    udelay(50); /* TBC: probably not needed */
}

void clockgate_enable(int gate, bool enable)
{
    int i = (gate >> 5) & 1;
    uint32_t bit = 1 << (gate & 0x1f);
    if (enable) PWRCON(i) &= ~bit;
    else PWRCON(i) |= bit;
}

#ifdef BOOTLOADER
int pll_config(int pll, int op_mode, int p, int m, int s, int lock_time)
{
    PLLPMS(pll) = ((p & PLLPMS_PDIV_MSK) << PLLPMS_PDIV_POS)
                | ((m & PLLPMS_MDIV_MSK) << PLLPMS_MDIV_POS)
                | ((s & PLLPMS_SDIV_MSK) << PLLPMS_SDIV_POS);

    /* lock_time are PClk ticks */
    PLLCNT(pll) = lock_time & PLLCNT_MSK;

    if (pll < 2)
    {
        if (op_mode == PLLOP_MM) {
            PLLMODE &= ~PLLMODE_PMSMOD_BIT(pll);  /* MM */
            return 0;
        }

        PLLMODE |= PLLMODE_PMSMOD_BIT(pll);

        if (op_mode == PLLOP_DM) {
            PLLMOD2 &= ~PLLMOD2_DMOSC_BIT(pll);   /* DM */
            return 0;
        }

        PLLMOD2 |= PLLMOD2_DMOSC_BIT(pll);
    }
    else
    {
        if (op_mode == PLLOP_MM)
            return -1;  /* PLL2 does not support MM */

        if (op_mode == PLLOP_DM) {
            PLLMODE &= ~PLLMODE_PLL2DMOSC_BIT;    /* DM */
            return 0;
        }

        PLLMODE |= PLLMODE_PLL2DMOSC_BIT;
    }

    /* ALTOSCx */
    PLLMOD2 = (PLLMOD2 & ~PLLMOD2_ALTOSC_BIT(pll)) |
                ((op_mode == PLLOP_ALT0) ? 0 : PLLMOD2_ALTOSC_BIT(pll));

    return 0;
}

int pll_onoff(int pll, bool onoff)
{
    if (onoff)
    {
        PLLMODE |= PLLMODE_EN_BIT(pll); /* start PLL */
        while (!(PLLLOCK & PLLLOCK_LCK_BIT(pll))); /* locking... */
        PLLMODE |= PLLMODE_PLLOUT_BIT(pll); /* slow mode OFF */

        /* returns DMLCK status, only meaningful for Divisor Mode (DM) */
        return (PLLLOCK & PLLLOCK_DMLCK_BIT(pll)) ? 1 : 0;
    }
    else
    {
        PLLMODE &= ~PLLMODE_PLLOUT_BIT(pll); /* slow mode ON */
        udelay(50); /* TBC: needed when current F_in is 0 Hz */
        PLLMODE &= ~PLLMODE_EN_BIT(pll); /* stop PLL */

        return 0;
    }
}

/* configure and enable/disable 16-bit clockgate */
void cg16_config(volatile uint16_t* cg16,
                    bool onoff, int clksel, int div1, int div2)
{
    uint16_t val16 = ((clksel & CG16_SEL_MSK) << CG16_SEL_POS)
                   | (((div1 - 1) & CG16_DIV1_MSK) << CG16_DIV1_POS)
                   | (((div2 - 1) & CG16_DIV2_MSK) << CG16_DIV2_POS)
                   | (onoff ? 0 : CG16_DISABLE_BIT);

    volatile uint32_t* reg32 = (uint32_t *)((int)cg16 & ~3);
    int shift = ((int)cg16 & 2) << 3;

    *reg32 = (*reg32 & (0xffff0000 >> shift)) | (val16 << shift);

    /*udelay(100);*/  /* probably not needed */

    while (*cg16 != val16);
}

/* Configures EClk for USEC_TIMER. DRAM refresh also depends on EClk,
 * this clock should be initialized by the bootloader, so USEC_TIMER
 * is ready to use for RB.
 */
void usec_timer_init(void)
{
    /* select OSC0 for CG16 SEL_OSC */
    PLLMODE &= ~PLLMODE_OSCSEL_BIT;

    /* configure and enable ECLK */
    cg16_config(&CG16_RTIME, true, CG16_SEL_OSC, 1, 1);

    /* unmask timer controller clock gate */
    clockgate_enable(CLOCKGATE_TIMER, true);

    /* configure and start timer E */
    TECON = (4 << 8) |      /* TE_CS = ECLK / 1 */
            (1 << 6) |      /* select ECLK (12 MHz on Classic) */
            (0 << 4);       /* TE_MODE_SEL = interval mode */
    TEPRE = (S5L8702_OSC0_HZ / 1000000) - 1;  /* prescaler */
    TEDATA0 = ~0;
    TECMD = (1 << 1) |      /* TE_CLR = initialize timer */
            (1 << 0);       /* TE_EN = enable */
}

#if 0
/* - This function is mainly to documment how s5l8702 ROMBOOT and iPod
 *   Classic diagnostic OF detects primary external clock.
 * - ATM it is unknown if 24 MHz are used on other targets (i.e. Nano 3G),
 *   other SoC (ROMBOOT identifies itself as s5l8900/s5l8702), a Classic
 *   prototype, or (probably) never used...
 * - This function should be called only at boot time, GPIO3.5 is also
 *   used for ATA controller.
 */
unsigned soc_get_osc0(void)
{
    GPIOCMD = 0x30500;  /* configure GPIO3.5 as input */
    return (PDAT3 & 0x20) ? 24000000 : 12000000;
}
#endif

#endif  /* BOOTLOADER */
