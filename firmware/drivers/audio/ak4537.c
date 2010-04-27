/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2009 Mark Arigo
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
#include "config.h"
#include "system.h"
#include "string.h"

/*#define LOGF_ENABLE*/
#include "logf.h"

#include "pcm_sampr.h"
#include "audio.h"
#include "akcodec.h"
#include "audiohw.h"
#include "sound.h"

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB", 0,  1,-127,   0, -25},
    /* HAVE_SW_TONE_CONTROLS */
    [SOUND_BASS]          = {"dB", 0,  1, -24,  24,   0},
    [SOUND_TREBLE]        = {"dB", 0,  1, -24,  24,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
#if defined(HAVE_RECORDING)
    [SOUND_LEFT_GAIN]     = {"dB", 1,  1,   0,  31,  23},
    [SOUND_RIGHT_GAIN]    = {"dB", 1,  1,   0,  31,  23},
    [SOUND_MIC_GAIN]      = {"dB", 1,  1,   0,   1,   0},
#endif
};

static unsigned char akc_regs[AKC_NUM_REGS];

static void akc_write(int reg, unsigned val)
{
    if ((unsigned)reg >= AKC_NUM_REGS)
        return;

    akc_regs[reg] = (unsigned char)val;
    akcodec_write(reg, val);
}

static void akc_set(int reg, unsigned bits)
{
    akc_write(reg, akc_regs[reg] | bits);
}

static void akc_clear(int reg, unsigned bits)
{
    akc_write(reg, akc_regs[reg] & ~bits);
}

static void akc_write_masked(int reg, unsigned bits, unsigned mask)
{
    akc_write(reg, (akc_regs[reg] & ~mask) | (bits & mask));
}

#if 0
static void codec_set_active(int active)
{
    (void)active;
}
#endif

/* convert tenth of dB volume (-1270..0) to master volume register value */
int tenthdb2master(int db)
{
    if (db < VOLUME_MIN)
        return 0xff; /* mute */
    else if (db >= VOLUME_MAX)
        return 0x00;
    else
        return ((-db)/5);
}

int sound_val2phys(int setting, int value)
{
    int result;

    switch(setting)
    {
#ifdef HAVE_RECORDING
    case SOUND_LEFT_GAIN:
    case SOUND_RIGHT_GAIN:
        result = (value - 23) * 15; /* fix */
        break;
    case SOUND_MIC_GAIN:
        result = value * 200; /* fix */
        break;
#endif
    default:
        result = value;
        break;
    }

    return result;
}

/*static void audiohw_mute(bool mute)
{
    if (mute)
    {
        akc_set(AK4537_DAC, SMUTE);
        udelay(200000);
    }
    else
    {
        udelay(200000);
        akc_clear(AK4537_DAC, SMUTE);
    }
}*/

void audiohw_preinit(void)
{
    int i;
    for (i = 0; i < AKC_NUM_REGS; i++)
        akc_regs[i] = akcodec_read(i);

    /* POWER UP SEQUENCE (from the datasheet) */
    /* Note: the delay length is what the OF uses, although the datasheet
       suggests they can be shorter */

    /* power up VCOM */
    akc_set(AK4537_PM1, PMVCM);
    udelay(100000);

    /* setup AK4537_SIGSEL1 */
    akc_set(AK4537_SIGSEL1, ALCS | MOUT2);        
    udelay(100000);

    /* setup AK4537_SIGSEL2 */
    akc_write_masked(AK4537_SIGSEL2, DAHS, (DAHS | HPL | HPR));
    udelay(100000);

    /* setup AK4537_MODE1 */
    akc_write_masked(AK4537_MODE1, DIF_I2S | BICK_32FS | MCKI_PLL_12000KHZ,
                     (DIF_MASK | BICK_MASK | MCKI_MASK));
    udelay(100000);

    /* CLOCK SETUP - X'tal used in PLL mode (master mode) */

    /* release the pull-down of the XTI pin and power-up the X'tal osc */
    akc_write_masked(AK4537_PM2, PMXTL, (MCLKPD | PMXTL));
    udelay(100000);

    /* power-up the PLL */
    akc_set(AK4537_PM2, PMPLL);
    udelay(100000);

    /* enable MCKO output and setup MCKO output freq */
    akc_set(AK4537_MODE1, MCKO_EN);
    udelay(100000);

    /* ENABLE HEADPHONE AMP OUTPUT */

    /* setup the sampling freq if PLL mode is used */
    akc_write_masked(AK4537_MODE2, AKC_PLL_44100HZ, FS_MASK);

    /* setup the low freq boost level */
    akc_write_masked(AK4537_DAC, BST_OFF, BST_MASK);

    /* setup the digital volume */
    akc_write(AK4537_ATTL, 0x10);
    akc_write(AK4537_ATTR, 0x10);

    /* power up the DAC */
    akc_set(AK4537_PM2, PMDAC);
    udelay(100000);

    /* power up the headphone amp */
    akc_clear(AK4537_SIGSEL2, HPL | HPR);
    udelay(100000);

    /* power up the common voltage of headphone amp */
    akc_set(AK4537_PM2, PMHPL | PMHPR);
    udelay(100000);
}

void audiohw_postinit(void)
{
    /* nothing */
}

void audiohw_close(void)
{
    /* POWER DOWN SEQUENCE (from the datasheet) */

    /* mute */
    akc_write(AK4537_ATTL, 0xff);
    akc_write(AK4537_ATTR, 0xff);
    akc_set(AK4537_DAC, SMUTE);
    udelay(100000);

    /* power down the common voltage of headphone amp */
    akc_clear(AK4537_PM2, PMHPL | PMHPR);

    /* power down the DAC */
    akc_clear(AK4537_PM2, PMDAC);

    /* power down the headphone amp */
    akc_set(AK4537_SIGSEL2, HPL | HPR);

    /* disable MCKO */
    akc_clear(AK4537_MODE1, MCKO_EN);

    /* power down X'tal and PLL, pull down the XTI pin */
    akc_write_masked(AK4537_PM2, MCLKPD, (MCLKPD | PMXTL | PMPLL));

    /* power down VCOM */
    akc_clear(AK4537_PM1, PMVCM);
    udelay(100000);

    akcodec_close(); /* target-specific */
}

void audiohw_set_master_vol(int vol_l, int vol_r)
{
    akc_write(AK4537_ATTL, vol_l & 0xff);
    akc_write(AK4537_ATTR, vol_r & 0xff);
}

void audiohw_set_frequency(int fsel)
{
    static const unsigned char srctrl_table[HW_NUM_FREQ] =
    {
        HW_HAVE_8_([HW_FREQ_8]   = AKC_PLL_8000HZ, )
        HW_HAVE_11_([HW_FREQ_11] = AKC_PLL_11025HZ,)
        HW_HAVE_16_([HW_FREQ_16] = AKC_PLL_16000HZ,)
        HW_HAVE_22_([HW_FREQ_22] = AKC_PLL_22050HZ,)
        HW_HAVE_24_([HW_FREQ_24] = AKC_PLL_24000HZ,)
        HW_HAVE_32_([HW_FREQ_32] = AKC_PLL_32000HZ,)
        HW_HAVE_44_([HW_FREQ_44] = AKC_PLL_44100HZ,)
        HW_HAVE_48_([HW_FREQ_48] = AKC_PLL_48000HZ,)
    };

    if ((unsigned)fsel >= HW_NUM_FREQ)
        fsel = HW_FREQ_DEFAULT;

    akc_write_masked(AK4537_MODE2, srctrl_table[fsel], FS_MASK);
}

#if defined(HAVE_RECORDING)
void audiohw_enable_recording(bool source_mic)
{
    (void)source_mic;
}

void audiohw_disable_recording(void)
{
}

void audiohw_set_recvol(int left, int right, int type)
{
    (void)left;
    (void)right;
    (void)type;
}

void audiohw_set_monitor(bool enable)
{
    (void)enable;
}
#endif /* HAVE_RECORDING */
