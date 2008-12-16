/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for AS3514 audio codec
 *
 * Copyright (c) 2007 Daniel Ankers
 * Copyright (c) 2007 Christian Gmeiner
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
#include "cpu.h"
#include "debug.h"
#include "system.h"
#include "audio.h"
#include "sound.h"

#include "audiohw.h"
#include "i2s.h"
#include "ascodec.h"

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB",   0,   1, -74,   6, -25},
    /* HAVE_SW_TONE_CONTROLS */
    [SOUND_BASS]          = {"dB",   0,   1, -24,  24,   0},
    [SOUND_TREBLE]        = {"dB",   0,   1, -24,  24,   0},
    [SOUND_BALANCE]       = {"%",    0,   1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",     0,   1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",    0,   5,   0, 250, 100},
#ifdef HAVE_RECORDING
    [SOUND_MIC_GAIN]      = {"dB",   1,   1,   0,  39,  23},
    [SOUND_LEFT_GAIN]     = {"dB",   1,   1,   0,  31,  23},
    [SOUND_RIGHT_GAIN]    = {"dB",   1,   1,   0,  31,  23},
#endif
};

/* Shadow registers */
static struct as3514_info
{
    int     vol_r;       /* Cached volume level (R) */
    int     vol_l;       /* Cached volume level (L) */
    uint8_t regs[AS3514_NUM_AUDIO_REGS]; /* 8-bit registers */
} as3514;

/* In order to keep track of source for combining volume ranges */
enum
{
    SOURCE_DAC = 0,
    SOURCE_MIC1,
    SOURCE_LINE_IN1,
    SOURCE_LINE_IN1_ANALOG
};

static unsigned int source = SOURCE_DAC;

/*
 * little helper method to set register values.
 * With the help of as3514.regs, we minimize i2c
 * traffic.
 */
static void as3514_write(unsigned int reg, unsigned int value)
{
    if (ascodec_write(reg, value) != 2)
    {
        DEBUGF("as3514 error reg=0x%02x", reg);
    }

    if (reg < ARRAYLEN(as3514.regs))
    {
        as3514.regs[reg] = value;
    }
    else
    {
        DEBUGF("as3514 error reg=0x%02x", reg);
    }
}

/* Helpers to set/clear bits */
static void as3514_set(unsigned int reg, unsigned int bits)
{
    as3514_write(reg, as3514.regs[reg] | bits);
}

static void as3514_clear(unsigned int reg, unsigned int bits)
{
    as3514_write(reg, as3514.regs[reg] & ~bits);
}

static void as3514_write_masked(unsigned int reg, unsigned int bits,
                                unsigned int mask)
{
    as3514_write(reg, (as3514.regs[reg] & ~mask) | (bits & mask));
}

/* convert tenth of dB volume to master volume register value */
int tenthdb2master(int db)
{
    /* +6 to -73.5dB in 1.5dB steps == 53 levels */
    if (db < VOLUME_MIN) {
        return 0x0;
    } else if (db >= VOLUME_MAX) {
        return 0x35;
    } else {
        return((db-VOLUME_MIN)/15); /* VOLUME_MIN is negative */
    }
}

int sound_val2phys(int setting, int value)
{
    int result;

    switch(setting)
    {
#if defined(HAVE_RECORDING)
    case SOUND_LEFT_GAIN:
    case SOUND_RIGHT_GAIN:
    case SOUND_MIC_GAIN:
        result = (value - 23) * 15;
        break;
#endif

    default:
        result = value;
        break;
    }

    return result;
}

/*
 * Initialise the PP I2C and I2S.
 */
void audiohw_preinit(void)
{
    unsigned int i;

    /* read all reg values */
    for (i = 0; i < ARRAYLEN(as3514.regs); i++)
    {
        as3514.regs[i] = ascodec_read(i);
    }

    /* Set ADC off, mixer on, DAC on, line out off, line in off, mic off */

    /* Turn on SUM, DAC */
    as3514_write(AS3514_AUDIOSET1, AUDIOSET1_DAC_on | AUDIOSET1_SUM_on);

    /* Set BIAS on, DITH on, AGC on, IBR_DAC max, LSP_LP on, IBR_LSP min */
    as3514_write(AS3514_AUDIOSET2,
                 AUDIOSET2_IBR_DAC_0 | AUDIOSET2_LSP_LP |
                 AUDIOSET2_IBR_LSP_50);

/* AMS Sansas based on the AS3525 need HPCM enabled, otherwise they output the
   L-R signal on both L and R headphone outputs instead of normal stereo.
   Turning it off saves a little power on targets that don't need it. */
#if (CONFIG_CPU == AS3525)
    /* Set HPCM on, ZCU on */
    as3514_write(AS3514_AUDIOSET3, 0);
#else
    /* Set HPCM off, ZCU on */
    as3514_write(AS3514_AUDIOSET3, AUDIOSET3_HPCM_off);
#endif

    /* Mute and disable speaker */
    as3514_write(AS3514_LSP_OUT_R, LSP_OUT_R_SP_OVC_TO_256MS | 0x00);
    as3514_write(AS3514_LSP_OUT_L, LSP_OUT_L_SP_MUTE | 0x00);

    /* Set headphone over-current to 0, Min volume */
    as3514_write(AS3514_HPH_OUT_R,
                 HPH_OUT_R_HP_OVC_TO_0MS | 0x00);

    /* Headphone ON, MUTE, Min volume */
    as3514_write(AS3514_HPH_OUT_L,
                 HPH_OUT_L_HP_ON | HPH_OUT_L_HP_MUTE | 0x00);

    /* LRCK 24-48kHz */
    as3514_write(AS3514_PLLMODE, PLLMODE_LRCK_24_48);

    /* DAC_Mute_off */
    as3514_set(AS3514_DAC_L, DAC_L_DAC_MUTE_off);

    /* M1_Sup_off */
    as3514_set(AS3514_MIC1_L, MIC1_L_M1_SUP_off);
    /* M2_Sup_off */
    as3514_set(AS3514_MIC2_L, MIC2_L_M2_SUP_off);
}

void audiohw_postinit(void)
{
    /* wait until outputs have stabilized */
    sleep(HZ/4);

#ifdef CPU_PP
    ascodec_suppressor_on(false);
#endif

    audiohw_mute(false);
}

void audiohw_set_master_vol(int vol_l, int vol_r)
{
    unsigned int hph_r, hph_l;
    unsigned int mix_l, mix_r;
    unsigned int mix_reg_r, mix_reg_l;

    /* keep track of current setting */
    as3514.vol_l = vol_l;
    as3514.vol_r = vol_r;

    if (source == SOURCE_LINE_IN1_ANALOG) {
        mix_reg_r = AS3514_LINE_IN1_R;
        mix_reg_l = AS3514_LINE_IN1_L;
    } else {
        mix_reg_r = AS3514_DAC_R;
        mix_reg_l = AS3514_DAC_L;
    }

    /* We combine the mixer channel volume range with the headphone volume
       range - keep first stage as loud as possible */
    if (vol_r <= 0x16) {
        mix_r = vol_r;
        hph_r = 0;
    } else {
        mix_r = 0x16;
        hph_r = vol_r - 0x16;
    }

    if (vol_l <= 0x16) {
        mix_l = vol_l;
        hph_l = 0;
    } else {
        mix_l = 0x16;
        hph_l = vol_l - 0x16;
    }

    as3514_write_masked(mix_reg_r, mix_r, AS3514_VOL_MASK);
    as3514_write_masked(mix_reg_l, mix_l, AS3514_VOL_MASK);
    as3514_write_masked(AS3514_HPH_OUT_R, hph_r, AS3514_VOL_MASK);
    as3514_write_masked(AS3514_HPH_OUT_L, hph_l, AS3514_VOL_MASK);
}

void audiohw_set_lineout_vol(int vol_l, int vol_r)
{
    as3514_write_masked(AS3514_LINE_OUT_R, vol_r,
                        AS3514_VOL_MASK);
    as3514_write_masked(AS3514_LINE_OUT_L, vol_l,
                        AS3514_VOL_MASK);
}

void audiohw_mute(bool mute)
{
    if (mute) {
        as3514_set(AS3514_HPH_OUT_L, HPH_OUT_L_HP_MUTE);
    } else {
        as3514_clear(AS3514_HPH_OUT_L, HPH_OUT_L_HP_MUTE);
    }
}

/* Nice shutdown of AS3514 audio codec */
void audiohw_close(void)
{
    /* mute headphones */
    audiohw_mute(true);

#ifdef CPU_PP
    ascodec_suppressor_on(true);
#endif

    /* turn on common */
    as3514_clear(AS3514_AUDIOSET3, AUDIOSET3_HPCM_off);

    /* turn off everything */
    as3514_clear(AS3514_HPH_OUT_L, HPH_OUT_L_HP_ON);
    as3514_write(AS3514_AUDIOSET1, 0x0);

    /* Allow caps to discharge */
    sleep(HZ/4);
}

void audiohw_set_frequency(int fsel)
{
    (void)fsel;
}

#if defined(HAVE_RECORDING)
void audiohw_enable_recording(bool source_mic)
{
    if (source_mic) {
        source = SOURCE_MIC1;

        /* Sync mixer volumes before switching inputs */
        audiohw_set_master_vol(as3514.vol_l, as3514.vol_r);

        /* ADCmux = Stereo Microphone */
        as3514_write_masked(AS3514_ADC_R, ADC_R_ADCMUX_ST_MIC,
                            ADC_R_ADCMUX);

        /* MIC1_on, LIN1_off */
        as3514_write_masked(AS3514_AUDIOSET1, AUDIOSET1_MIC1_on,
                            AUDIOSET1_MIC1_on | AUDIOSET1_LIN1_on);
        /* M1_AGC_off */
        as3514_clear(AS3514_MIC1_R, MIC1_R_M1_AGC_off);
    } else {
        source = SOURCE_LINE_IN1;

        audiohw_set_master_vol(as3514.vol_l, as3514.vol_r);

        /* ADCmux = Line_IN1 */
        as3514_write_masked(AS3514_ADC_R, ADC_R_ADCMUX_LINE_IN1,
                            ADC_R_ADCMUX);

        /* MIC1_off, LIN1_on */
        as3514_write_masked(AS3514_AUDIOSET1, AUDIOSET1_LIN1_on,
                            AUDIOSET1_MIC1_on | AUDIOSET1_LIN1_on);
    }

    /* ADC_Mute_off */
    as3514_set(AS3514_ADC_L, ADC_L_ADC_MUTE_off);
    /* ADC_on */
    as3514_set(AS3514_AUDIOSET1, AUDIOSET1_ADC_on);
}

void audiohw_disable_recording(void)
{
    source = SOURCE_DAC;

    /* ADC_Mute_on */
    as3514_clear(AS3514_ADC_L, ADC_L_ADC_MUTE_off);

    /* ADC_off, LIN1_off, MIC_off */
    as3514_clear(AS3514_AUDIOSET1,
                 AUDIOSET1_ADC_on | AUDIOSET1_LIN1_on |
                 AUDIOSET1_MIC1_on);

    audiohw_set_master_vol(as3514.vol_l, as3514.vol_r);
}

/**
 * Set recording volume
 *
 * Line in   : 0 .. 23 .. 31 =>
               Volume -34.5 .. +00.0 .. +12.0 dB
 * Mic (left): 0 .. 23 .. 39 =>
 *             Volume -34.5 .. +00.0 .. +24.0 dB
 *
 */
void audiohw_set_recvol(int left, int right, int type)
{
    switch (type)
    {
    case AUDIO_GAIN_MIC:
    {
        /* Combine MIC gains seamlessly with ADC levels */
        unsigned int mic1_r;

        if (left >= 36) {
            /* M1_Gain = +40db, ADR_Vol = +7.5dB .. +12.0 dB =>
               +19.5 dB .. +24.0 dB */
            left -= 8;
            mic1_r = MIC1_R_M1_GAIN_40DB;
        } else if (left >= 32) {
            /* M1_Gain = +34db, ADR_Vol = +7.5dB .. +12.0 dB =>
               +13.5 dB .. +18.0 dB */
            left -= 4; 
            mic1_r = MIC1_R_M1_GAIN_34DB;
        } else {
            /* M1_Gain = +28db, ADR_Vol = -34.5dB .. +12.0 dB =>
               -34.5 dB .. +12.0 dB */
            mic1_r = MIC1_R_M1_GAIN_28DB;
        }

        right = left;

        as3514_write_masked(AS3514_MIC1_R, mic1_r, MIC1_R_M1_GAIN);
        break;
        }
    case AUDIO_GAIN_LINEIN:
        break;
    default:
        return;
    }

    as3514_write_masked(AS3514_ADC_R, right, AS3514_VOL_MASK);
    as3514_write_masked(AS3514_ADC_L, left, AS3514_VOL_MASK);
}

/**
 * Enable line in 1 analog monitoring
 *
 */
void audiohw_set_monitor(bool enable)
{
    if (enable) {
        source = SOURCE_LINE_IN1_ANALOG;

        as3514_set(AS3514_AUDIOSET1, AUDIOSET1_LIN1_on);
        as3514_set(AS3514_LINE_IN1_R, LINE_IN1_R_LI1R_MUTE_off);
        as3514_set(AS3514_LINE_IN1_L, LINE_IN1_L_LI1L_MUTE_off);
    }
    else {
        as3514_clear(AS3514_AUDIOSET1, AUDIOSET1_LIN1_on);
        as3514_clear(AS3514_LINE_IN1_R, LINE_IN1_R_LI1R_MUTE_off);
        as3514_clear(AS3514_LINE_IN1_L, LINE_IN1_L_LI1L_MUTE_off);
    }

    /* Sync mixer volume */
    audiohw_set_master_vol(as3514.vol_l, as3514.vol_r);
}
#endif /* HAVE_RECORDING */
