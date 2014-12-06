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
#include "kernel.h"
#include "string.h"
#include "audio.h"
#include "sound.h"
#include "audiohw.h"
#include "cscodec.h"
#include "cs42l55.h"

static int bass, treble;
static int active_dsp_modules;  /* powered DSP modules mask */

/* convert tenth of dB volume (-600..120) to volume register value */
static int vol_tenthdb2hw(int db)
{
    /* -60dB to +12dB in 1dB steps */
    /* 0001100 == +12dB (0xc) */
    /* 0000000 == 0dB   (0x0) */
    /* 1000100 == -60dB (0x44, this is actually -58dB) */
    if (db <= -600) return HPACTL_HPAMUTE;
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
    active_dsp_modules = 0;

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

void audiohw_set_volume(int vol_l, int vol_r)
{
    vol_l = vol_tenthdb2hw(vol_l);
    vol_r = vol_tenthdb2hw(vol_r);
    cscodec_setbits(HPACTL, HPACTL_HPAVOL_MASK | HPACTL_HPAMUTE,
                    vol_l << HPACTL_HPAVOL_SHIFT);
    cscodec_setbits(HPBCTL, HPBCTL_HPBVOL_MASK | HPBCTL_HPBMUTE,
                    vol_r << HPBCTL_HPBVOL_SHIFT);
}

void audiohw_set_lineout_volume(int vol_l, int vol_r)
{
    vol_l = vol_tenthdb2hw(vol_l);
    vol_r = vol_tenthdb2hw(vol_r);
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

static void handle_dsp_power(int dsp_module, bool onoff)
{
    if (onoff)
    {
        if (!active_dsp_modules)
            cscodec_setbits(PLAYCTL, PLAYCTL_PDN_DSP, 0);
        active_dsp_modules |= dsp_module;
    }
    else
    {
        active_dsp_modules &= ~dsp_module;
        if (!active_dsp_modules)
            cscodec_setbits(PLAYCTL, 0, PLAYCTL_PDN_DSP);
    }
}

static void handle_tone_onoff(void)
{
    if (bass || treble)
    {
        handle_dsp_power(DSP_MODULE_TONE, 1);
        cscodec_setbits(BTCTL, 0, BTCTL_TCEN);
    }
    else
    {
        cscodec_setbits(BTCTL, BTCTL_TCEN, 0);
        handle_dsp_power(DSP_MODULE_TONE, 0);
    }
}

void audiohw_set_bass(int value)
{
    bass = value;
    handle_tone_onoff();
    if (value >= -105 && value <= 120)
        cscodec_setbits(TONECTL, TONECTL_BASS_MASK,
                        (8 - value / 15) << TONECTL_BASS_SHIFT);
}

void audiohw_set_treble(int value)
{
    treble = value;
    handle_tone_onoff();
    if (value >= -105 && value <= 120)
        cscodec_setbits(TONECTL, TONECTL_TREB_MASK,
                        (8 - value / 15) << TONECTL_TREB_SHIFT);
}

void audiohw_set_bass_cutoff(int value)
{
    cscodec_setbits(BTCTL, BTCTL_BASSCF_MASK,
                    (value - 1) << BTCTL_BASSCF_SHIFT);
}

void audiohw_set_treble_cutoff(int value)
{
    cscodec_setbits(BTCTL, BTCTL_TREBCF_MASK,
                    (value - 1) << BTCTL_TREBCF_SHIFT);
}

void audiohw_set_prescaler(int value)
{
    cscodec_setbits(MSTAVOL, MSTAVOL_VOLUME_MASK,
                    (-value / 5) << MSTAVOL_VOLUME_SHIFT);
    cscodec_setbits(MSTBVOL, MSTBVOL_VOLUME_MASK,
                    (-value / 5) << MSTBVOL_VOLUME_SHIFT);
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
/* Saves power for targets supporting only mono microphone. */
#define MONO_MIC

/* Classic OF does not bypass PGA when line-in is selected. It can be
 * bypassed for power saving instead of using it at fixed 0 dB gain.
 * See CS42L55 DS, Figure 1 (Typical Connection Diagram), Note 4.
 */
/*#define BYPASS_PGA*/

void audiohw_enable_recording(bool source_mic)
{
    /* mute ADCs */
    cscodec_write(ADCCTL, ADCCTL_ADCAMUTE | ADCCTL_ADCBMUTE);

    /* from cs42l55 DS:
     *  PWRCTL1[3]: ADC charge pump, for optimal ADC performance
     *  and power consumption, set to ‘1’b when VA > 2.1 V and
     *  set to ‘0’b when VA < 2.1 V.
     */
    if (source_mic)
    {
        #ifdef MONO_MIC
        /* power-up CODEC, ADCA and ADC pump */
        cscodec_write(PWRCTL1, PWRCTL1_PDN_ADCB);
        #else
        /* power-up CODEC, ADCA, ADCB and ADC pump */
        cscodec_write(PWRCTL1, 0);
        #endif

        #ifdef BYPASS_PGA
        /* select PGA as input */
        cscodec_setbits(ALHMUX, ALHMUX_ADCAMUX_MASK | ALHMUX_ADCBMUX_MASK,
                            ALHMUX_ADCAMUX_PGAA | ALHMUX_ADCBMUX_PGAB);
        #endif

        /* configure PGA: select AIN2 and set gain */
        cscodec_write(PGAACTL, PGAACTL_MUX_AIN2A |
                            ((PGA_GAIN_DB << 1) & PGAACTL_VOLUME_MASK));
        #ifndef MONO_MIC
        cscodec_write(PGABCTL, PGABCTL_MUX_AIN2B |
                            ((PGA_GAIN_DB << 1) & PGABCTL_VOLUME_MASK));
        #endif
    }
    else /* line-in */
    {
        /* power-up CODEC, ADCA, ADCB and ADC pump */
        cscodec_write(PWRCTL1, 0);

        #ifdef BYPASS_PGA
        /* selects AIN1 as input (PGA is bypassed) */
        cscodec_setbits(ALHMUX, ALHMUX_ADCAMUX_MASK | ALHMUX_ADCBMUX_MASK,
                            ALHMUX_ADCAMUX_AIN1A | ALHMUX_ADCBMUX_AIN1B);
        #else
        /* configure PGA: select AIN1 and set gain to 0dB */
        cscodec_write(PGAACTL, PGAACTL_MUX_AIN1A);
        cscodec_write(PGABCTL, PGABCTL_MUX_AIN1B);
        #endif
    }

    cscodec_write(ADCCTL, 0);   /* unmute ADCs */
}

void audiohw_disable_recording(void)
{
    /* reset used registers to default values */
    cscodec_write(PGAACTL, 0);
    cscodec_write(PGABCTL, 0);
#ifdef BYPASS_PGA
    cscodec_setbits(ALHMUX, ALHMUX_ADCAMUX_MASK | ALHMUX_ADCBMUX_MASK,
                        ALHMUX_ADCAMUX_PGAA | ALHMUX_ADCBMUX_PGAB);
#endif
    /* power-down ADC section */
    cscodec_write(PWRCTL1, PWRCTL1_PDN_CHRG |
                        PWRCTL1_PDN_ADCA | PWRCTL1_PDN_ADCB);
}

void audiohw_set_recvol(int left, int right, int type)
{
#ifndef MONO_MIC
    (void) type;
#else
    if (type == AUDIO_GAIN_LINEIN)
#endif
    cscodec_write(ADCBATT, right);
    cscodec_write(ADCAATT, left);
}

void audiohw_set_monitor(bool enable)
{
    if (enable)
    {
        /* enable DSP power if it is actually disabled */
        handle_dsp_power(DSP_MODULE_MONITOR, 1);
        /* unmute ADC mixer */
        cscodec_write(AMIXACTL, 0);
        cscodec_write(AMIXBCTL, 0);
    }
    else
    {
        /* mute ADC mixer */
        cscodec_write(AMIXACTL, AMIXACTL_AMIXAMUTE);
        cscodec_write(AMIXBCTL, AMIXBCTL_AMIXBMUTE);
        /* disable DSP power if it is not being used by other modules */
        handle_dsp_power(DSP_MODULE_MONITOR, 0);
    }
}
#endif /* HAVE_RECORDING */
