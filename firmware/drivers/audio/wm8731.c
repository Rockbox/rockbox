/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for WM8711/WM8721/WM8731 audio codecs
 *
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in January 2006
 *
 * Original file: linux/arch/armnommu/mach-ipod/audio.c
 *
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
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
#include "logf.h"
#include "system.h"
#include "string.h"
#include "audio.h"

#include "wmcodec.h"
#include "audiohw.h"
#include "sound.h"

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB", 0,  1, -74,   6, -25},
    /* HAVE_SW_TONE_CONTROLS */
    [SOUND_BASS]          = {"dB", 0,  1, -24,  24,   0},
    [SOUND_TREBLE]        = {"dB", 0,  1, -24,  24,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
#if defined(HAVE_WM8731) && defined(HAVE_RECORDING)
    [SOUND_LEFT_GAIN]     = {"dB", 1,  1,   0,  31,  23},
    [SOUND_RIGHT_GAIN]    = {"dB", 1,  1,   0,  31,  23},
    [SOUND_MIC_GAIN]      = {"dB", 1,  1,   0,   1,   0},
#endif
};

/* Init values/shadows
 * Ignore bit 8 since that only specifies "both" for updating
 * gains - "RESET" (15h) not included */
static unsigned char wmc_regs[WMC_NUM_REGS] =
{
#ifdef HAVE_WM8731
    [LINVOL]     = LINVOL_DEFAULT,
    [RINVOL]     = RINVOL_DEFAULT,
#endif
    [LOUTVOL]    = LOUTVOL_DEFAULT | WMC_OUT_ZCEN,
    [ROUTVOL]    = LOUTVOL_DEFAULT | WMC_OUT_ZCEN,
#if defined(HAVE_WM8711) || defined(HAVE_WM8731)
    /* BYPASS on by default - OFF until needed */
    [AAPCTRL]    = AAPCTRL_DEFAULT & ~AAPCTRL_BYPASS,
    /* CLKOUT and OSC on by default - OFF unless needed by a target */ 
    [PDCTRL]     = PDCTRL_DEFAULT | PDCTRL_CLKOUTPD | PDCTRL_OSCPD,
#elif defined(HAVE_WM8721)
    /* No BYPASS */
    [AAPCTRL]    = AAPCTRL_DEFAULT,
    /* No CLKOUT or OSC */
    [PDCTRL]     = PDCTRL_DEFAULT,
#endif
    [DAPCTRL]    = DAPCTRL_DEFAULT,
#ifndef CODEC_SLAVE
    [AINTFCE]    = AINTFCE_FORMAT_I2S | AINTFCE_IWL_16BIT | AINTFCE_MS,
#else
    [AINTFCE]    = AINTFCE_FORMAT_I2S | AINTFCE_IWL_16BIT,
#endif
    [SAMPCTRL]   = SAMPCTRL_DEFAULT,
    [ACTIVECTRL] = ACTIVECTRL_DEFAULT,
};

static void wmc_write(int reg, unsigned val)
{
    if ((unsigned)reg >= WMC_NUM_REGS)
        return;

    wmc_regs[reg] = (unsigned char)val;
    wmcodec_write(reg, val);
}

static void wmc_set(int reg, unsigned bits)
{
    wmc_write(reg, wmc_regs[reg] | bits);
}

static void wmc_clear(int reg, unsigned bits)
{
    wmc_write(reg, wmc_regs[reg] & ~bits);
}

static void wmc_write_masked(int reg, unsigned bits, unsigned mask)
{
    wmc_write(reg, (wmc_regs[reg] & ~mask) | (bits & mask));
}

/* convert tenth of dB volume (-730..60) to master volume register value */
int tenthdb2master(int db)
{
    /* +6 to -73dB 1dB steps (plus mute == 80levels) 7bits */
    /* 1111111 == +6dB  (0x7f) */
    /* 1111001 == 0dB   (0x79) */
    /* 0110000 == -73dB (0x30 */
    /* 0101111 == mute  (0x2f) */

    if (db < VOLUME_MIN) {
        return 0x2f;
    } else {
        return((db/10)+0x30+73);
    }
}

int sound_val2phys(int setting, int value)
{
    int result;

    switch(setting)
    {
#ifdef HAVE_RECORDING
    case SOUND_LEFT_GAIN:
    case SOUND_RIGHT_GAIN:
        result = (value - 23) * 15;
        break;
    case SOUND_MIC_GAIN:
        result = value * 200;
        break;
#endif
    default:
        result = value;
        break;
    }

    return result;
}

void audiohw_mute(bool mute)
{
    if (mute) {
        /* Set DACMU = 1 to soft-mute the audio DACs. */
        wmc_set(DAPCTRL, DAPCTRL_DACMU);
    } else {
        /* Set DACMU = 0 to soft-un-mute the audio DACs. */
        wmc_clear(DAPCTRL, DAPCTRL_DACMU);
    }
}

static void codec_set_active(int active)
{
    /* set active to 0x0 or 0x1 */
    wmc_write(ACTIVECTRL, active ? ACTIVECTRL_ACTIVE : 0);
}

void audiohw_preinit(void)
{
    /* POWER UP SEQUENCE */
    /* 1) Switch on power supplies. By default the WM codec is in Standby Mode,
     *    the DAC is digitally muted and the Audio Interface and Outputs are
     *    all OFF. */
    wmcodec_write(RESET, RESET_RESET);

    /* 2) Set all required bits in the Power Down register (0Ch) to '0';
     *    EXCEPT the OUTPD bit, this should be set to '1' (Default). */
    wmc_clear(PDCTRL, PDCTRL_DACPD | PDCTRL_POWEROFF);

    /* 3) Set required values in all other registers except 12h (Active). */
    wmc_set(AINTFCE, 0); /* Set no bits - write init/shadow value */

    wmc_set(AAPCTRL, AAPCTRL_DACSEL);
    wmc_write(SAMPCTRL, WMC_USB24_44100HZ);

    /* 4) Set the 'Active' bit in register 12h. */
    codec_set_active(true);

    /* 5) The last write of the sequence should be setting OUTPD to '0'
     *    (active) in register 0Ch, enabling the DAC signal path, free
     *     of any significant power-up noise. */
    wmc_clear(PDCTRL, PDCTRL_OUTPD);
}

void audiohw_postinit(void)
{
    sleep(HZ);

    audiohw_mute(false);

#if defined(IRIVER_H10) || defined(IRIVER_H10_5GB)
    /* We need to enable bit 4 of GPIOL for output for sound on H10 */
    GPIO_SET_BITWISE(GPIOL_OUTPUT_VAL, 0x10);
#elif defined(PHILIPS_HDD1630)
    GPO32_ENABLE |= 0x2;
    GPO32_VAL &= ~0x2;
#endif
}

void audiohw_set_master_vol(int vol_l, int vol_r)
{
    /* +6 to -73dB 1dB steps (plus mute == 80levels) 7bits */
    /* 1111111 == +6dB */
    /* 1111001 == 0dB */
    /* 0110000 == -73dB */
    /* 0101111 == mute (0x2f) */
    wmc_write_masked(LOUTVOL, vol_l, WMC_OUT_VOL_MASK);
    wmc_write_masked(ROUTVOL, vol_r, WMC_OUT_VOL_MASK);
}

/* Nice shutdown of WM codec */
void audiohw_close(void)
{
    /* POWER DOWN SEQUENCE */
    /* 1) Set the OUTPD bit to '1' (power down). */
    wmc_set(PDCTRL, PDCTRL_OUTPD);
    /* 2) Remove the WM codec supplies. */
}

void audiohw_set_frequency(int fsel)
{
    /* For 24MHz MCLK */
    static const unsigned char srctrl_table[HW_NUM_FREQ] =
    {
        [HW_FREQ_8]  = WMC_USB24_8000HZ,
        [HW_FREQ_32] = WMC_USB24_32000HZ,
        [HW_FREQ_44] = WMC_USB24_44100HZ,
        [HW_FREQ_48] = WMC_USB24_48000HZ,
        [HW_FREQ_88] = WMC_USB24_88200HZ,
        [HW_FREQ_96] = WMC_USB24_96000HZ,
    };

    if ((unsigned)fsel >= HW_NUM_FREQ)
        fsel = HW_FREQ_DEFAULT;

    codec_set_active(false);
    wmc_write(SAMPCTRL, srctrl_table[fsel]);
    codec_set_active(true);
}

#if defined(HAVE_WM8731) && defined(HAVE_RECORDING)
/* WM8731 only */
void audiohw_enable_recording(bool source_mic)
{
    /* NOTE: When switching to digital monitoring we will not want
     * the DAC disabled. */

    codec_set_active(false);

    if (source_mic) {
        wmc_set(LINVOL, WMC_IN_MUTE);
        wmc_set(RINVOL, WMC_IN_MUTE);

        wmc_write_masked(PDCTRL, PDCTRL_LINEINPD | PDCTRL_DACPD,
                         PDCTRL_LINEINPD | PDCTRL_MICPD |
                         PDCTRL_ADCPD | PDCTRL_DACPD);
        wmc_write_masked(AAPCTRL, AAPCTRL_INSEL | AAPCTRL_SIDETONE,
                         AAPCTRL_MUTEMIC | AAPCTRL_INSEL |
                         AAPCTRL_BYPASS | AAPCTRL_DACSEL |
                         AAPCTRL_SIDETONE);
    } else {
        wmc_write_masked(PDCTRL, PDCTRL_MICPD | PDCTRL_DACPD,
                         PDCTRL_LINEINPD | PDCTRL_MICPD |
                         PDCTRL_ADCPD | PDCTRL_DACPD);
        wmc_write_masked(AAPCTRL, AAPCTRL_MUTEMIC | AAPCTRL_BYPASS,
                         AAPCTRL_MUTEMIC | AAPCTRL_INSEL |
                         AAPCTRL_BYPASS | AAPCTRL_DACSEL |
                         AAPCTRL_SIDETONE);

        wmc_clear(LINVOL, WMC_IN_MUTE);
        wmc_clear(RINVOL, WMC_IN_MUTE);
    }

    codec_set_active(true);
}

void audiohw_disable_recording(void)
{
    codec_set_active(false);

    /* Mute line inputs */
    wmc_set(LINVOL, WMC_IN_MUTE);
    wmc_set(RINVOL, WMC_IN_MUTE);
    wmc_set(AAPCTRL, AAPCTRL_MUTEMIC);

    /* Turn off input analog audio paths */
    wmc_clear(AAPCTRL, AAPCTRL_BYPASS | AAPCTRL_SIDETONE);

    /* Set power config */
    wmc_write_masked(PDCTRL,
                     PDCTRL_LINEINPD | PDCTRL_MICPD | PDCTRL_ADCPD,
                     PDCTRL_LINEINPD | PDCTRL_MICPD | PDCTRL_DACPD |
                     PDCTRL_ADCPD);

    /* Select DAC */
    wmc_set(AAPCTRL, AAPCTRL_DACSEL);

    codec_set_active(true);
}

void audiohw_set_recvol(int left, int right, int type)
{
    switch (type)
    {
    case AUDIO_GAIN_MIC:
        if (left > 0) {
            wmc_set(AAPCTRL, AAPCTRL_MIC_BOOST);
        }
        else {
            wmc_clear(AAPCTRL, AAPCTRL_MIC_BOOST);
        }
        break;
    case AUDIO_GAIN_LINEIN:
        wmc_write_masked(LINVOL, left, WMC_IN_VOL_MASK);
        wmc_write_masked(RINVOL, right, WMC_IN_VOL_MASK);
        break;
    default:
        return;
    }
}

void audiohw_set_monitor(bool enable)
{
    if(enable) {
        wmc_clear(PDCTRL, PDCTRL_LINEINPD);
        wmc_set(AAPCTRL, AAPCTRL_BYPASS);
    }
    else {
        wmc_clear(AAPCTRL, AAPCTRL_BYPASS);
        wmc_set(PDCTRL, PDCTRL_LINEINPD);
    }
}
#endif /* HAVE_WM8731 && HAVE_RECORDING */
