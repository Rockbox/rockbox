/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "audiohw.h"
#include "sound.h"
#include "system.h"
#include "ak4376.h"
#include "pcm_sampr.h"
#include "pcm_sw_volume.h"
#include "aic-x1000.h"
#include "i2c-x1000.h"
#include "gpio-x1000.h"
#include "x1000/aic.h"
#include "x1000/cpm.h"

#define AK_CHN  0
#define AK_ADDR 0x10

#define AK_PDN_PORT GPIO_A
#define AK_PDN_PIN  (1 << 16)

static const struct ak_reg_setting {
    int reg;
    int val;
} ak_reg_initial_settings[] = {
    {AK4376_REG_PWR1,         0x01},
    {AK4376_REG_PWR2,         0x33},
    {AK4376_REG_PWR3,         0x01},
    {AK4376_REG_PWR4,         0x03},
    {AK4376_REG_OUTPUT_MODE,  0x00},
    {AK4376_REG_CLOCK_MODE,   AK4376_FS_48},
    {AK4376_REG_FILTER,       0x00},
    {AK4376_REG_MIXER,        0x21},
    {AK4376_REG_LCH_VOLUME,   0x80},
    {AK4376_REG_RCH_VOLUME,   0x00},
    {AK4376_REG_HP_VOLUME,    0x0b},
    {AK4376_REG_PLL_CLK_SRC,  0x01},
    {AK4376_REG_PLL_REF_DIV1, 0x00},
    {AK4376_REG_PLL_REF_DIV2, 0x00},
    {AK4376_REG_PLL_FB_DIV1,  0x00},
    {AK4376_REG_PLL_FB_DIV2,  39},
    {AK4376_REG_DAC_CLK_SRC,  0x01},
    {AK4376_REG_DAC_CLK_DIV,  0x09},
    {AK4376_REG_AUDIO_IF_FMT, 0xe2},
    {AK4376_REG_MODE_CTRL,    0x00},
    {-1, -1},
};

static const struct ak_freq_settings {
    unsigned master_mult;
    unsigned ak_clock_mode;
    unsigned ak_pll_d;
    unsigned ak_pll_m;
} ak_freq_settings[] = {
    {
        /* 192 KHz */
        .master_mult = 128,
        .ak_clock_mode = 0x60|AK4376_FS_192,
        .ak_pll_d = 4, .ak_pll_m = 40,
    },
    {
        /* 176.4 KHz */
        .master_mult = 128,
        .ak_clock_mode = 0x60|AK4376_FS_176,
        .ak_pll_d = 4, .ak_pll_m = 40,
    },
    {
        /* 96 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_96,
        .ak_pll_d = 2, .ak_pll_m = 40,
    },
    {
        /* 88.2 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_88,
        .ak_pll_d = 2, .ak_pll_m = 40,
    },
    {
        /* 64 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_64,
        .ak_pll_d = 2, .ak_pll_m = 60,
    },
    {
        /* 48 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_48,
        .ak_pll_d = 1, .ak_pll_m = 40,
    },
    {
        /* 44.1 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_44,
        .ak_pll_d = 1, .ak_pll_m = 40,
    },
    {
        /* 32 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_32,
        .ak_pll_d = 1, .ak_pll_m = 60,
    },
    {
        /* 24 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_24,
        .ak_pll_d = 1, .ak_pll_m = 80,
    },
    {
        /* 22.05 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_22,
        .ak_pll_d = 1, .ak_pll_m = 80,
    },
    {
        /* 16 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_16,
        .ak_pll_d = 1, .ak_pll_m = 120,
    },
    {
        /* 12 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_12,
        .ak_pll_d = 1, .ak_pll_m = 160,
    },
    {
        /* 11.025 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_11,
        .ak_pll_d = 1, .ak_pll_m = 160,
    },
    {
        /* 8 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_8,
        .ak_pll_d = 1, .ak_pll_m = 240,
    },
};

static int _vol_min, _vol_max;
static int _fsel = HW_FREQ_48;

void audiohw_init(void)
{
    /* Get the min/max volumes defined in the codec header.
     * Makes it easier to change the levels if needed.
     */
    _vol_min = sound_min(SOUND_VOLUME);
    _vol_max = sound_max(SOUND_VOLUME);

    /* DAC init. This is how the original firmware does it, but the DAC's
     * datasheet would have us go through an elaborate power-up sequence.
     * Since this seems to work I see no reason for added complication.
     */
    i2c_set_freq(AK_CHN, I2C_FREQ_400K);
    ak4376_power_on();
    ak4376_clear_reg_cache();
    for(int i = 0; ak_reg_initial_settings[i].reg != -1; ++i) {
        ak4376_write(ak_reg_initial_settings[i].reg,
                     ak_reg_initial_settings[i].val);
    }

    /* Configure AIC for I2S operation */
    jz_writef(CPM_CLKGR, AIC(0));
    gpio_config(GPIO_B, 0x1f, GPIO_DEVICE(1));
    jz_writef(AIC_I2SCR, STPBK(1));

    /* Operate as I2S master, use external codec */
    jz_writef(AIC_CFG, AUSEL(1), ICDC(0), BCKD(1), SYNCD(1), LSMP(1));
    jz_writef(AIC_I2SCR, ESCLK(1), AMSL(0));

    /* Stereo audio, packed 16 bit samples */
    jz_writef(AIC_CCR, PACK16(1), CHANNEL(1), OSS(1));

    /* Initial frequency setting */
    audiohw_set_frequency(HW_FREQ_48);
}

void audiohw_postinit(void)
{
}

void audiohw_close(void)
{
    /* Mute before power off, hopefully stops the popping noise
     * FIXME: the actual problem not following the right power-down sequence.
     */
    ak4376_set_dig_volume(AK4376_DIG_VOLUME_MUTE, AK4376_DIG_VOLUME_MUTE);
    ak4376_power_off();
    gpio_out_level(AK_PDN_PORT, AK_PDN_PIN, 0);
}

void audiohw_set_filter_roll_off(int value)
{
    /* TODO: Numbering scheme doesn't match Rockbox
     *
     * nr   rockbox     DAC
     * 0    sharp       sharp
     * 1    slow        slow
     * 2    short       short sharp
     * 3    bypass      short slow
     */
    ak4376_set_filter_roll_off(value);
}

/* TODO: expose this through audiohw API and add a Rockbox setting for it */
void audiohw_set_power_save(bool enable)
{
    /* Set LPMODE bit when power saving is enabled to enter "Low Mode" */
    int reg = ak4376_read(AK4376_REG_PWR3);
    if(enable)
        reg |= 0x10;
    else
        reg &= ~0x10;

    ak4376_write(AK4376_REG_PWR3, reg);

    /* Need to re-set frequency after changing power save mode */
    audiohw_set_frequency(_fsel);
}

void audiohw_set_frequency(int fsel)
{
    const struct ak_freq_settings* fs = &ak_freq_settings[fsel];

    /* Stop the bit clock */
    jz_writef(AIC_I2SCR, STPBK(1));

    /* Low mode requires a slight change to the config */
    bool low_mode = ak4376_read(AK4376_REG_PWR3) & 0x10 ? false : true;

    /* Set clock mode */
    ak4376_write(AK4376_REG_CLOCK_MODE, fs->ak_clock_mode);
    if(low_mode || hw_freq_sampr[fsel] < SAMPR_12)
        ak4376_write(AK4376_REG_MODE_CTRL, 0x40);
    else
        ak4376_write(AK4376_REG_MODE_CTRL, 0x00);

    /* Change PLL */
    ak4376_write(AK4376_REG_PLL_REF_DIV1, (fs->ak_pll_d - 1) >> 8);
    ak4376_write(AK4376_REG_PLL_REF_DIV2, (fs->ak_pll_d - 1) & 0xff);
    ak4376_write(AK4376_REG_PLL_FB_DIV1, (fs->ak_pll_m - 1) >> 8);
    ak4376_write(AK4376_REG_PLL_FB_DIV2, (fs->ak_pll_m - 1) & 0xff);

    /* Update master clock rate */
    aic_i2s_set_mclk(X1000_CLK_SCLK_A, hw_freq_sampr[fsel], fs->master_mult);

    /* Start bit clock */
    jz_writef(AIC_I2SCR, STPBK(0));

    /* Save new frequency */
    _fsel = fsel;
}

/* -12 to +3 dB in 0.5 dB steps (from DAC digital volume)
 * -20 to +6 dB in 2.0 dB steps (from DAC headphone amp)
 * -73 to +6 dB in 0.1 dB steps (from software volume)
 */
static void calc_volumes(int vol, int* amp, int* dig, int* sw)
{
#ifdef HAVE_SW_VOLUME_CONTROL
    const int SW_VOL_MAX = 60;
    const int SW_VOL_MIN = -730;
#else
    const int SW_VOL_MAX = 0;
    const int SW_VOL_MIN = 0;
#endif

    /* MUTE | SW_ATTEN | DIG_ATTEN | MAIN | DIG_BOOST | SW_BOOST | FULL
     *     ABS        DAC         AMP    AMP         DAC        ABS
     *     MIN        MIN         MIN    MAX         MAX        MAX
     */
    const int AMP_MIN = AK4376_AMP_VOLUME_MIN;
    const int AMP_MAX = AK4376_AMP_VOLUME_MAX;
    const int DAC_MIN = AMP_MIN + AK4376_DIG_VOLUME_MIN;
    const int DAC_MAX = AMP_MAX + AK4376_DIG_VOLUME_MAX;
    const int ABS_MIN = DAC_MIN + SW_VOL_MIN;
    const int ABS_MAX = DAC_MAX + SW_VOL_MAX;

    if(vol < _vol_min || vol < ABS_MIN) {
        /* Handle mute */
        *amp = AK4376_AMP_VOLUME_MUTE;
        *dig = AK4376_DIG_VOLUME_MUTE;
        *sw  = 0;
        return;
    }

    /* Clamp to maximum volume level */
    if(vol > MIN(ABS_MAX, _vol_max))
        vol = MIN(ABS_MAX, _vol_max);

    if(vol <= DAC_MIN) {
        /* Software attenuation */
        *amp = AK4376_AMP_VOLUME_MIN;
        *dig = AK4376_DIG_VOLUME_MIN;
        *sw  = vol - DAC_MIN;
    } else if(vol <= AMP_MIN) {
        /* DAC digital attenuation */
        *amp = AK4376_AMP_VOLUME_MIN;
        *dig = vol - AMP_MIN;
        *sw  = 0;
    } else if(vol <= AMP_MAX) {
        /* Within the headphone amp's range we round volume up to the nearest
         * amp volume and use DAC digital volume for fine-grained adjustment
         */
        int rem = vol % AK4376_AMP_VOLUME_STEP;
        if(rem > 0)
            rem -= AK4376_AMP_VOLUME_STEP;

        *amp = vol - rem;
        *dig = rem;
        *sw  = 0;
    } else if(vol <= DAC_MAX) {
        /* DAC digital boost */
        *amp = AK4376_AMP_VOLUME_MAX;
        *dig = vol - AMP_MAX;
        *sw  = 0;
    } else {
        /* Software boost */
        *amp = AK4376_AMP_VOLUME_MAX;
        *dig = AK4376_DIG_VOLUME_MAX;
        *sw  = vol - DAC_MAX;
    }
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    int amp_l, dig_l, sw_l;
    int amp_r, dig_r, sw_r;

    calc_volumes(vol_l, &amp_l, &dig_l, &sw_l);
    calc_volumes(vol_r, &amp_r, &dig_r, &sw_r);

    /* TODO: try harder to handle amp volume when vol_l != vol_r */
    /* TODO: also try using the mixer /2 divider mode to get more range */
    ak4376_set_dig_volume(dig_l, dig_r);
    ak4376_set_amp_volume((amp_l + amp_r) / 2);
#ifdef HAVE_SW_VOLUME_CONTROL
    pcm_set_master_volume(sw_l, sw_r);
#endif
}

void ak4376_power_on(void)
{
    gpio_out_level(AK_PDN_PORT, AK_PDN_PIN, 1);
    mdelay(1);
}

void ak4376_power_off(void)
{
    gpio_out_level(AK_PDN_PORT, AK_PDN_PIN, 0);
}

void ak4376_reg_write(int reg, int value)
{
    unsigned char buf[2] = { reg & 0xff, value & 0xff };
    i2c_lock(AK_CHN);
    i2c_write(AK_CHN, AK_ADDR, I2C_STOP, 2, &buf[0]);
    i2c_unlock(AK_CHN);
}

int ak4376_reg_read(int reg)
{
    unsigned char buf[2] = { reg & 0xff, 0 };
    int ret = -1;

    i2c_lock(AK_CHN);
    if(i2c_write(AK_CHN, AK_ADDR, I2C_CONTINUE, 1, &buf[0]))
        goto _exit;
    if(i2c_read(AK_CHN, AK_ADDR, I2C_RESTART|I2C_STOP, 1, &buf[1]))
        goto _exit;
    ret = buf[1];
  _exit:
    i2c_unlock(AK_CHN);
    return ret;
}
