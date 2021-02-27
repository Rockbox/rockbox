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

#include "axp173.h"
#include "adc.h"

static const unsigned char axp173_adc_regs[NUM_ADC_CHANNELS] = {
    0x56, /* ACIN voltage */
    0x58, /* ACIN current */
    0x5a, /* VBUS voltage */
    0x5c, /* VBUS current */
    0x5e, /* Internal temperature */
    0x62, /* TS input */
    0x78, /* Battery voltage */
    0x7a, /* Battery charge current */
    0x7c, /* Battery discharge current */
    0x7e, /* APS voltage */
    0x70, /* Battery power */
};

int axp173_adc_read(int adc)
{
    int value = axp173_adc_read_raw(adc);
    if(value < 0)
        return value;

    return axp173_adc_conv_raw(adc, value);
}

int axp173_adc_read_raw(int adc)
{
    if(adc >= NUM_ADC_CHANNELS)
        return -1;

    /* Read registers */
    unsigned char buf[3];
    int count = (adc == ADC_BATTERY_POWER ? 3 : 2);
    if(axp173_reg_read_multi(axp173_adc_regs[adc], count, &buf[0]))
        return -1;

    /* ADCs are 12 bits on the AXP173. The charge/discharge current is for
     * some reason arranged differently than the other ADCs.
     *
     * The battery power ADC is synthetic: in the AXP192 datasheet it is
     * described as the product of voltage and current.
     */
    int val;
    if(adc == ADC_BATTERY_POWER)
        val = (buf[0] << 16) | (buf[1] << 8) | buf[0];
    else if(adc == ADC_CHARGE_CURRENT || adc == ADC_DISCHARGE_CURRENT)
        val = (buf[0] << 5) | (buf[1] & 0x1f);
    else
        val = (buf[0] << 4) | (buf[1] & 0xf);

    return val;
}

int axp173_adc_conv_raw(int adc, int value)
{
    switch(adc) {
    case ADC_ACIN_VOLTAGE:
    case ADC_VBUS_VOLTAGE:
        /* 0 mV ... 6.9615 mV, step 1.7 mV */
        return value * 17 / 10;
    case ADC_ACIN_CURRENT:
        /* 0 mA ... 2.5594 A, step 0.625 mA */
        return value * 5 / 8;
    case ADC_VBUS_CURRENT:
        /* 0 mA ... 1.5356 A, step 0.375 mA */
        return value * 3 / 8;
    case ADC_INTERNAL_TEMP:
        /* -144.7 C ... 264.8 C, step 0.1 C */
        return value - 1447;
    case ADC_TS_INPUT:
        /* 0 mV ... 3.276 V, step 0.8 mV */
        return value * 4 / 5;
    case ADC_BATTERY_VOLTAGE:
        /* 0 mV ... 4.5045 V, step 1.1 mV */
        return value * 11 / 10;
    case ADC_CHARGE_CURRENT:
    case ADC_DISCHARGE_CURRENT:
        /* 0 mA to 4.095 A, step 0.5 mA */
        return value / 2;
    case ADC_APS_VOLTAGE:
        /* 0 mV to 5.733 V, step 1.4 mV */
        return value * 7 / 5;
    case ADC_BATTERY_POWER:
        /* 0 uW to 23.6404 W, step 0.55 uW */
        return value * 11 / 20;
    default:
        /* Just pass through other values raw */
        return value;
    }
}

int axp173_adc_enable(int adc, bool enabled)
{
    int reg = 0x82, bit;
    switch(adc) {
    case ADC_ACIN_VOLTAGE: bit = 5; break;
    case ADC_ACIN_CURRENT: bit = 4; break;
    case ADC_VBUS_VOLTAGE: bit = 3; break;
    case ADC_VBUS_CURRENT: bit = 2; break;
    case ADC_INTERNAL_TEMP: reg = 0x83; bit = 7; break;
    case ADC_TS_INPUT: bit = 0; break;
    case ADC_BATTERY_VOLTAGE: bit = 7; break;
    case ADC_APS_VOLTAGE: bit = 1; break;
    case ADC_CHARGE_CURRENT:
    case ADC_DISCHARGE_CURRENT:
        bit = 6;
        break;
    case ADC_BATTERY_POWER:
    default:
        return -1;
    }

    if(enabled)
        return axp173_reg_set(reg, (1 << bit));
    else
        return axp173_reg_clr(reg, (1 << bit));
}

int axp173_cc_read(int counter, unsigned* value)
{
    if(counter != AXP173_CC_CHARGE && counter != AXP173_CC_DISCHARGE)
        return -1;

    /* We defined the counter constants to equal the register */
    unsigned char buf[4];
    if(axp173_reg_read_multi(counter, 4, &buf[0]))
        return -1;

    *value = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    return 0;
}

int axp173_cc_clear(void)
{
    return axp173_reg_set(0xb8, 1 << 5);
}

int axp173_cc_pause(bool paused)
{
    if(paused)
        return axp173_reg_set(0xb8, 1 << 6);
    else
        return axp173_reg_clr(0xb8, 1 << 6);
}

int axp173_cc_enable(bool en)
{
    if(en)
        return axp173_reg_set(0xb8, 1 << 7);
    else
        return axp173_reg_clr(0xb8, 1 << 7);
}

#if defined(HAVE_AXP173_POWERMGMT) && (CONFIG_CHARGING == CHARGING_TARGET)
/* TODO: Implement charge reporting based on coulomb counters
 *
 * Currently this is blocked until the I2C implementation improves, since
 * we've now got two drivers (this and button) needing better I2C access.
 *
 * - The first time Rockbox boots, or after a total power loss, the AXP173
 *   contains no data. Need to run tests to figure out sane defaults so we
 *   can provide a reasonable battery estimate.
 *
 * - Net charge is full_charge + charge_cc - discharge_cc. Since counters run
 *   even while power is off, this is a fully persistent value.
 *
 * - When the battery is fully charged (according to chip), we record the
 *   current net charge as the full_charge level. This is written to the
 *   persistent storage. Counters should be reset at this time.
 *
 * - Anything else I'm missing??
 */

void powermgmt_init_target(void)
{
    /* Here we do an initial sync and check for total power loss condition,
     * so we can set our in-memory charge levels
     */
}

void charging_algorithm_step(void)
{
    /* This is a good place to kick off polling and sync our charge levels.
     *
     * Powermgmt thread will call us every 1/2 second, which is too much,
     * so put some additional throttling here.
     */
}

void charging_algorithm_close(void)
{
    /* We can use this opportunity to write some final data into the chip,
     * but IMO it's better to just do that periodically in step() rather
     * than waiting till shutdown
     */
}

int _battery_level(void)
{
    return 100;
}

int _battery_time(void)
{
    return 60;
}
#endif
