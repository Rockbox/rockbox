/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#include "kernel.h"
#include "panic.h"
#include "aic310x.h"
#include "pcm_sw_volume.h"
#include "power-echoplayer.h"
#include "gpio-stm32h7.h"
#include "i2c-echoplayer.h"

static int tlv_read_multiple(uint8_t reg, uint8_t *data, size_t count)
{
    return stm32_i2c_read_mem(&i2c1_ctl, AIC310X_I2C_ADDR, reg, data, count);
}

static int tlv_write_multiple(uint8_t reg, const uint8_t *data, size_t count)
{
    return stm32_i2c_write_mem(&i2c1_ctl, AIC310X_I2C_ADDR, reg, data, count);
}

static struct aic310x tlv_codec = {
    .read_multiple = tlv_read_multiple,
    .write_multiple = tlv_write_multiple,
};

struct codec_reg_data
{
    uint8_t reg;
    uint8_t val;
};

static const struct codec_reg_data codec_init_seq[] = {
    /* Set output driver configuration to stereo psuedo-differential */
    { .reg = AIC310X_HEADSET_DETECT_B,          .val = 0x08 },

    /* Clock input is CODEC_CLKIN (bypass codec PLL) */
    { .reg = AIC310X_CLOCK,                     .val = 0x01 },

    /* Route left ch. -> left DAC, right ch -> right DAC */
    { .reg = AIC310X_DATA_PATH,                 .val = 0x0A },

    /* Power on DACs, set HPLCOM = VCM, set HPRCOM = VCM,
     * enable short circuit protection */
    { .reg = AIC310X_HPOUT_DRIVER_CTRL,         .val = 0x0C },
    { .reg = AIC310X_DAC_POWER_CTRL,            .val = 0xD0 },

    /* Unmute the DACs */
    { .reg = AIC310X_LEFT_DAC_DIGITAL_VOLUME,   .val = 0x00 },
    { .reg = AIC310X_RIGHT_DAC_DIGITAL_VOLUME,  .val = 0x00 },

    /* Route DACs to output drivers */
    { .reg = AIC310X_DAC_L1_TO_HPLOUT_VOLUME,   .val = 0x80 },
    { .reg = AIC310X_DAC_R1_TO_HPROUT_VOLUME,   .val = 0x80 },

    /* Route DACs to line out port */
    { .reg = AIC310X_DAC_L1_TO_LEFT_LO_VOLUME,  .val = 0x80 },
    { .reg = AIC310X_DAC_R1_TO_RIGHT_LO_VOLUME, .val = 0x80 },

    /* Power on headphone output drivers */
    { .reg = AIC310X_HPLCOM_LEVEL_CONTROL,      .val = 0x01 },
    { .reg = AIC310X_HPRCOM_LEVEL_CONTROL,      .val = 0x01 },
    { .reg = AIC310X_HPLOUT_LEVEL_CONTROL,      .val = 0x01 },
    { .reg = AIC310X_HPROUT_LEVEL_CONTROL,      .val = 0x01 },

    /* Power up line out drivers */
    { .reg = AIC310X_RIGHT_LO_LEVEL_CONTROL,    .val = 0x01 },
    { .reg = AIC310X_LEFT_LO_LEVEL_CONTROL,     .val = 0x01 },
};

static void write_aic310x_seq(const struct codec_reg_data *seq, size_t count)
{
    while (count > 0)
    {
        if (aic310x_write(&tlv_codec, seq->reg, seq->val))
            panicf("aic310x wr fail: %02x", (unsigned int)seq->reg);

        seq++;
        count--;
    }
}

static void wait_aic310x_active(void)
{
    long end_tick = current_tick + HZ;
    while (TIME_BEFORE(current_tick, end_tick))
    {
        uint8_t tmp;
        int err = tlv_read_multiple(AIC310X_PAGE_SELECT, &tmp, 1);
        if (err == 0)
            return;

        sleep(1);
    }

    panicf("aic310x init timeout");
}

void audiohw_init(void)
{
    /* initialize driver */
    aic310x_init(&tlv_codec);

    /* bring 1.8v regulator up */
    echoplayer_enable_1v8_regulator(true);

    /* apply AVDD/DRVDD and DVDD */
    gpio_set_level(GPIO_CODEC_AVDD_EN, 0);
    gpio_set_level(GPIO_CODEC_DVDD_EN, 0);

    /* delay for power stabilization */
    sleep(HZ/100);

    /* bring codec out of reset */
    gpio_set_level(GPIO_CODEC_RESET, 1);
}

void audiohw_postinit(void)
{
    wait_aic310x_active();
    write_aic310x_seq(codec_init_seq, ARRAYLEN(codec_init_seq));
}

void audiohw_close(void)
{
    /* apply reset */
    gpio_set_level(GPIO_CODEC_RESET, 0);

    /* remove power */
    gpio_set_level(GPIO_CODEC_AVDD_EN, 1);
    gpio_set_level(GPIO_CODEC_DVDD_EN, 1);

    /* disable regulator */
    echoplayer_enable_1v8_regulator(false);
}

void audiohw_mute(bool mute)
{
    uint8_t wr_val = (mute ? 0x01 : 0x09);

    /* Mute and unmute at the output drivers */
    aic310x_write(&tlv_codec, AIC310X_HPLOUT_LEVEL_CONTROL, wr_val);
    aic310x_write(&tlv_codec, AIC310X_HPROUT_LEVEL_CONTROL, wr_val);
    aic310x_write(&tlv_codec, AIC310X_LEFT_LO_LEVEL_CONTROL, wr_val);
    aic310x_write(&tlv_codec, AIC310X_RIGHT_LO_LEVEL_CONTROL, wr_val);
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    pcm_set_master_volume(vol_l, vol_r);
}
