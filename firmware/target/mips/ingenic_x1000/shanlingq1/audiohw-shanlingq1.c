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
#include "pcm_sampr.h"
#include "logf.h"
#include "aic-x1000.h"
#include "i2c-x1000.h"
#include "gpio-x1000.h"
#include "x1000/aic.h"
#include "x1000/cpm.h"

/* TODO this needs to be refactored out to es9218 driver */
static int audiohw_init_table[] = {
    ES9218_REG_VOLUME_LEFT,             0x00,
    ES9218_REG_VOLUME_RIGHT,            0x00,
    ES9218_REG_MIX_AUTOMUTE,            0x34,
    ES9218_REG_AUTOMUTE_TIME,           0xff,
    ES9218_REG_AUTUMUTE_LEVEL,          0x00,
    ES9218_REG_INPUT_SEL,               0x80,
    ES9218_REG_ANALOG_VOL,              0x40,
    ES9218_REG_GPIO1_2_CONFIG,          0xdd,
    ES9218_REG_RESERVED_1,              0x18, /* OF writes default value 0x18 */
    ES9218_REG_MASTER_MODE_CONFIG,      0x02,
    ES9218_REG_ASRC_DPLL_BANDWIDTH,     0xaa,
    ES9218_REG_THD_COMP_BYPASS,         0x00,
    ES9218_REG_SOFT_START_CONFIG,       0x0a,
    ES9218_REG_MASTER_TRIM_BIT0_7,      0xff,
    ES9218_REG_MASTER_TRIM_BIT8_15,     0xff,
    ES9218_REG_MASTER_TRIM_BIT16_23,    0xff,
    ES9218_REG_MASTER_TRIM_BIT24_31,    0x7f,
    ES9218_REG_GPIO_INPUT_SEL,          0x0d,
    ES9218_REG_THD_COMP_C2_LO,          0x00,
    ES9218_REG_THD_COMP_C2_HI,          0x00,
    ES9218_REG_THD_COMP_C3_LO,          0x00,
    ES9218_REG_THD_COMP_C3_HI,          0x00,
    ES9218_REG_CHARGE_PUMP_SS_DELAY,    0x62,
    ES9218_REG_GENERAL_CONFIG,          0xd4,
    ES9218_REG_RESERVED_2,              0xf0, /* OF writes default value 0xf0 */
    ES9218_REG_GPIO_INV_CLOCK_GEAR,     0x00,
    ES9218_REG_CHARGE_PUMP_CLK_LO,      0x00,
    ES9218_REG_CHARGE_PUMP_CLK_HI,      0x00,
    ES9218_REG_INTERRUPT_MASK,          0x3c,
    ES9218_REG_PROG_NCO_BIT0_7,         0x00,
    ES9218_REG_PROG_NCO_BIT8_15,        0x00,
    ES9218_REG_PROG_NCO_BIT16_23,       0x00,
    ES9218_REG_PROG_NCO_BIT24_31,       0x00,
    0x26,                               0x00, /* undocumented register */
    ES9218_REG_RESERVED_3,              0x00, /* OF writes 0, default value is 0xcc */
    ES9218_REG_FIR_RAM_ADDR,            0x00,
    ES9218_REG_FIR_DATA_BIT0_7,         0x00,
    ES9218_REG_FIR_DATA_BIT8_15,        0x00,
    ES9218_REG_FIR_DATA_BIT16_23,       0x00,
};

void audiohw_init(void)
{
    /* Configure AIC */
    aic_set_external_codec(true);
    aic_set_i2s_mode(AIC_I2S_SLAVE_MODE);

    /* TODO: figure out which of these is the correct setting */
#if 1
    aic_enable_i2s_master_clock(true);
    aic_set_i2s_clock(X1000_CLK_EXCLK, X1000_EXCLK_FREQ, 0);
#else
    aic_enable_i2s_master_clock(false);
#endif

    /* Open DAC driver */
    i2c_x1000_set_freq(1, I2C_FREQ_400K);
    es9218_open();

    for(size_t i = 0; i < ARRAYLEN(audiohw_init_table); i += 2)
        es9218_write(audiohw_init_table[i], audiohw_init_table[i+1]);
}

void audiohw_postinit(void)
{
    /* unmute the DAC here */
}

void audiohw_close(void)
{
    es9218_close();
}

void audiohw_set_frequency(int fsel)
{
    // TODO
    (void)fsel;
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    // TODO
    (void)vol_l;
    (void)vol_r;
}

void audiohw_set_filter_roll_off(int value)
{
    // TODO
    (void)value;
}

void audiohw_set_power_mode(int mode)
{
    // TODO this should probably select between the 1 Vrms / 2 Vrms modes
    (void)mode;
}

void es9218_set_power_pin(int level)
{
    gpio_set_level(GPIO_ES9218_POWER, level ? 1 : 0);
}

void es9218_set_reset_pin(int level)
{
    gpio_set_level(GPIO_ES9218_RESET, level ? 1 : 0);
}
