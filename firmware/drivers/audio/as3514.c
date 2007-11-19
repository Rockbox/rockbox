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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "cpu.h"
#include "debug.h"
#include "system.h"
#include "audio.h"

#include "audiohw.h"
#include "i2s.h"
#include "i2c-pp.h"

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB",   0,   1, -74,   6, -25},
    /* HAVE_SW_TONE_CONTROLS */
    [SOUND_BASS]          = {"dB",   0,   1, -24,  24,   0},
    [SOUND_TREBLE]        = {"dB",   0,   1, -24,  24,   0},
    [SOUND_BALANCE]       = {"%",    0,   1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",     0,   1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",    0,   5,   0, 250, 100},
    [SOUND_MIC_GAIN]      = {"dB",   1,   1,   0,  39,  23},
    [SOUND_LEFT_GAIN]     = {"dB",   1,   1,   0,  31,  23},
    [SOUND_RIGHT_GAIN]    = {"dB",   1,   1,   0,  31,  23},
};

/* Shadow registers */
struct as3514_info
{
    int          vol_r;       /* Cached volume level (R) */
    int          vol_l;       /* Cached volume level (L) */
    unsigned int regs[0x1e];  /* last audio register: PLLMODE 0x1d */
} as3514;

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
    if (pp_i2c_send(AS3514_I2C_ADDR, reg, value) != 2)
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
static void as3514_write_or(unsigned int reg, unsigned int bits)
{
    as3514_write(reg, as3514.regs[reg] | bits);
}

static void as3514_write_and(unsigned int reg, unsigned int bits)
{
    as3514_write(reg, as3514.regs[reg] & bits);
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
    case SOUND_LEFT_GAIN:
    case SOUND_RIGHT_GAIN:
    case SOUND_MIC_GAIN:
        result = (value - 23) * 15;
        break;

    default:
        result = value;
        break;
    }

    return result;
}

void audiohw_reset(void);

/*
 * Initialise the PP I2C and I2S.
 */
void audiohw_init(void)
{
    unsigned int i;

    /* reset I2C */
    i2c_init();

    /* normal outputs for CDI and I2S pin groups */
    DEV_INIT2 &= ~0x300;

    /*mini2?*/
    DEV_INIT1 &=~0x3000000;
    /*mini2?*/

    /* device reset */
    DEV_RS |= DEV_I2S;
    DEV_RS &=~DEV_I2S;

    /* device enable */
    DEV_EN |= (DEV_I2S | 0x7);

    /* enable external dev clock clocks */
    DEV_EN |= 0x2;

    /* external dev clock to 24MHz */
    outl(inl(0x70000018) & ~0xc, 0x70000018);

    i2s_reset();

    /* Set ADC off, mixer on, DAC on, line out off, line in off, mic off */

    /* Turn on SUM, DAC */
    as3514_write(AUDIOSET1, (1 << 6) | (1 << 5));

    /* Set BIAS on, DITH on, AGC on, IBR_DAC max, LSP_LP on, IBR_LSP min */
    as3514_write(AUDIOSET2, (1 << 2) | (3 << 0));

    /* Set HPCM off, ZCU off*/
    as3514_write(AUDIOSET3, (1 << 2) | (1 << 0));

    /* Mute and disable speaker */
    as3514_write(LSP_OUT_R, 0);
    as3514_write(LSP_OUT_L, (1 << 7));

    /* set vol and set headphone over-current to 0 */
    as3514_write(HPH_OUT_R, (0x3 << 6) | 0x16);
    /* set default vol for headphone */
    as3514_write(HPH_OUT_L, 0x16);

    /* LRCK 24-48kHz */
    as3514_write(PLLMODE, 0x00);

    /* DAC_Mute_off */
    as3514_write_or(DAC_L, (1 << 6));

    /* M1_Sup_off */
    as3514_write_or(MIC1_L, (1 << 7));
    /* M2_Sup_off */
    as3514_write_or(MIC2_L, (1 << 7));

    /* read all reg values */
    for (i = 0; i < ARRAYLEN(as3514.regs); i++)
    {
        as3514.regs[i] = i2c_readbyte(AS3514_I2C_ADDR, i);
    }
}

void audiohw_postinit(void)
{
}

/* Silently enable / disable audio output */
void audiohw_enable_output(bool enable)
{
    if (enable) {
        /* reset the I2S controller into known state */
        i2s_reset();

        as3514_write_or(HPH_OUT_L, (1 << 6)); /* power on */
        audiohw_mute(0);
    } else {
        audiohw_mute(1);
        as3514_write_and(HPH_OUT_L, ~(1 << 6)); /* power off */
    }
}

int audiohw_set_master_vol(int vol_l, int vol_r)
{
    unsigned int hph_r = as3514.regs[HPH_OUT_R] & ~0x1f;
    unsigned int hph_l = as3514.regs[HPH_OUT_L] & ~0x1f;
    unsigned int mix_l, mix_r;
    unsigned int mix_reg_r, mix_reg_l;

    /* keep track of current setting */
    as3514.vol_l = vol_l;
    as3514.vol_r = vol_r;

    if (source == SOURCE_LINE_IN1_ANALOG) {
        mix_reg_r = LINE_IN1_R;
        mix_reg_l = LINE_IN1_L;
    } else {
        mix_reg_r = DAC_R;
        mix_reg_l = DAC_L;
    }

    mix_r = as3514.regs[mix_reg_r] & ~0x1f;    
    mix_l = as3514.regs[mix_reg_l] & ~0x1f;    

    /* we combine the mixer channel volume range with the headphone volume
       range */
    if (vol_r <= 0x16) {
        mix_r |= vol_r;
        /* hph_r - set 0 */
    } else {
        mix_r |= 0x16;
        hph_r += vol_r - 0x16;
    }

    if (vol_l <= 0x16) {
        mix_l |= vol_l;
        /* hph_l - set 0 */
    } else {
        mix_l |= 0x16;
        hph_l += vol_l - 0x16;
    }

    as3514_write(mix_reg_r, mix_r);
    as3514_write(mix_reg_l, mix_l);
    as3514_write(HPH_OUT_R, hph_r);
    as3514_write(HPH_OUT_L, hph_l);

    return 0;
}

int audiohw_set_lineout_vol(int vol_l, int vol_r)
{
    as3514_write(LINE_OUT_R, vol_r);
    as3514_write(LINE_OUT_L, (1 << 6) | vol_l);

    return 0;
}

void audiohw_mute(bool mute)
{
    if (mute) {
        as3514_write_or(HPH_OUT_L, (1 << 7));
    } else {
        as3514_write_and(HPH_OUT_L, ~(1 << 7));
    }
}

/* Nice shutdown of WM8758 codec */
void audiohw_close(void)
{
    /* mute headphones */
    audiohw_mute(true);

    /* turn off everything */
    as3514_write(AUDIOSET1, 0x0);
}

void audiohw_set_sample_rate(int sampling_control)
{
    (void)sampling_control;
}

void audiohw_enable_recording(bool source_mic)
{
    if (source_mic) {
        source = SOURCE_MIC1;

        /* Sync mixer volumes before switching inputs */
        audiohw_set_master_vol(as3514.vol_l, as3514.vol_r);

        /* ADCmux = Stereo Microphone */
        as3514_write_and(ADC_R, ~(0x3 << 6));
        /* MIC1_on, LIN1_off */
        as3514_write(AUDIOSET1,
            (as3514.regs[AUDIOSET1] & ~(1 << 2)) | (1 << 0));
        /* M1_AGC_off */
        as3514_write_and(MIC1_R, ~(1 << 7));
    } else {
        source = SOURCE_LINE_IN1;

        audiohw_set_master_vol(as3514.vol_l, as3514.vol_r);

        /* ADCmux = Line_IN1 */
        as3514_write(ADC_R,
            (as3514.regs[ADC_R] & ~(0x3 << 6)) | (0x1 << 6));
        /* MIC1_off, LIN1_on */
        as3514_write(AUDIOSET1,
            (as3514.regs[AUDIOSET1] & ~(1 << 0)) | (1 << 2));
    }

    /* ADC_Mute_off */
    as3514_write_or(ADC_L, (1 << 6));
    /* ADC_on */
    as3514_write_or(AUDIOSET1, (1 << 7));
}

void audiohw_disable_recording(void)
{
    source = SOURCE_DAC;

    /* ADC_Mute_on */
    as3514_write_and(ADC_L, ~(1 << 6));

    /* ADC_off, LIN1_off, MIC_off */
    as3514_write_and(AUDIOSET1, ~((1 << 7) | (1 << 2) | (1 << 0)));

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
        unsigned int mic1_r = as3514.regs[MIC1_R] & ~(0x3 << 5);

        if (left >= 36) {
            /* M1_Gain = +40db, ADR_Vol = +7.5dB .. +12.0 dB =>
               +19.5 dB .. +24.0 dB */
            left -= 8;
            mic1_r |= (0x2 << 5);
        } else if (left >= 32) {
            /* M1_Gain = +34db, ADR_Vol = +7.5dB .. +12.0 dB =>
               +13.5 dB .. +18.0 dB */
            left -= 4; 
            mic1_r |= (0x1 << 5);
        }
            /* M1_Gain = +28db, ADR_Vol = -34.5dB .. +12.0 dB =>
               -34.5 dB .. +12.0 dB */

        right = left;

        as3514_write(MIC1_R, mic1_r);
        break;
        }
    case AUDIO_GAIN_LINEIN:
        break;
    default:
        return;
    }

    as3514_write(ADC_R, (as3514.regs[ADC_R] & ~0x1f) | right);
    as3514_write(ADC_L, (as3514.regs[ADC_L] & ~0x1f) | left);
}

/**
 * Enable line in 1 analog monitoring
 *
 */
void audiohw_set_monitor(bool enable)
{
    /* LI1R_Mute_on - default */
    unsigned int line_in1_r = as3514.regs[LINE_IN1_R] & ~(1 << 5);
    /* LI1L_Mute_on - default */
    unsigned int line_in1_l = as3514.regs[LINE_IN1_L] & ~(1 << 5);
    /* LIN1_off - default */
    unsigned int audioset1 = as3514.regs[AUDIOSET1] & ~(1 << 2);

    if (enable) {
        source = SOURCE_LINE_IN1_ANALOG;

        /* LI1R_Mute_off */
        line_in1_r |= (1 << 5);
        /* LI1L_Mute_off */
        line_in1_l |= (1 << 5);
        /* LIN1_on */
        audioset1 |= (1 << 2);
    }

    as3514_write(AUDIOSET1, audioset1);
    as3514_write(LINE_IN1_R, line_in1_r);
    as3514_write(LINE_IN1_L, line_in1_l);

    /* Sync mixer volume */
    audiohw_set_master_vol(as3514.vol_l, as3514.vol_r);
}
