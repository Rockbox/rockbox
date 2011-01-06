/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: wm8975.c 28572 2010-11-13 11:38:38Z theseven $
 *
 * Driver for Cirrus Logic CS42L55 audio codec
 *
 * Copyright (c) 2010 Michael Sparmann
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
#include "logf.h"
#include "system.h"
#include "string.h"
#include "audio.h"
#include "sound.h"
#include "audiohw.h"
#include "cscodec.h"
#include "cs42l55.h"

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB", 0,  1, -60,  12, -25},
    [SOUND_BASS]          = {"dB", 1, 15,-105, 120,   0},
    [SOUND_TREBLE]        = {"dB", 1, 15,-105, 120,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
};

static int bass, treble;

/* convert tenth of dB volume (-600..120) to master volume register value */
int tenthdb2master(int db)
{
    /* -60dB to +12dB in 1dB steps */
    /* 0001100 == +12dB (0xc) */
    /* 0000000 == 0dB   (0x0) */
    /* 1000100 == -60dB (0x44, this is actually -58dB) */

    if (db < VOLUME_MIN) return HPACTL_HPAMUTE;
    return (db / 10) & HPACTL_HPAVOL_MASK;
}

static void cscodec_setbits(int reg, unsigned char off, unsigned char on)
{
    unsigned char data = (cscodec_read(reg) & ~off) | on;
    cscodec_write(reg, data);
}

static void audiohw_mute(bool mute)
{
    if (mute) cscodec_setbits(PLAYCTL, 0, PLAYCTL_MSTAMUTE | PLAYCTL_MSTBMUTE);
    else cscodec_setbits(PLAYCTL, PLAYCTL_MSTAMUTE | PLAYCTL_MSTBMUTE, 0);
}

void audiohw_preinit(void)
{
    cscodec_power(true);
    cscodec_clock(true);
    cscodec_reset(true);
    sleep(HZ / 100);
    cscodec_reset(false);

    bass = 0;
    treble = 0;

    /* Ask Cirrus or maybe Apple what the hell this means */
    cscodec_write(HIDDENCTL, HIDDENCTL_UNLOCK);
    cscodec_write(HIDDEN2E, HIDDEN2E_DEFAULT);
    cscodec_write(HIDDEN32, HIDDEN32_DEFAULT);
    cscodec_write(HIDDEN33, HIDDEN33_DEFAULT);
    cscodec_write(HIDDEN34, HIDDEN34_DEFAULT);
    cscodec_write(HIDDEN35, HIDDEN35_DEFAULT);
    cscodec_write(HIDDEN36, HIDDEN36_DEFAULT);
    cscodec_write(HIDDEN37, HIDDEN37_DEFAULT);
    cscodec_write(HIDDEN3A, HIDDEN3A_DEFAULT);
    cscodec_write(HIDDEN3C, HIDDEN3C_DEFAULT);
    cscodec_write(HIDDEN3D, HIDDEN3D_DEFAULT);
    cscodec_write(HIDDEN3E, HIDDEN3E_DEFAULT);
    cscodec_write(HIDDEN3F, HIDDEN3F_DEFAULT);
    cscodec_write(HIDDENCTL, HIDDENCTL_LOCK);

    cscodec_write(PWRCTL2, PWRCTL2_PDN_LINA_ALWAYS | PWRCTL2_PDN_LINB_ALWAYS
                         | PWRCTL2_PDN_HPA_NEVER | PWRCTL2_PDN_HPB_NEVER);
    cscodec_write(CLKCTL1, CLKCTL1_MASTER | CLKCTL1_SCLKMCLK_BEFORE
                         | CLKCTL1_MCLKDIV2);
    cscodec_write(CLKCTL2, CLKCTL2_44100HZ);
    cscodec_write(MISCCTL, MISCCTL_UNDOC4 | MISCCTL_ANLGZC | MISCCTL_DIGSFT);
    cscodec_write(PWRCTL1, PWRCTL1_PDN_CHRG | PWRCTL1_PDN_ADCA
                         | PWRCTL1_PDN_ADCB | PWRCTL1_PDN_CODEC);
    cscodec_write(PLAYCTL, PLAYCTL_PDN_DSP
                         | PLAYCTL_MSTAMUTE | PLAYCTL_MSTBMUTE);
    cscodec_write(PGAACTL, 0);
    cscodec_write(PGABCTL, 0);
    cscodec_write(HPACTL, HPACTL_HPAMUTE);
    cscodec_write(HPBCTL, HPBCTL_HPBMUTE);
    cscodec_write(LINEACTL, LINEACTL_LINEAMUTE);
    cscodec_write(LINEBCTL, LINEBCTL_LINEBMUTE);
    cscodec_write(PWRCTL1, PWRCTL1_PDN_CHRG | PWRCTL1_PDN_ADCA
                         | PWRCTL1_PDN_ADCB);
}

void audiohw_postinit(void)
{
    cscodec_write(HPACTL, 0);
    cscodec_write(HPBCTL, 0);
    cscodec_write(LINEACTL, 0);
    cscodec_write(LINEBCTL, 0);
    cscodec_write(CLSHCTL, CLSHCTL_ADPTPWR_SIGNAL);
    audiohw_mute(false);
}

void audiohw_set_master_vol(int vol_l, int vol_r)
{
    /* -60dB to +12dB in 1dB steps */
    /* 0001100 == +12dB (0xc) */
    /* 0000000 == 0dB   (0x0) */
    /* 1000100 == -60dB (0x44, this is actually -58dB) */

    cscodec_setbits(HPACTL, HPACTL_HPAVOL_MASK | HPACTL_HPAMUTE,
                    vol_l << HPACTL_HPAVOL_SHIFT);
    cscodec_setbits(HPBCTL, HPBCTL_HPBVOL_MASK | HPBCTL_HPBMUTE,
                    vol_r << HPBCTL_HPBVOL_SHIFT);
}

void audiohw_set_lineout_vol(int vol_l, int vol_r)
{
    /* -60dB to +12dB in 1dB steps */
    /* 0001100 == +12dB (0xc) */
    /* 0000000 == 0dB   (0x0) */
    /* 1000100 == -60dB (0x44, this is actually -58dB) */

    cscodec_setbits(LINEACTL, LINEACTL_LINEAVOL_MASK | LINEACTL_LINEAMUTE,
                    vol_l << LINEACTL_LINEAVOL_SHIFT);
    cscodec_setbits(LINEBCTL, LINEBCTL_LINEBVOL_MASK | LINEBCTL_LINEBMUTE,
                    vol_r << LINEBCTL_LINEBVOL_SHIFT);
}

void audiohw_enable_lineout(bool enable)
{
    if (enable)
        cscodec_setbits(PWRCTL2, PWRCTL2_PDN_LINA_MASK | PWRCTL2_PDN_LINB_MASK,
                        PWRCTL2_PDN_LINA_NEVER | PWRCTL2_PDN_LINB_NEVER);
    else
        cscodec_setbits(PWRCTL2, PWRCTL2_PDN_LINA_MASK | PWRCTL2_PDN_LINB_MASK,
                        PWRCTL2_PDN_LINA_ALWAYS | PWRCTL2_PDN_LINB_ALWAYS);
}

void audiohw_set_bass(int value)
{
    bass = value;
    if (bass || treble) cscodec_setbits(PLAYCTL, PLAYCTL_PDN_DSP, 0);
    else cscodec_setbits(PLAYCTL, 0, PLAYCTL_PDN_DSP);
    if (value >= -105 && value <= 120)
        cscodec_setbits(TONECTL, TONECTL_BASS_MASK,
                        (value / 15) << TONECTL_BASS_SHIFT);
}

void audiohw_set_treble(int value)
{
    treble = value;
    if (bass || treble) cscodec_setbits(PLAYCTL, PLAYCTL_PDN_DSP, 0);
    else cscodec_setbits(PLAYCTL, 0, PLAYCTL_PDN_DSP);
    if (value >= -105 && value <= 120)
        cscodec_setbits(TONECTL, TONECTL_TREB_MASK,
                        (value / 15) << TONECTL_TREB_SHIFT);
}

/* Nice shutdown of CS42L55 codec */
void audiohw_close(void)
{
    audiohw_mute(true);
    cscodec_write(HPACTL, HPACTL_HPAMUTE);
    cscodec_write(HPBCTL, HPBCTL_HPBMUTE);
    cscodec_write(LINEACTL, LINEACTL_LINEAMUTE);
    cscodec_write(LINEBCTL, LINEBCTL_LINEBMUTE);
    cscodec_write(PWRCTL1, PWRCTL1_PDN_CHRG | PWRCTL1_PDN_ADCA
                         | PWRCTL1_PDN_ADCB | PWRCTL1_PDN_CODEC);
    cscodec_reset(true);
    cscodec_clock(false);
    cscodec_power(false);
}

/* Note: Disable output before calling this function */
void audiohw_set_frequency(int fsel)
{
    if (fsel == HW_FREQ_8) cscodec_write(CLKCTL2, CLKCTL2_8000HZ);
    else if (fsel == HW_FREQ_11) cscodec_write(CLKCTL2, CLKCTL2_11025HZ);
    else if (fsel == HW_FREQ_12) cscodec_write(CLKCTL2, CLKCTL2_12000HZ);
    else if (fsel == HW_FREQ_16) cscodec_write(CLKCTL2, CLKCTL2_16000HZ);
    else if (fsel == HW_FREQ_22) cscodec_write(CLKCTL2, CLKCTL2_22050HZ);
    else if (fsel == HW_FREQ_24) cscodec_write(CLKCTL2, CLKCTL2_24000HZ);
    else if (fsel == HW_FREQ_32) cscodec_write(CLKCTL2, CLKCTL2_32000HZ);
    else if (fsel == HW_FREQ_44) cscodec_write(CLKCTL2, CLKCTL2_44100HZ);
    else if (fsel == HW_FREQ_48) cscodec_write(CLKCTL2, CLKCTL2_48000HZ);
}

#ifdef HAVE_RECORDING
//TODO: Implement
#endif /* HAVE_RECORDING */
