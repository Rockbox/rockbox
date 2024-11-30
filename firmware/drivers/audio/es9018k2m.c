/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 *
 * Copyright (c) 2023 Dana Conrad
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
#include "es9018k2m.h"
#include "i2c-async.h"
#ifndef BOOTLOADER
# include "action.h"
#endif

//======================================================================================
// ES9018K2M support stuff

#ifndef ES9018K2M_BUS
# error "No definition for ES9018K2M I2C bus!"
#endif

#ifndef ES9018K2M_ADDR
# error "No definition for ES9018K2M I2C address!"
#endif

#ifndef BOOTLOADER
static int vol_tenthdb2hw(const int tdb)
{
    if (tdb < ES9018K2M_VOLUME_MIN) {
        return 0xff;
    } else if (tdb > ES9018K2M_VOLUME_MAX) {
        return 0x00;
    } else {
        return (-tdb/5);
    }
}

/* NOTE: This implementation is for the ES9018K2M specifically. */
/* Register defaults from the datasheet. */
/* These are basically just for reference. All defaults work out for us.
 * static uint8_t reg0_system_settings         = 0x00; // System settings. Default value of register 0
 * static uint8_t reg1_input_configuration     = 0x8C; // Input settings. I2S input, 32-bit
 * static uint8_t reg4_automute_time           = 0x00; // Automute time. Default = disabled
 * static uint8_t reg5_automute_level          = 0x68; // Automute level. Default is some level
 * static uint8_t reg6_deemphasis              = 0x4A; // Deemphasis. Default = disabled
 */
static uint8_t reg7_general_settings        = 0x80; // General settings. Default sharp fir, pcm iir and unmuted
/*
 * static uint8_t reg8_gpio_configuration      = 0x10; // GPIO configuration
 * static uint8_t reg10_master_mode_control    = 0x05; // Master Mode Control. Default value: master mode off
 * static uint8_t reg11_channel_mapping        = 0x02; // Channel Mapping. Default stereo is Ch1=left, Ch2=right
 * static uint8_t reg12_dpll_settings          = 0x5A; // DPLL Settings. Default = 5 for I2S, A for DSD
 * static uint8_t reg13_thd_comp               = 0x40; // THD Compensation
 * static uint8_t reg14_softstart_settings     = 0x8A; // Soft Start Settings
 */
static uint8_t reg21_gpio_input_selection   = 0x00; // Oversampling filter. Default: oversampling ON

#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))

static uint8_t vol_reg_l = ES9018K2M_REG15_VOLUME_L;
static uint8_t reg15_vol_l = 0;
i2c_descriptor vol_desc_l = {
    .slave_addr = ES9018K2M_ADDR,
    .bus_cond = I2C_START | I2C_STOP,
    .tran_mode = I2C_WRITE,
    .buffer[0] = &vol_reg_l,
    .count[0] = 1,
    .buffer[1] = &reg15_vol_l,
    .count[1] = 1,
    .callback = NULL,
    .arg = 0,
    .next = NULL,
};

static uint8_t vol_reg_r = ES9018K2M_REG16_VOLUME_R;
static uint8_t reg16_vol_r = 0;
i2c_descriptor vol_desc_r = {
    .slave_addr = ES9018K2M_ADDR,
    .bus_cond = I2C_START | I2C_STOP,
    .tran_mode = I2C_WRITE,
    .buffer[0] = &vol_reg_r,
    .count[0] = 1,
    .buffer[1] = &reg16_vol_r,
    .count[1] = 1,
    .callback = NULL,
    .arg = 0,
    .next = NULL,
};

void es9018k2m_set_volume_async(int vol_l, int vol_r)
{
    /* Queue writes to the DAC's volume.
     * Note that this needs to be done asynchronously. From testing, calling
     * synchronous writes from HP/LO detect causes hangs. */
    reg15_vol_l = vol_tenthdb2hw(vol_l);
    reg16_vol_r = vol_tenthdb2hw(vol_r);
    i2c_async_queue(ES9018K2M_BUS, TIMEOUT_NOBLOCK, I2C_Q_ADD, 0, &vol_desc_l);
    i2c_async_queue(ES9018K2M_BUS, TIMEOUT_NOBLOCK, I2C_Q_ADD, 0, &vol_desc_r);
}

void es9018k2m_set_filter_roll_off(int value)
{
    /* Note: the ihifi800 implementation manipulates
    * bit 0 of reg21, but I think that is incorrect?
    * Bit 2 is the bypass for the IIR digital filters,
    * Whereas Bit 0 is the oversampling filter, which
    * the datasheet seems to say should be left on. */

    /* 0 = "Sharp" / Fast Rolloff (Default)
       1 = Slow Rolloff
       2 = "Short" / Minimum Phase
       3 = Bypass */
    switch(value)
    {
        case 0:
            bitClear(reg7_general_settings, 5);
            bitClear(reg7_general_settings, 6);
            bitClear(reg21_gpio_input_selection, 2);
            break;
        case 1:
            bitSet(reg7_general_settings, 5);
            bitClear(reg7_general_settings, 6);
            bitClear(reg21_gpio_input_selection, 2);
            break;
        case 2:
            bitClear(reg7_general_settings, 5);
            bitSet(reg7_general_settings, 6);
            bitClear(reg21_gpio_input_selection, 2);
            break;
        case 3:
            bitSet(reg21_gpio_input_selection, 2);
            break;
    }
    es9018k2m_write_reg(ES9018K2M_REG7_GENERAL_SETTINGS, reg7_general_settings);
    es9018k2m_write_reg(ES9018K2M_REG21_GPIO_INPUT_SELECT, reg21_gpio_input_selection);
}
#endif /* !defined(BOOTLOADER) */

/* returns I2C_STATUS_OK upon success, I2C_STATUS_* errors upon error */
int es9018k2m_write_reg(uint8_t reg, uint8_t val)
{
    return i2c_reg_write1(ES9018K2M_BUS, ES9018K2M_ADDR, reg, val);
}

/* returns register value, or -1 upon error */
int es9018k2m_read_reg(uint8_t reg)
{
    return i2c_reg_read1(ES9018K2M_BUS, ES9018K2M_ADDR, reg);
}