/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for internal Rockchip rk27xx audio codec
 * (shCODlp-100.01-HD IP core from Dolphin)
 *
 * Copyright (c) 2011 Marcin Bukat
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
#include "kernel.h"
#include "audio.h"
#include "audiohw.h"
#include "system.h"
#include "i2c-rk27xx.h"

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB", 1,  5,-335,  45,-255},
    /* HAVE_SW_TONE_CONTROLS */
    [SOUND_BASS]          = {"dB", 0,  1, -24,  24,   0},
    [SOUND_TREBLE]        = {"dB", 0,  1, -24,  24,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
#ifdef HAVE_RECORDING /* disabled for now */
    [SOUND_LEFT_GAIN]     = {"dB", 2,  75, -1725, 3000, 0},
    [SOUND_RIGHT_GAIN]    = {"dB", 2,  75, -1725, 3000, 0},
    [SOUND_MIC_GAIN]      = {"dB", 0,   1,  0,  20,  20},
#endif
};

/* private functions to read/write codec registers */
static int codec_write(uint8_t reg, uint8_t val)
{
    return i2c_write(CODEC_I2C_ADDR, reg, 1, &val);
}

#if 0
static int codec_read(uint8_t reg, uint8_t *val)
{
    return i2c_read(CODEC_I2C_ADDR, reg, 1, val);
}
#endif

static void audiohw_mute(bool mute)
{
    if (mute)
        codec_write(CR1, SB_MICBIAS|DAC_MUTE|DACSEL);
    else
        codec_write(CR1, SB_MICBIAS|DACSEL);
}

/* public functions */
int tenthdb2master(int tdb)
{
    /* we lie here a bit and present 0.5dB gain steps
     * but codec has 'variable' gain steps (0.5, 1.0, 2.0)
     * depending on gain region.
     */

    if (tdb < VOLUME_MIN)
        return 31;
    else if (tdb < -115)
        return -(((tdb + 115)/20) - 20); /* 2.0 dB steps */
    else if (tdb < 5)
        return -(((tdb + 5)/10) - 9); /* 1.0 dB steps */
    else
        return -((tdb - 45)/5); /* 0.5 dB steps */
}

void audiohw_preinit(void)
{
    /* PD7 output low */
    GPIO_PDDR &= ~(1<<7);
    GPIO_PDCON |= (1<<7);

    codec_write(PMR2, SB_SLEEP|GIM|SB_MC);
    codec_write(AICR, DAC_SERIAL|ADC_SERIAL|DAC_I2S|ADC_I2S);
    codec_write(CR1, SB_MICBIAS|DAC_MUTE|DACSEL);
    codec_write(CR2, ADC_HPF);
    codec_write(CCR1, CRYSTAL_12M);
    codec_write(CCR2, (FREQ44100 << 4)|FREQ44100);
    codec_write(CRR, RATIO_8|KFAST_32|THRESHOLD_128);
    codec_write(TR1, NOSC);
}

void audiohw_postinit(void)
{
    codec_write(PMR1, SB_OUT|SB_MIX|SB_ADC|SB_IN1|SB_IN2|SB_MIC|SB_IND);

    udelay(10000);

    codec_write(PMR2, GIM | SB_MC);

    udelay(10000);

    codec_write(PMR1, SB_OUT|SB_ADC|SB_IN1|SB_IN2|SB_MIC|SB_IND);

    udelay(10000);

    codec_write(PMR1, SB_ADC|SB_IN1|SB_IN2|SB_MIC|SB_IND);

    sleep(3*HZ);
    GPIO_PDDR |= (1<<7); /* PD7 high */
    sleep(HZ/10);

    audiohw_mute(false);
}

void audiohw_close(void)
{
    /* stub */
}

void audiohw_set_frequency(int fsel)
{
    static const unsigned char values_freq[HW_NUM_FREQ] =
    {
        HW_HAVE_8_([HW_FREQ_8] =   (FREQ8000<<4)|FREQ8000,)
        HW_HAVE_11_([HW_FREQ_11] = (FREQ11025<<4)|FREQ11025,)
        HW_HAVE_12_([HW_FREQ_12] = (FREQ12000<<4)|FREQ12000,)
        HW_HAVE_16_([HW_FREQ_16] = (FREQ16000<<4)|FREQ16000,)
        HW_HAVE_22_([HW_FREQ_22] = (FREQ22050<<4)|FREQ22050,)
        HW_HAVE_24_([HW_FREQ_24] = (FREQ24000<<4)|FREQ24000,)
        HW_HAVE_32_([HW_FREQ_32] = (FREQ32000<<4)|FREQ32000,)
        HW_HAVE_44_([HW_FREQ_44] = (FREQ44100<<4)|FREQ44100,)
        HW_HAVE_48_([HW_FREQ_48] = (FREQ48000<<4)|FREQ48000,)
        HW_HAVE_96_([HW_FREQ_96] = (FREQ96000<<4)|FREQ96000,)
    };

    if ((unsigned)fsel >= HW_NUM_FREQ)
        fsel = HW_FREQ_DEFAULT;

    /* we setup the same sampling freq for DAC and ADC */
    codec_write(CCR2, values_freq[fsel]);
}

void audiohw_set_master_vol(int vol_l, int vol_r)
{
    uint8_t val;

    val = (uint8_t)(vol_r & 0x1f);
    codec_write(CGR9, val);

    val = (uint8_t)(vol_l & 0x1f);
    codec_write(CGR8, val);
}
