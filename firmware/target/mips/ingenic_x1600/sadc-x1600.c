/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

/* SADC repeat sampling. adc_read() reads the latest conversion without
 * waiting, making it usable from the button tick. */

#include "system.h"
#include "kernel.h"
#include "adc.h"
#include "sadc-x1600.h"
#include "clk-x1600.h"
#include "x1600/sadc.h"
#include "x1600/cpm.h"
#include "adc-target.h"

/* Use PCLK as the inferred SADC source. The PM does not specify its source
 * or maximum ADC clock. */
#define SADC_TARGET_ADC_CLK  6000000
#define SADC_TARGET_US_CLK   1000000

/* At least 2 ms must pass between clearing POWER and enabling the SADC */
#define SADC_POWERUP_DELAY_MS  3

/* Allow 100 clocks for the key-ladder input to settle. */
#define SADC_STABLE_CLOCKS   100
#define SADC_PD_DELAY_CLOCKS 3

/* Repeat interval: a fresh sample per button tick, without spinning flat out */
#define SADC_REPEAT_MS       5

/* AUX3 is excluded: nothing in the vendor firmware references channel 3 */
#define SADC_ENABLED_CHANNELS \
    ((1u << ADC_BUTTONS) | (1u << ADC_HP_DETECT) | (1u << ADC_REMOTE))

/* Round to even: an odd CLKDIV divides by CLKDIV-1 */
static uint32_t sadc_even_div(uint32_t in, uint32_t out)
{
    if(out == 0)
        return 2;

    uint32_t d = (in + out - 1) / out;
    if(d < 2)
        d = 2;
    if(d & 1)
        d += 1;
    if(d > 254)
        d = 254;
    return d;
}

void adc_init(void)
{
    /* Ungate */
    jz_writef(CPM_CLKGR, SADC(0));

    /* Out of soft reset; a warm boot may not be. ADC_TRIM stays 0. */
    jz_overwritef(SADC_CFG, ADC_RST(0));
    udelay(10);
    jz_overwritef(SADC_CFG, ADC_RST(1));

    /* Mask every data-ready interrupt -- we poll. A warm boot may not have. */
    jz_write(SADC_CTRL, BM_SADC_CTRL_ARDYM0 | BM_SADC_CTRL_ARDYM1 |
                        BM_SADC_CTRL_ARDYM2 | BM_SADC_CTRL_ARDYM3);

    /* Clock dividers; see the note above on the device clock */
    uint32_t dev_clk = clk_get(X1600_CLK_PCLK);
    if(dev_clk == 0) {
        /* Powered down rather than a divider of 0, which bypasses the clock */
        return;
    }

    uint32_t clkdiv = sadc_even_div(dev_clk, SADC_TARGET_ADC_CLK);
    uint32_t adc_clk = dev_clk / clkdiv;
    uint32_t usdiv = sadc_even_div(adc_clk, SADC_TARGET_US_CLK);
    uint32_t us_clk = adc_clk / usdiv;
    /* ms_clk = us_clk / (CLKDIV_MS + 1); aim for 1 kHz */
    uint32_t msdiv = us_clk / 1000;
    if(msdiv < 1)
        msdiv = 1;

    jz_overwritef(SADC_CLK, CLKDIV_MS(msdiv - 1), CLKDIV_US(usdiv),
                            CLKDIV(clkdiv));

    /* Settling delays, counted in ADC clocks */
    jz_overwritef(SADC_STB, PD_DELAY(SADC_PD_DELAY_CLOCKS),
                            STABLE(SADC_STABLE_CLOCKS));

    /* Repeat interval, counted in ms clocks */
    jz_write(SADC_RETM, SADC_REPEAT_MS);

    /* Clear POWER, wait, then enable the channels and repeat sampling */
    jz_writef(SADC_ENA, POWER(0));
    mdelay(SADC_POWERUP_DELAY_MS);

    /* POW_OPT is left disabled: the PM does not say what it trades away */
    jz_writef(SADC_ENA, REPTEN(1));

    for(int ch = 0; ch < NUM_ADC_CHANNELS; ++ch)
        adc_enable_channel(ch, (SADC_ENABLED_CHANNELS >> ch) & 1);
}

void adc_enable_channel(int channel, bool enable)
{
    if(channel < 0 || channel >= NUM_ADC_CHANNELS)
        return;

    /* AUX0EN..AUX3EN are ADENA bits 0..3 */
    if(enable)
        REG_SADC_ENA |= (1u << channel);
    else
        REG_SADC_ENA &= ~(1u << channel);
}

unsigned short adc_read(int channel)
{
    uint32_t data;

    /* ADATA0 holds AUX1 in [27:16] and AUX0 in [11:0]; ADATA1 holds AUX3 in
     * [27:16] and AUX2 in [11:0] */
    switch(channel) {
    case 0: data = jz_readf(SADC_DATA0, AUX0); break;
    case 1: data = jz_readf(SADC_DATA0, AUX1); break;
    case 2: data = jz_readf(SADC_DATA1, AUX2); break;
    case 3: data = jz_readf(SADC_DATA1, AUX3); break;
    default: return 0;
    }

    /* ARDYn is write-1-to-clear: write one bit, not read-modify-write */
    jz_write(SADC_STATE, 1u << channel);

    return (unsigned short)data;
}

/* adc_read_mv() is the device layer's; power-hibyr1.c defines it */
