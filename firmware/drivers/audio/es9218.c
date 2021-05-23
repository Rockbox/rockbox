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
#include "i2c-async.h"

struct es9218_state {
    enum es9218_clock_gear clk_gear;
    uint32_t fsr;
    uint32_t nco;
};

static struct es9218_state es9218;

void es9218_open(void)
{
    /* Enable power supply */
    es9218_set_power_pin(1);

    /* "Wiggle" reset pin to get the internal oscillator to stabilize.
     * This should also work if using an external powered oscillator,
     * although in that case it's unnecessary to do this dance. */
    es9218_set_reset_pin(1);
    udelay(75);
    es9218_set_reset_pin(0);
    udelay(50);
    es9218_set_reset_pin(1);
    mdelay(2);

    /* Initialize driver state */
    es9218.clk_gear = ES9218_CLK_GEAR_1;
    es9218.fsr = 0;
    es9218.nco = 0;
}

void es9218_close(void)
{
    /* Turn off power supply */
    es9218_set_power_pin(0);
    es9218_set_reset_pin(0);
}

static void recalc_nco(void)
{
    /*        nco * CLK   *
     *  fsr = ---------   *
     *          2**32     */

    uint32_t clk = es9218_get_mclk();
    clk >>= (int)es9218.clk_gear;

    uint64_t nco64 = es9218.fsr;
    nco64 <<= 32;
    nco64 /= clk;

    /* let's just ignore overflow... */
    uint32_t nco = nco64;
    if(nco != es9218.nco) {
        es9218.nco = nco;

        /* registers must be written in this order */
        es9218_write(ES9218_REG_PROG_NCO_BIT0_7,   (nco >>  0) & 0xff);
        es9218_write(ES9218_REG_PROG_NCO_BIT8_15,  (nco >>  8) & 0xff);
        es9218_write(ES9218_REG_PROG_NCO_BIT16_23, (nco >> 16) & 0xff);
        es9218_write(ES9218_REG_PROG_NCO_BIT24_31, (nco >> 24) & 0xff);
    }
}

void es9218_set_clock_gear(enum es9218_clock_gear gear)
{
    if(gear != es9218.clk_gear) {
        es9218.clk_gear = gear;
        es9218_update(ES9218_REG_SYSTEM, 0x0c, (uint8_t)(gear & 3) << 2);
        recalc_nco();
    }
}

void es9218_set_nco_frequency(uint32_t fsr)
{
    if(fsr != es9218.fsr) {
        es9218.fsr = fsr;
        recalc_nco();
    }
}

void es9218_recompute_nco(void)
{
    recalc_nco();
}

void es9218_set_amp_mode(enum es9218_amp_mode mode)
{
    es9218_update(ES9218_REG_AMP_CONFIG, 0x03, (uint8_t)mode & 3);
}

void es9218_set_amp_powered(bool en)
{
    /* this doesn't seem to be necessary..? */
    es9218_update(ES9218_REG_ANALOG_CTRL, 0x40, en ? 0x40 : 0x00);
}

void es9218_set_iface_role(enum es9218_iface_role role)
{
    /* asrc is used to lock onto the incoming audio frequency and is
     * only used in aysnchronous slave mode. In synchronous operation,
     * including master mode, it can be disabled to save power. */
    int asrc_en = (role == ES9218_IFACE_ROLE_SLAVE ? 1 : 0);
    int master_mode = (role == ES9218_IFACE_ROLE_MASTER ? 1 : 0);

    es9218_update(ES9218_REG_MASTER_MODE_CONFIG, 1 << 7, master_mode << 7);
    es9218_update(ES9218_REG_GENERAL_CONFIG, 1 << 7, asrc_en << 7);
}

void es9218_set_iface_format(enum es9218_iface_format fmt,
                             enum es9218_iface_bits bits)
{
    uint8_t val = 0;
    val |= ((uint8_t)bits & 3) << 6;
    val |= ((uint8_t)fmt & 3) << 4;
    /* keep low 4 bits zero -> use normal I2S mode, disable DSD mode */
    es9218_write(ES9218_REG_INPUT_SEL, val);
}

static int dig_vol_to_hw(int x)
{
    x = MIN(x, ES9218_DIG_VOLUME_MAX);
    x = MAX(x, ES9218_DIG_VOLUME_MIN);
    return 0xff - (x - ES9218_DIG_VOLUME_MIN) / ES9218_DIG_VOLUME_STEP;
}

static int amp_vol_to_hw(int x)
{
    x = MIN(x, ES9218_AMP_VOLUME_MAX);
    x = MAX(x, ES9218_AMP_VOLUME_MIN);
    return 24 - (x - ES9218_AMP_VOLUME_MIN) / ES9218_AMP_VOLUME_STEP;
}

void es9218_set_dig_volume(int vol_l, int vol_r)
{
    es9218_write(ES9218_REG_VOLUME_LEFT,  dig_vol_to_hw(vol_l));
    es9218_write(ES9218_REG_VOLUME_RIGHT, dig_vol_to_hw(vol_r));
}

void es9218_set_amp_volume(int vol)
{
    es9218_update(ES9218_REG_ANALOG_VOL, 0x1f,  amp_vol_to_hw(vol));
}

void es9218_mute(bool en)
{
    es9218_update(ES9218_REG_FILTER_SYS_MUTE, 1, en ? 1 : 0);
}

void es9218_set_filter(enum es9218_filter_type filt)
{
    es9218_update(ES9218_REG_FILTER_SYS_MUTE, 0xe0, ((int)filt & 7) << 5);
}

void es9218_set_automute_time(int time)
{
    if(time < 0)   time = 0;
    if(time > 255) time = 255;
    es9218_write(ES9218_REG_AUTOMUTE_TIME, time);
}

void es9218_set_automute_level(int dB)
{
    es9218_update(ES9218_REG_AUTOMUTE_LEVEL, 0x7f, dB);
}

void es9218_set_automute_fast_mode(bool en)
{
    es9218_update(ES9218_REG_MIX_AUTOMUTE, 0x10, en ? 0x10 : 0x00);
}

void es9218_set_dpll_bandwidth(int knob)
{
    es9218_update(ES9218_REG_ASRC_DPLL_BANDWIDTH, 0xf0, (knob & 0xf) << 4);
}

void es9218_set_thd_compensation(bool en)
{
    es9218_update(ES9218_REG_THD_COMP_BYPASS, 0x40, en ? 0x40 : 0);
}

void es9218_set_thd_coeffs(uint16_t c2, uint16_t c3)
{
    es9218_write(ES9218_REG_THD_COMP_C2_LO, c2 & 0xff);
    es9218_write(ES9218_REG_THD_COMP_C2_HI, (c2 >> 8) & 0xff);
    es9218_write(ES9218_REG_THD_COMP_C3_LO, c3 & 0xff);
    es9218_write(ES9218_REG_THD_COMP_C3_HI, (c3 >> 8) & 0xff);
}

int es9218_read(int reg)
{
    return i2c_reg_read1(ES9218_BUS, ES9218_ADDR, reg);
}

void es9218_write(int reg, uint8_t val)
{
    i2c_reg_write1(ES9218_BUS, ES9218_ADDR, reg, val);
}

void es9218_update(int reg, uint8_t msk, uint8_t val)
{
    i2c_reg_modify1(ES9218_BUS, ES9218_ADDR, reg, msk, val, NULL);
}
