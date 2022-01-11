/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021-2022 Aidan MacDonald
 * Copyright 2014 Ingenic Semiconductor Co.,Ltd
 *      cscheng <shicheng.cheng@ingenic.com>
 * sound/soc/ingenic/icodec/icdc_d3.c
 * ALSA SoC Audio driver -- ingenic internal codec (icdc_d3) driver
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

#include "x1000-codec.h"
#include "audiohw.h"
#include "pcm_sampr.h"
#include "kernel.h"
#include "x1000/aic.h"

static const uint8_t fsel_to_hw[HW_NUM_FREQ] = {
    [0 ... HW_NUM_FREQ-1] = 0,
    HW_HAVE_8_([HW_FREQ_8] = 0,)
    HW_HAVE_11_([HW_FREQ_11] = 1,)
    HW_HAVE_12_([HW_FREQ_12] = 2,)
    HW_HAVE_16_([HW_FREQ_16] = 3,)
    HW_HAVE_22_([HW_FREQ_22] = 4,)
    HW_HAVE_24_([HW_FREQ_24] = 5,)
    HW_HAVE_32_([HW_FREQ_32] = 6,)
    HW_HAVE_44_([HW_FREQ_44] = 7,)
    HW_HAVE_48_([HW_FREQ_48] = 8,)
    HW_HAVE_88_([HW_FREQ_88] = 9,)
    HW_HAVE_96_([HW_FREQ_96] = 10,)
    HW_HAVE_176_([HW_FREQ_176] = 11,)
    HW_HAVE_192_([HW_FREQ_192] = 12,)
};

void x1000_icodec_open(void)
{
    /* Ingenic does not specify any timing constraints for reset,
     * let's do a 1ms delay for fun */
    jz_writef(AIC_RGADW, ICRST(1));
    mdelay(1);
    jz_writef(AIC_RGADW, ICRST(0));

    /* Power-up and initial config sequence */
    static const uint8_t init_config[] = {
        JZCODEC_CR_VIC,     0x03, /* ensure codec is powered off */
        JZCODEC_CR_CK,      0x40, /* MCLK_DIV=1, SHUTDOWN_CLK=0, CRYSTAL=12Mhz */
        JZCODEC_AICR_DAC,   0x13, /* ADWL=0 (16bit word length)
                                   * SLAVE=0 (i2s master mode)
                                   * SB_DAC=1 (power down DAC)
                                   * AUDIOIF=3 (i2s mode) */
        JZCODEC_AICR_ADC,   0x13, /* ADWL=0 (16bit word length)
                                   * SB_ADC=1 (power down ADC)
                                   * AUDIOIF=3 (i2s mode)
                                   */
        JZCODEC_CR_DAC,     0x91, /* DAC mute, power down */
        JZCODEC_CR_DAC2,    0x38, /* DAC power down */
        JZCODEC_CR_DMIC,    0x00, /* DMIC clock off */
        JZCODEC_CR_MIC1,    0x30, /* MIC1 power down */
        JZCODEC_CR_MIC2,    0x30, /* MIC2 power down */
        JZCODEC_CR_ADC,     0x90, /* ADC mute, power down */
        JZCODEC_ICR,        0x00, /* INT_FORM=0 (high level IRQ) */
        JZCODEC_IMR,        0xff, /* Mask all interrupts */
        JZCODEC_IMR2,       0xff,
        JZCODEC_IFR,        0xff, /* Clear all interrupt flags */
        JZCODEC_IFR2,       0xff,
    };

    for(size_t i = 0; i < ARRAYLEN(init_config); i += 2)
        x1000_icodec_write(init_config[i], init_config[i+1]);

    /* SB -> 0 (power up) */
    x1000_icodec_write(JZCODEC_CR_VIC, 0x02);
    mdelay(250);

    /* Initial gain setting. Apparently we need to set one gain and
     * then set another after 10ms; afterward it can be changed freely. */
    static const uint8_t gain_regs[] = {
        JZCODEC_GCR_DACL,
        JZCODEC_GCR_DACR,
        JZCODEC_GCR_DACL2,
        JZCODEC_GCR_DACR2,
        JZCODEC_GCR_MIC1,
        JZCODEC_GCR_MIC2,
        JZCODEC_GCR_ADCL,
        JZCODEC_GCR_ADCR,
    };

    for(size_t i = 0; i < ARRAYLEN(gain_regs); ++i)
        x1000_icodec_write(gain_regs[i], 0);

    mdelay(10);

    for(size_t i = 0; i < ARRAYLEN(gain_regs); ++i)
        x1000_icodec_write(gain_regs[i], 1);

    /* SB_SLEEP -> 0 (exit sleep/standby mode) */
    x1000_icodec_write(JZCODEC_CR_VIC, 0x00);
    mdelay(200);
}

void x1000_icodec_close(void)
{
    /* SB_SLEEP -> 1 (enable sleep mode) */
    x1000_icodec_write(JZCODEC_CR_VIC, 0x02);

    /* SB -> 1 (power down) */
    x1000_icodec_write(JZCODEC_CR_VIC, 0x03);
}

/*
 * DAC configuration
 */

void x1000_icodec_dac_frequency(int fsel)
{
    x1000_icodec_update(JZCODEC_FCR_DAC, 0x0f, fsel_to_hw[fsel]);
}

/*
 * ADC configuration
 */

void x1000_icodec_adc_enable(bool en)
{
    x1000_icodec_update(JZCODEC_AICR_ADC, 0x10, en ? 0x00 : 0x10);
    x1000_icodec_update(JZCODEC_CR_ADC, 0x10, en ? 0x00 : 0x10);
}

void x1000_icodec_adc_mute(bool muted)
{
    x1000_icodec_update(JZCODEC_CR_ADC, 0x80, muted ? 0x80 : 0x00);
}

void x1000_icodec_adc_mic_sel(int sel)
{
    x1000_icodec_update(JZCODEC_CR_ADC, 0x40,
                        sel == JZCODEC_MIC_SEL_DIGITAL ? 0x40 : 0x00);
}

void x1000_icodec_adc_frequency(int fsel)
{
    x1000_icodec_update(JZCODEC_FCR_ADC, 0x0f, fsel_to_hw[fsel]);
}

void x1000_icodec_adc_highpass_filter(bool en)
{
    x1000_icodec_update(JZCODEC_FCR_ADC, 0x40, en ? 0x40 : 0x00);
}

void x1000_icodec_adc_gain(int gain_dB)
{
    if(gain_dB < X1000_ICODEC_ADC_GAIN_MIN)
        gain_dB = X1000_ICODEC_ADC_GAIN_MIN;
    else if(gain_dB > X1000_ICODEC_ADC_GAIN_MAX)
        gain_dB = X1000_ICODEC_ADC_GAIN_MAX;

    /* bit 7 = use the same gain for both channels */
    x1000_icodec_write(JZCODEC_GCR_ADCL, 0x80 | gain_dB);
}

/*
 * MIC1 configuration
 */

void x1000_icodec_mic1_enable(bool en)
{
    x1000_icodec_update(JZCODEC_CR_MIC1, 0x10, en ? 0x00 : 0x10);
}

void x1000_icodec_mic1_bias_enable(bool en)
{
    x1000_icodec_update(JZCODEC_CR_MIC1, 0x20, en ? 0x00 : 0x20);
}

void x1000_icodec_mic1_configure(int settings)
{
    x1000_icodec_update(JZCODEC_CR_MIC1, JZCODEC_MIC1_CONFIGURE_MASK,
                        settings & JZCODEC_MIC1_CONFIGURE_MASK);
}

void x1000_icodec_mic1_gain(int gain_dB)
{
    if(gain_dB < X1000_ICODEC_MIC_GAIN_MIN)
        gain_dB = X1000_ICODEC_MIC_GAIN_MIN;
    else if(gain_dB > X1000_ICODEC_MIC_GAIN_MAX)
        gain_dB = X1000_ICODEC_MIC_GAIN_MAX;

    x1000_icodec_write(JZCODEC_GCR_MIC1, gain_dB/X1000_ICODEC_MIC_GAIN_STEP);
}

/*
 * Mixer configuration
 */

void x1000_icodec_mixer_enable(bool en)
{
    x1000_icodec_update(JZCODEC_CR_MIX, 0x80, en ? 0x80 : 0x00);
}

/*
 * Register access
 */

static int x1000_icodec_read_direct(int reg)
{
    jz_writef(AIC_RGADW, ADDR(reg));
    return jz_readf(AIC_RGDATA, DATA);
}

static void x1000_icodec_write_direct(int reg, int value)
{
    jz_writef(AIC_RGADW, ADDR(reg), DATA(value));
    jz_writef(AIC_RGADW, RGWR(1));
    while(jz_readf(AIC_RGADW, RGWR));
}

static void x1000_icodec_update_direct(int reg, int mask, int value)
{
    int x = x1000_icodec_read_direct(reg) & ~mask;
    x |= value;
    x1000_icodec_write_direct(reg, x);
}

static int x1000_icodec_read_indirect(int c_reg, int index)
{
    x1000_icodec_update_direct(c_reg, 0x7f, index & 0x3f);
    return x1000_icodec_read_direct(c_reg+1);
}

static void x1000_icodec_write_indirect(int c_reg, int index, int value)
{
    /* NB: The X1000 programming manual says we should write the data
     * register first, but in fact the control register needs to be
     * written first (following Ingenic's Linux driver). */
    x1000_icodec_update_direct(c_reg, 0x7f, 0x40 | (index & 0x3f));
    x1000_icodec_write_direct(c_reg+1, value);
}

static void x1000_icodec_update_indirect(int c_reg, int index, int mask, int value)
{
    int x = x1000_icodec_read_indirect(c_reg, index) & ~mask;
    x |= value;
    x1000_icodec_write_indirect(c_reg, index, x);
}

int x1000_icodec_read(int reg)
{
    if(reg & JZCODEC_INDIRECT_BIT)
        return x1000_icodec_read_indirect(JZCODEC_INDIRECT_CREG(reg),
                                          JZCODEC_INDIRECT_INDEX(reg));
    else
        return x1000_icodec_read_direct(reg);
}

void x1000_icodec_write(int reg, int value)
{
    if(reg & JZCODEC_INDIRECT_BIT)
        return x1000_icodec_write_indirect(JZCODEC_INDIRECT_CREG(reg),
                                           JZCODEC_INDIRECT_INDEX(reg), value);
    else
        return x1000_icodec_write_direct(reg, value);
}

void x1000_icodec_update(int reg, int mask, int value)
{
    if(reg & JZCODEC_INDIRECT_BIT)
        return x1000_icodec_update_indirect(JZCODEC_INDIRECT_CREG(reg),
                                            JZCODEC_INDIRECT_INDEX(reg),
                                            mask, value);
    else
        return x1000_icodec_update_direct(reg, mask, value);
}
