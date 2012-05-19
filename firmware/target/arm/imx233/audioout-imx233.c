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

void imx233_audioout_preinit(void)
{
    /* Enable AUDIOOUT block */
    imx233_reset_block(&HW_AUDIOOUT_CTRL);
    /* Enable digital filter clock */
    imx233_clkctrl_enable_xtal(XTAL_FILT, true);
    /* Enable DAC */
    __REG_CLR(HW_AUDIOOUT_ANACLKCTRL) = HW_AUDIOOUT_ANACLKCTRL__CLKGATE;
    /* Set capless mode */
    __REG_CLR(HW_AUDIOOUT_PWRDN) = HW_AUDIOOUT_PWRDN__CAPLESS;
    /* Set word-length to 16-bit */
    __REG_SET(HW_AUDIOOUT_CTRL) = HW_AUDIOOUT_CTRL__WORD_LENGTH;
    /* Power up DAC */
    __REG_CLR(HW_AUDIOOUT_PWRDN) = HW_AUDIOOUT_PWRDN__DAC;
    /* Hold HP to ground to avoid pop, then release and power up HP */
    __REG_SET(HW_AUDIOOUT_ANACTRL) = HW_AUDIOOUT_ANACTRL__HP_HOLD_GND;
    __REG_SET(HW_RTC_PERSISTENT0) = HW_RTC_PERSISTENT0__SPARE__RELEASE_GND;
    __REG_CLR(HW_AUDIOOUT_PWRDN) = HW_AUDIOOUT_PWRDN__HEADPHONE;
    /* Set HP mode to AB */
    __REG_SET(HW_AUDIOOUT_ANACTRL) = HW_AUDIOOUT_ANACTRL__HP_CLASSAB;
    /* Stop holding to ground */
    __REG_CLR(HW_AUDIOOUT_ANACTRL) = HW_AUDIOOUT_ANACTRL__HP_HOLD_GND;
    /* Set dmawait count to 31 (see errata, workaround random stop) */
    __REG_CLR(HW_AUDIOOUT_CTRL) = HW_AUDIOOUT_CTRL__DMAWAIT_COUNT_BM;
    __REG_SET(HW_AUDIOOUT_CTRL) = 31 << HW_AUDIOOUT_CTRL__DMAWAIT_COUNT_BP;
}

void imx233_audioout_postinit(void)
{
    /* start converting audio */
    __REG_SET(HW_AUDIOOUT_CTRL) = HW_AUDIOOUT_CTRL__RUN;
}

void imx233_audioout_close(void)
{
    /* Switch to class A */
    __REG_CLR(HW_AUDIOOUT_ANACTRL) = HW_AUDIOOUT_ANACTRL__HP_CLASSAB;
    /* Hold HP to ground */
    __REG_SET(HW_AUDIOOUT_ANACTRL) = HW_AUDIOOUT_ANACTRL__HP_HOLD_GND;
    /* Mute HP and power down */
    __REG_SET(HW_AUDIOOUT_HPVOL) = HW_AUDIOOUT_HPVOL__MUTE;
    /* Power down HP */
    __REG_SET(HW_AUDIOOUT_PWRDN) = HW_AUDIOOUT_PWRDN__HEADPHONE;
    /* Mute DAC */
    __REG_SET(HW_AUDIOOUT_DACVOLUME) = HW_AUDIOOUT_DACVOLUME__MUTE_LEFT
        | HW_AUDIOOUT_DACVOLUME__MUTE_RIGHT;
    /* Power down DAC */
    __REG_SET(HW_AUDIOOUT_PWRDN) = HW_AUDIOOUT_PWRDN__DAC;
    /* Gate off DAC */
    __REG_SET(HW_AUDIOOUT_ANACLKCTRL) = HW_AUDIOOUT_ANACLKCTRL__CLKGATE;
    /* Disable digital filter clock */
    imx233_clkctrl_enable_xtal(XTAL_FILT, false);
    /* will also gate off the module */
    __REG_CLR(HW_AUDIOOUT_CTRL) = HW_AUDIOOUT_CTRL__RUN;
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
    HW_AUDIOOUT_DACVOLUME =
        (0xff + vol_l) << HW_AUDIOOUT_DACVOLUME__VOLUME_LEFT_BP |
        (0xff + vol_r) << HW_AUDIOOUT_DACVOLUME__VOLUME_RIGHT_BP |
        HW_AUDIOOUT_DACVOLUME__EN_ZCD;
}

/* volume in half dB
 * don't check input values */
static void set_hp_vol(int vol_l, int vol_r)
{
    uint32_t select = (HW_AUDIOOUT_HPVOL & HW_AUDIOOUT_HPVOL__SELECT);
    /* minimum is -57.5dB and max is 6dB in DAC mode
     * and -51.5dB / 12dB in Line1 mode */
    int min = select ? -103 : -115;
    int max = select ? 24 : 12;

    vol_l = MAX(min, MIN(vol_l, max));
    vol_r = MAX(min, MIN(vol_r, max));
    /* unmute, enable zero cross and set volume. Keep select value. */
    HW_AUDIOOUT_HPVOL =
        (max - vol_l) << HW_AUDIOOUT_HPVOL__VOL_LEFT_BP |
        (max - vol_r) << HW_AUDIOOUT_HPVOL__VOL_RIGHT_BP |
        select |
        HW_AUDIOOUT_HPVOL__EN_MSTR_ZCD;
}

static void apply_volume(void)
{
    /* Two cases: line1 and dac */
    if(HW_AUDIOOUT_HPVOL & HW_AUDIOOUT_HPVOL__SELECT)
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

    HW_AUDIOOUT_DACSRR =
        dacssr[fsel].src_frac << HW_AUDIOOUT_DACSRR__SRC_FRAC_BP |
        dacssr[fsel].src_int << HW_AUDIOOUT_DACSRR__SRC_INT_BP |
        dacssr[fsel].src_hold << HW_AUDIOOUT_DACSRR__SRC_HOLD_BP |
        dacssr[fsel].base_mult << HW_AUDIOOUT_DACSRR__BASEMULT_BP;
    
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

    HW_AUDIOOUT_DACSRR =
        src_frac << HW_AUDIOOUT_DACSRR__SRC_FRAC_BP |
        src_int << HW_AUDIOOUT_DACSRR__SRC_INT_BP |
        src_hold << HW_AUDIOOUT_DACSRR__SRC_HOLD_BP |
        base_mult << HW_AUDIOOUT_DACSRR__BASEMULT_BP;
    #endif
}

/* select between DAC and Line1 */
void imx233_audioout_select_hp_input(bool line1)
{
    if(line1)
        __REG_SET(HW_AUDIOOUT_HPVOL) = HW_AUDIOOUT_HPVOL__SELECT;
    else
        __REG_CLR(HW_AUDIOOUT_HPVOL) = HW_AUDIOOUT_HPVOL__SELECT;
    /* reapply volume setting */
    apply_volume();
}
