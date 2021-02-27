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
#include "system.h"
#include "ak4376.h"
#include "pcm_sampr.h"
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
    unsigned ak_mode_ctrl;
    unsigned ak_pll_d;
    unsigned ak_pll_m;
} ak_freq_settings[] = {
    {
        /* 192 KHz */
        .master_mult = 128,
        .ak_clock_mode = 0x60|AK4376_FS_192,
        .ak_mode_ctrl = 0,
        .ak_pll_d = 4, .ak_pll_m = 40,
    },
    {
        /* 176.4 KHz */
        .master_mult = 128,
        .ak_clock_mode = 0x60|AK4376_FS_176,
        .ak_mode_ctrl = 0,
        .ak_pll_d = 4, .ak_pll_m = 40,
    },
    {
        /* 96 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_96,
        .ak_mode_ctrl = 0,
        .ak_pll_d = 2, .ak_pll_m = 40,
    },
    {
        /* 88.2 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_88,
        .ak_mode_ctrl = 0,
        .ak_pll_d = 2, .ak_pll_m = 40,
    },
    {
        /* 64 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_64,
        .ak_mode_ctrl = 0,
        .ak_pll_d = 2, .ak_pll_m = 60,
    },
    {
        /* 48 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_48,
        .ak_mode_ctrl = 0,
        .ak_pll_d = 1, .ak_pll_m = 40,
    },
    {
        /* 44.1 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_44,
        .ak_mode_ctrl = 0,
        .ak_pll_d = 1, .ak_pll_m = 40,
    },
    {
        /* 32 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_32,
        .ak_mode_ctrl = 0,
        .ak_pll_d = 1, .ak_pll_m = 60,
    },
    {
        /* 24 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_24,
        .ak_mode_ctrl = 0,
        .ak_pll_d = 1, .ak_pll_m = 80,
    },
    {
        /* 22.05 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_22,
        .ak_mode_ctrl = 0,
        .ak_pll_d = 1, .ak_pll_m = 80,
    },
    {
        /* 16 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_16,
        .ak_mode_ctrl = 0,
        .ak_pll_d = 1, .ak_pll_m = 120,
    },
    {
        /* 12 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_12,
        .ak_mode_ctrl = 0x40,
        .ak_pll_d = 1, .ak_pll_m = 160,
    },
    {
        /* 11.025 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_11,
        .ak_mode_ctrl = 0x40,
        .ak_pll_d = 1, .ak_pll_m = 160,
    },
    {
        /* 8 KHz */
        .master_mult = 256,
        .ak_clock_mode = AK4376_FS_8,
        .ak_mode_ctrl = 0x40,
        .ak_pll_d = 1, .ak_pll_m = 240,
    },
};

void audiohw_init(void)
{
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

void audiohw_set_frequency(int fsel)
{
    const struct ak_freq_settings* fs = &ak_freq_settings[fsel];
    jz_writef(AIC_I2SCR, STPBK(1));
    ak4376_write(AK4376_REG_CLOCK_MODE, fs->ak_clock_mode);
    ak4376_write(AK4376_REG_MODE_CTRL, fs->ak_mode_ctrl);
    ak4376_write(AK4376_REG_PLL_REF_DIV1, (fs->ak_pll_d - 1) >> 8);
    ak4376_write(AK4376_REG_PLL_REF_DIV2, (fs->ak_pll_d - 1) & 0xff);
    ak4376_write(AK4376_REG_PLL_FB_DIV1, (fs->ak_pll_m - 1) >> 8);
    ak4376_write(AK4376_REG_PLL_FB_DIV2, (fs->ak_pll_m - 1) & 0xff);
    aic_i2s_set_mclk(AIC_CLKSRC_SCLK_A, hw_freq_sampr[fsel], fs->master_mult);
    jz_writef(AIC_I2SCR, STPBK(0));
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    /* TODO: add proper SW volume handling */
    ak4376_set_dig_volume(vol_l, vol_r);
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
