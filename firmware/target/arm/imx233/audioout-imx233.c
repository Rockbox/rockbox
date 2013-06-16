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
#include "audioout-imx233.h"
#include "clkctrl-imx233.h"
#include "rtc-imx233.h"
#include "pcm_sampr.h"

static int hp_vol_l, hp_vol_r;
static bool input_line1;
static struct timeout hp_unmute_oneshort;

static int hp_unmute_cb(struct timeout *tmo)
{
    (void) tmo;
    /* unmute HP */
    BF_CLR(AUDIOOUT_HPVOL, MUTE);
    return 0;
}

void imx233_audioout_preinit(void)
{
    /* Enable AUDIOOUT block */
    imx233_reset_block(&HW_AUDIOOUT_CTRL);
    /* Enable digital filter clock */
    imx233_clkctrl_enable(CLK_FILT, true);
    /* Enable DAC */
    BF_CLR(AUDIOOUT_ANACLKCTRL, CLKGATE);
    /* Set capless mode */
    BF_CLR(AUDIOOUT_PWRDN, CAPLESS);
    /* Set word-length to 16-bit */
    BF_SET(AUDIOOUT_CTRL, WORD_LENGTH);
    /* Power up DAC */
    BF_CLR(AUDIOOUT_PWRDN, DAC);
    /* Hold HP to ground to avoid pop, then release and power up HP */
    BF_SET(AUDIOOUT_ANACTRL, HP_HOLD_GND);
    BF_CLR(AUDIOOUT_PWRDN, HEADPHONE);
    /* Set HP mode to AB */
    BF_SET(AUDIOOUT_ANACTRL, HP_CLASSAB);
    /* change bias to -50% */
    BF_WR(AUDIOOUT_TEST, HP_I1_ADJ, 1);
    BF_WR(AUDIOOUT_REFCTRL, BIAS_CTRL, 1);
    BF_SET(AUDIOOUT_REFCTRL, RAISE_REF);
    BF_SET(AUDIOOUT_REFCTRL, XTAL_BGR_BIAS);
    /* Stop holding to ground */
    BF_CLR(AUDIOOUT_ANACTRL, HP_HOLD_GND);
    /* Set dmawait count to 31 (see errata, workaround random stop) */
    BF_WR(AUDIOOUT_CTRL, DMAWAIT_COUNT, 31);
    /* start converting audio */
    BF_SET(AUDIOOUT_CTRL, RUN);
    /* unmute DAC */
    HW_AUDIOOUT_DACVOLUME_CLR = BM_OR2(AUDIOOUT_DACVOLUME, MUTE_LEFT, MUTE_RIGHT);
    /* send a few samples to avoid pop */
    HW_AUDIOOUT_DATA = 0;
    HW_AUDIOOUT_DATA = 0;
    HW_AUDIOOUT_DATA = 0;
    HW_AUDIOOUT_DATA = 0;
    /* wait for everything to stabilize before unmuting */
    timeout_register(&hp_unmute_oneshort, hp_unmute_cb, HZ / 2, 0);
}

void imx233_audioout_postinit(void)
{
}

void imx233_audioout_close(void)
{
    /* Switch to class A */
    BF_CLR(AUDIOOUT_ANACTRL, HP_CLASSAB);
    /* Hold HP to ground */
    BF_SET(AUDIOOUT_ANACTRL, HP_HOLD_GND);
    /* Mute HP and power down */
    BF_SET(AUDIOOUT_HPVOL, MUTE);
    /* Power down HP */
    BF_SET(AUDIOOUT_PWRDN, HEADPHONE);
    /* Mute DAC */
    HW_AUDIOOUT_DACVOLUME_SET = BM_OR2(AUDIOOUT_DACVOLUME, MUTE_LEFT, MUTE_RIGHT);
    /* Power down DAC */
    BF_SET(AUDIOOUT_PWRDN, DAC);
    /* Gate off DAC */
    BF_SET(AUDIOOUT_ANACLKCTRL, CLKGATE);
    /* Disable digital filter clock */
    imx233_clkctrl_enable(CLK_FILT, false);
    /* will also gate off the module */
    BF_CLR(AUDIOOUT_CTRL, RUN);
}

/* volume in half dB
 * don't check input values */
static void set_dac_vol(int vol_l, int vol_r)
{
    /* minimum is -100dB and max is 0dB */
    vol_l = MAX(-200, MIN(vol_l, 0));
    vol_r = MAX(-200, MIN(vol_r, 0));
    /* unmute, enable zero cross and set volume.
     * 0xff is 0dB */
    HW_AUDIOOUT_DACVOLUME = BF_OR3(AUDIOOUT_DACVOLUME,
        VOLUME_LEFT(0xff + vol_l), VOLUME_RIGHT(0xff + vol_r), EN_ZCD(1));
}

/* volume in half dB
 * don't check input values */
static void set_hp_vol(int vol_l, int vol_r)
{
    /* minimum is -57.5dB and max is 6dB in DAC mode
     * and -51.5dB / 12dB in Line1 mode */
    int min = input_line1 ? -103 : -115;
    int max = input_line1 ? 24 : 12;

    vol_l = MAX(min, MIN(vol_l, max));
    vol_r = MAX(min, MIN(vol_r, max));
    /* unmute, enable zero cross and set volume.*/
    unsigned mstr_zcd = BM_AUDIOOUT_HPVOL_EN_MSTR_ZCD;
    HW_AUDIOOUT_HPVOL = mstr_zcd | BF_OR3(AUDIOOUT_HPVOL, SELECT(input_line1),
        VOL_LEFT(max - vol_l), VOL_RIGHT(max - vol_r));
}

static void apply_volume(void)
{
    /* Two cases: line1 and dac */
    if(input_line1)
    {
        /* In line1 mode, the HP is the only way to adjust the volume */
        set_hp_vol(hp_vol_l, hp_vol_r);
    }
    else
    {
        /* In DAC mode we can use both the HP and the DAC volume.
         * Use the DAC for volume <0 and HP for volume >0 */
        set_dac_vol(MIN(0, hp_vol_l), MIN(0, hp_vol_r));
        set_hp_vol(MAX(0, hp_vol_l), MAX(0, hp_vol_r));
    }
}

void imx233_audioout_set_hp_vol(int vol_l, int vol_r)
{
    hp_vol_l = vol_l;
    hp_vol_r = vol_r;
    apply_volume();
}

void imx233_audioout_set_freq(int fsel)
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

    HW_AUDIOOUT_DACSRR = BF_OR4(AUDIOOUT_DACSRR,
        SRC_FRAC(dacssr[fsel].src_frac), SRC_INT(dacssr[fsel].src_int),
        SRC_HOLD(dacssr[fsel].src_hold), BASEMULT(dacssr[fsel].base_mult));

    #if 0
    /* Select base_mult and src_hold depending on the audio range:
     *     0 < f <= 12000   --> base_mult = 1, src_hold = 3 (div by 4)
     * 12000 < f <= 24000   --> base_mult = 1, src_hold = 1 (div by 2)
     * 24000 < f <= 48000   --> base_mult = 1, src_hold = 0 (div by 1)
     * 48000 < f <= 96000   --> base_mult = 2, src_hold = 0 (mul by 2)
     * 96000 < f <= .....   --> base_mult = 4, src_hold = 0 (mul by 4) */
    int base_mult = 1;
    int src_hold = 0;
    if(f > 96000)
        base_mult = 4;
    else if(f > 48000)
        base_mult = 2;
    else if(f <= 12000)
        src_hold = 3;
    else if(f <= 24000)
        src_hold = 1;
    /* compute the divisor (a tricky to keep to do it with 32-bit words only) */
    int src_int = (750000 * base_mult) / (f * (src_hold + 1));
    int src_frac = ((750000 * base_mult - src_int * f * (src_hold + 1)) << 13) / (f * (src_hold + 1));

    HW_AUDIOOUT_DACSRR = BF_OR4(AUDIOOUT_DACSRR,
        SRC_FRAC(src_frac), SRC_INT(src_int),
        SRC_HOLD(src_hold), BASEMULT(base_mult));
    #endif
}

/* select between DAC and Line1 */
void imx233_audioout_select_hp_input(bool line1)
{
    input_line1 = line1;
    /* reapply volume setting */
    apply_volume();
}
