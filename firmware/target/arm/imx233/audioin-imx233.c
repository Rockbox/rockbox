/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "audioin-imx233.h"
#include "pcm_sampr.h"
#include "string.h"

#include "regs/audioin.h"
/* some audioout registers impact audioin */
#include "regs/audioout.h"

#include "audio-target.h"

#ifndef IMX233_AUDIO_MIC_SELECT
#define IMX233_AUDIO_MIC_SELECT 1 /* lradc1 */
#endif

#ifndef IMX233_AUDIO_MIC_BIAS
#define IMX233_AUDIO_MIC_BIAS   0 /* 1.21V */
#endif

#ifndef IMX233_AUDIO_MIC_RESISTOR
#define IMX233_AUDIO_MIC_RESISTOR   2KOhm
#endif

/* values in half-dB, one for each setting */
static int audioin_vol[2][4]; /* 0=left, 1=right */
static int audioin_select[2]; /* idem */

void imx233_audioin_preinit(void)
{
    /* Enable AUDIOIN block */
    imx233_reset_block(&HW_AUDIOIN_CTRL);
    /* Set word-length to 16-bit */
    BF_SET(AUDIOIN_CTRL, WORD_LENGTH);
    /* Gate Off */
    BF_SET(AUDIOIN_CTRL, CLKGATE);
}

void imx233_audioin_postinit(void)
{
}

void imx233_audioin_open(void)
{
    /* Gate On */
    BF_CLR(AUDIOIN_CTRL, CLKGATE);
    /* Enable ADC clock */
    BF_CLR(AUDIOIN_ANACLKCTRL, CLKGATE);
    /* Power up ADC (WARNING audioout register) */
    BF_CLR(AUDIOOUT_PWRDN, ADC);
    /* Start ADC */
    BF_SET(AUDIOIN_CTRL, RUN);
}

void imx233_audioin_close(void)
{
    /* Stop ADC (doc says it gates off the module but that's not the case) */
    BF_CLR(AUDIOIN_CTRL, RUN);
    /* Disable ADC clock */
    BF_SET(AUDIOIN_ANACLKCTRL, CLKGATE);
    /* Power down ADC (WARNING audioout register) */
    BF_SET(AUDIOOUT_PWRDN, ADC);
    /* Gate Off */
    BF_SET(AUDIOIN_CTRL, CLKGATE);
}

static void apply_config(void)
{
    int select_l = audioin_select[0];
    int select_r = audioin_select[1];
    int vol_l = audioin_vol[0][select_l];
    int vol_r = audioin_vol[1][select_r];
    /* Depending on the input, we have three available volumes to tweak:
     * - adc volume: -100dB -> -0.5dB in 0.5dB steps
     * - mux gain: 0dB -> 22.5dB in 1.5dB steps
     * - mic gain: 0dB -> 40dB in 10dB steps (except for 10)
     *
     * This means two available volume ranges:
     * - line1/line2/hp: -100dB -> 22dB in 0.5dB steps
     * - microphone: -100dB -> 62dB in 0.5dB steps
     */

    /* First apply mic gain if possible and necessary
     * Only left volume is relevant with microphone
     * If gain is > 22dB, use mic gain */
    if(select_l == AUDIOIN_SELECT_MICROPHONE && vol_l > 22 * 2)
    {
        /* take lowest microphone gain to get back into the -100..22 range
         * achievable with mux+adc.*/

        /* from 52.5 dB and beyond: 40dB gain */
        if(vol_l > 52 * 2)
        {
            BF_WR(AUDIOIN_MICLINE, MIC_GAIN_V(40dB));
            vol_l -= 40 * 2;
        }
        /* from 42.5 dB to 52dB: 30dB gain */
        else if(vol_l > 42 * 2)
        {
            BF_WR(AUDIOIN_MICLINE, MIC_GAIN_V(30dB));
            vol_l -= 30 * 2;
        }
        /* from 22.5 dB to 42dB: 20dB gain */
        else if(vol_l > 22 * 2)
        {
            BF_WR(AUDIOIN_MICLINE, MIC_GAIN_V(20dB));
            vol_l -= 20 * 2;
        }
        /* otherwise 0dB gain */
        else
            BF_WR(AUDIOIN_MICLINE, MIC_GAIN_V(0dB));
    }
    /* max is 22dB */
    vol_l = MIN(vol_l, 44);
    vol_r = MIN(vol_r, 44);
    /* we use the mux volume to reach the volume or higher with 1.5dB steps
     * and then we use the ADC to go below 0dB or to obtain 0.5dB accuracy */

    int mux_vol_l = MAX(0, (vol_l + 2) / 3); /* 1.5dB = 3 * 0.5dB */
    int mux_vol_r = MAX(0, (vol_r + 2) / 3);
#if IMX233_SUBTARGET >= 3700
    unsigned adc_zcd = BM_AUDIOIN_ADCVOL_EN_ADC_ZCD;
#else
    unsigned adc_zcd = 0;
#endif
    HW_AUDIOIN_ADCVOL = adc_zcd | BF_OR(AUDIOIN_ADCVOL, SELECT_LEFT(select_l),
        SELECT_RIGHT(select_r), GAIN_LEFT(mux_vol_l), GAIN_RIGHT(mux_vol_r));

    vol_l -= mux_vol_l * 3; /* mux vol is in 1.5dB = 3 * 0.5dB steps */
    vol_r -= mux_vol_l * 3;
    vol_l = MIN(MAX(-200, vol_l), -1);
    vol_r = MIN(MAX(-200, vol_r), -1);

    /* unmute, enable zero cross and set volume.
     * 0xfe is -0.5dB */
    BF_WR_ALL(AUDIOIN_ADCVOLUME, EN_ZCD(1),
        VOLUME_LEFT(0xff + vol_l), VOLUME_RIGHT(0xff + vol_r));
}

void imx233_audioin_select_mux_input(bool right, int select)
{
    audioin_select[right] = select;
    apply_config();
}

void imx233_audioin_set_vol(bool right, int vol, int select)
{
    audioin_vol[right][select] = vol;
    apply_config();
}

void imx233_audioin_enable_mic(bool enable)
{
    if(enable)
    {
        BF_WR(AUDIOIN_MICLINE, MIC_RESISTOR_V(IMX233_AUDIO_MIC_RESISTOR));
        BF_WR(AUDIOIN_MICLINE, MIC_BIAS(IMX233_AUDIO_MIC_BIAS));
        BF_WR(AUDIOIN_MICLINE, MIC_SELECT(IMX233_AUDIO_MIC_SELECT));
    }
    else
        BF_WR(AUDIOIN_MICLINE, MIC_RESISTOR_V(Off));
}

void imx233_audioin_set_freq(int fsel)
{
    static struct
    {
        int base_mult;
        int src_hold;
        int src_int;
        int src_frac;
    }dacssr[HW_NUM_FREQ] =
    {
        HW_HAVE_8_([HW_FREQ_8] = { 0x1, 0x3, 0x17, 0xe00 }  ,)
        HW_HAVE_11_([HW_FREQ_11] = { 0x1, 0x3, 0x11, 0x37 } ,)
        HW_HAVE_12_([HW_FREQ_12] = { 0x1, 0x3, 0xf, 0x13ff },)
        HW_HAVE_16_([HW_FREQ_16] = { 0x1, 0x1, 0x17, 0xe00},)
        HW_HAVE_22_([HW_FREQ_22] = { 0x1, 0x1, 0x11, 0x37 },)
        HW_HAVE_24_([HW_FREQ_24] = { 0x1, 0x1, 0xf, 0x13ff },)
        HW_HAVE_32_([HW_FREQ_32] = { 0x1, 0x0, 0x17, 0xe00},)
        HW_HAVE_44_([HW_FREQ_44] = { 0x1, 0x0, 0x11, 0x37 },)
        HW_HAVE_48_([HW_FREQ_48] = { 0x1, 0x0, 0xf, 0x13ff },)
        HW_HAVE_64_([HW_FREQ_64] = { 0x2, 0x0, 0x17, 0xe00},)
        HW_HAVE_88_([HW_FREQ_88] = { 0x2, 0x0, 0x11, 0x37 },)
        HW_HAVE_96_([HW_FREQ_96] = { 0x2, 0x0, 0xf, 0x13ff },)
    };

    BF_WR_ALL(AUDIOIN_ADCSRR,
        SRC_FRAC(dacssr[fsel].src_frac), SRC_INT(dacssr[fsel].src_int),
        SRC_HOLD(dacssr[fsel].src_hold), BASEMULT(dacssr[fsel].base_mult));
}

struct imx233_audioin_info_t imx233_audioin_get_info(void)
{
    struct imx233_audioin_info_t info;
    memset(&info, 0, sizeof(info));
    /* 6*10^6*basemult/(src_frac*8*(src_hold+1)) in Hz */
    info.freq = 60000000 * BF_RD(AUDIOIN_ADCSRR, BASEMULT) / 8 /
        BF_RD(AUDIOIN_ADCSRR, SRC_FRAC) / (1 + BF_RD(AUDIOIN_ADCSRR, SRC_HOLD));
    info.muxselect[0] = BF_RD(AUDIOIN_ADCVOL, SELECT_LEFT);
    info.muxselect[1] = BF_RD(AUDIOIN_ADCVOL, SELECT_RIGHT);
    /* convert half-dB to tenth-dB */
    info.muxvol[0] = BF_RD(AUDIOIN_ADCVOL, GAIN_LEFT) * 15;
    info.muxvol[1] = BF_RD(AUDIOIN_ADCVOL, GAIN_RIGHT) * 15;
    info.muxmute[0] = info.adcmute[1] = BF_RD(AUDIOIN_ADCVOL, MUTE);
    info.adcvol[0] = MAX((int)BF_RD(AUDIOIN_ADCVOLUME, VOLUME_LEFT) - 0xff, -100) * 5;
    info.adcvol[1] = MAX((int)BF_RD(AUDIOIN_ADCVOLUME, VOLUME_RIGHT) - 0xff, -100) * 5;
    info.adcmute[0] = info.adcmute[1] = false;
    info.micvol[0] = BF_RD(AUDIOIN_MICLINE, MIC_GAIN);
    if(info.micvol[0] != 0)
        info.micvol[0] = info.micvol[1] = info.micvol[0] * 100 + 100;
    info.micmute[0] = info.micmute[1] = false;
    info.adc = !BF_RD(AUDIOOUT_PWRDN, ADC);
    info.mic = info.mux = true;
    return info;
}
