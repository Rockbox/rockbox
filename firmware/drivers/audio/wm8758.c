/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for WM8758 audio codec - based on datasheet for WM8983
 *
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in December 2005
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
#include "system.h"
#include "string.h"
#include "audio.h"

#include "wmcodec.h"
#include "audiohw.h"

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB", 0,  1, -58,   6, -25},
    [SOUND_BASS]          = {"dB", 0,  1, -12,  12,   0},
    [SOUND_TREBLE]        = {"dB", 0,  1, -12,  12,   0},
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
#ifdef HAVE_RECORDING
    [SOUND_LEFT_GAIN]     = {"dB", 1,  1,   0,  63,  16},
    [SOUND_RIGHT_GAIN]    = {"dB", 1,  1,   0,  63,  16},
    [SOUND_MIC_GAIN]      = {"dB", 1,  1,   0,  63,  16},
#endif
    [SOUND_BASS_CUTOFF]   = {"",   0,  1,   1,   4,   1},
    [SOUND_TREBLE_CUTOFF] = {"",   0,  1,   1,   4,   1},
};

/* shadow registers */
static unsigned short eq1_reg = EQ1_EQ3DMODE | EQ_GAIN_VALUE(0);
static unsigned short eq5_reg = EQ_GAIN_VALUE(0);

/* convert tenth of dB volume (-57..6) to master volume register value */
int tenthdb2master(int db)
{
    if (db < VOLUME_MIN) {
        return 0x40;
    } else {
        return (db/10)+57;
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
    case SOUND_MIC_GAIN:
        result = ((value - 16) * 15) / 2;
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
        wmcodec_write(DACCTRL, DACCTRL_SOFTMUTE);
    } else {
        wmcodec_write(DACCTRL, 0);
    }
}

void audiohw_preinit(void)
{
    wmcodec_write(RESET, RESET_RESET);

    wmcodec_write(PWRMGMT1, PWRMGMT1_PLLEN | PWRMGMT1_BIASEN
                          | PWRMGMT1_VMIDSEL_5K);
    wmcodec_write(PWRMGMT2, PWRMGMT2_ROUT1EN | PWRMGMT2_LOUT1EN);
    wmcodec_write(PWRMGMT3, PWRMGMT3_LOUT2EN | PWRMGMT3_ROUT2EN
                          | PWRMGMT3_RMIXEN | PWRMGMT3_LMIXEN
                          | PWRMGMT3_DACENR | PWRMGMT3_DACENL);
                          
    wmcodec_write(AINTFCE, AINTFCE_IWL_16BIT | AINTFCE_FORMAT_I2S);
    wmcodec_write(OUTCTRL, OUTCTRL_VROI);
    wmcodec_write(CLKCTRL, CLKCTRL_MS); /* WM8758 is clock master */

    audiohw_set_frequency(HW_FREQ_44);
    
    wmcodec_write(LOUTMIX, LOUTMIX_DACL2LMIX);
    wmcodec_write(ROUTMIX, ROUTMIX_DACR2RMIX);
}

void audiohw_postinit(void)
{
    wmcodec_write(PWRMGMT1, PWRMGMT1_PLLEN | PWRMGMT1_BIASEN
                          | PWRMGMT1_VMIDSEL_75K); 
                          /* lower the VMID power consumption */
    audiohw_mute(false);
}

void audiohw_set_master_vol(int vol_l, int vol_r)
{
    /* OUT1 */
    wmcodec_write(LOUT1VOL, LOUT1VOL_LOUT1ZC | vol_l);
    wmcodec_write(ROUT1VOL, ROUT1VOL_OUT1VU | ROUT1VOL_ROUT1ZC | vol_r);
}

void audiohw_set_lineout_vol(int vol_l, int vol_r)
{
    /* OUT2 */
    wmcodec_write(LOUT2VOL, LOUT2VOL_LOUT2ZC | vol_l);
    wmcodec_write(ROUT2VOL, ROUT2VOL_OUT2VU | ROUT2VOL_ROUT2ZC | vol_r);
}

void audiohw_set_bass(int value)
{
    eq1_reg = (eq1_reg & ~EQ_GAIN_MASK) | EQ_GAIN_VALUE(value);
    wmcodec_write(EQ1, eq1_reg);
}

void audiohw_set_bass_cutoff(int value)
{
    eq1_reg = (eq1_reg & ~EQ_CUTOFF_MASK) | EQ_CUTOFF_VALUE(value);
    wmcodec_write(EQ1, eq1_reg);
}

void audiohw_set_treble(int value)
{
    eq5_reg = (eq5_reg & ~EQ_GAIN_MASK) | EQ_GAIN_VALUE(value);
    wmcodec_write(EQ5, eq5_reg); 
}

void audiohw_set_treble_cutoff(int value)
{
    eq5_reg = (eq5_reg & ~EQ_CUTOFF_MASK) | EQ_CUTOFF_VALUE(value);
    wmcodec_write(EQ5, eq5_reg); 
}

/* Nice shutdown of WM8758 codec */
void audiohw_close(void)
{
    audiohw_mute(true);

    wmcodec_write(PWRMGMT3, 0);
    wmcodec_write(PWRMGMT1, 0);
    wmcodec_write(PWRMGMT2, PWRMGMT2_SLEEP);
}

/* Note: Disable output before calling this function */
void audiohw_set_frequency(int fsel)
{
    /**** We force 44.1KHz for now. ****/
    (void)fsel;

    /* setup PLL for MHZ=11.2896 */
    wmcodec_write(PLLN, PLLN_PLLPRESCALE | 0x7);
    wmcodec_write(PLLK1, 0x21);
    wmcodec_write(PLLK2, 0x161);
    wmcodec_write(PLLK3, 0x26);

    /* set clock div */
    wmcodec_write(CLKCTRL, CLKCTRL_CLKSEL | CLKCTRL_MCLKDIV_2
                         | CLKCTRL_BCLKDIV_2 | CLKCTRL_MS);

    wmcodec_write(ADDCTRL, ADDCTRL_SR_48kHz | ADDCTRL_SLOWCLKEN);
    /* SLOWCLK enabled for zero cross timeout to work */
}

void audiohw_enable_recording(bool source_mic)
{
    (void)source_mic; /* We only have a line-in (I think) */
    
    wmcodec_write(PWRMGMT2, PWRMGMT2_ROUT1EN | PWRMGMT2_LOUT1EN
                          | PWRMGMT2_INPGAENR | PWRMGMT2_INPGAENL
                          | PWRMGMT2_ADCENR | PWRMGMT2_ADCENL);

    wmcodec_write(INCTRL, INCTRL_R2_2INPGA | INCTRL_L2_2INPGA);

    wmcodec_write(LADCBOOST, LADCBOOST_L2_2BOOST(5));
    wmcodec_write(RADCBOOST, RADCBOOST_R2_2BOOST(5));

    /* Enable monitoring */
    wmcodec_write(LOUTMIX, LOUTMIX_BYP2LMIXVOL(5)
                         | LOUTMIX_BYPL2LMIX | LOUTMIX_DACL2LMIX);
    wmcodec_write(ROUTMIX, ROUTMIX_BYP2RMIXVOL(5)
                         | ROUTMIX_BYPR2RMIX | ROUTMIX_DACR2RMIX);
}

void audiohw_disable_recording(void) 
{
    wmcodec_write(LOUTMIX, LOUTMIX_DACL2LMIX);
    wmcodec_write(ROUTMIX, ROUTMIX_DACR2RMIX);

    wmcodec_write(PWRMGMT2, PWRMGMT2_ROUT1EN | PWRMGMT2_LOUT1EN);
}

void audiohw_set_recvol(int left, int right, int type)
{
    switch (type)
    {
    case AUDIO_GAIN_MIC:
        right = left;
        /* fall through */
    case AUDIO_GAIN_LINEIN:
        wmcodec_write(LINPGAVOL, LINPGAVOL_INPGAZCL
                               | (left & LINPGAVOL_INPGAVOL_MASK));
        wmcodec_write(RINPGAVOL, RINPGAVOL_INPGAVU | RINPGAVOL_INPGAZCR
                               | (right & RINPGAVOL_INPGAVOL_MASK));
        break;
    default:
        return;
    }
}

void audiohw_set_monitor(bool enable) 
{
    (void)enable;
}

