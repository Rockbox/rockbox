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
#include "i2c-async.h"
#include "fuelgauge.h"
#include "system.h"

static struct axp173_fg {
    int batt_status;
    int temp;
    int v_batt;
    int i_batt;
    int p_batt;
    long charge;
    int adc_sample_rate;
} axp173_fg;

void fuelgauge_tgt_init(void)
{
    /* Force required ADCs to be enabled */
    int bits = axp173_adc_get_enabled();
    bits |= (1 << ADC_BATTERY_VOLTAGE);
    bits |= (1 << ADC_CHARGE_CURRENT);
    bits |= (1 << ADC_DISCHARGE_CURRENT);
    bits |= (1 << ADC_BATTERY_POWER);
    bits |= (1 << ADC_INTERNAL_TEMP);
    axp173_adc_set_enabled(bits);

    /* Higher sample rates increase the coulomb counter accuracy */
    axp173_adc_set_rate(AXP173_ADC_RATE_100HZ);
    axp173_fg.adc_sample_rate = 100;

    /* Ensure CCs are running */
    axp173_cc_enable(true);

    /* Populate the initial readings */
    fuelgauge_tgt_step();
}

void fuelgauge_tgt_step(void)
{
    /* Update status */
    axp173_fg.batt_status = axp173_battery_status();

    /* Update ADCs */
    axp173_fg.v_batt = axp173_adc_read_raw(ADC_BATTERY_VOLTAGE);
    axp173_fg.temp = axp173_adc_read_raw(ADC_INTERNAL_TEMP);

    switch(axp173_fg.batt_status) {
    case AXP173_BATT_DISCHARGING:
        axp173_fg.i_batt = axp173_adc_read_raw(ADC_DISCHARGE_CURRENT);
        axp173_fg.p_batt = axp173_adc_read_raw(ADC_BATTERY_POWER);
        break;

    case AXP173_BATT_CHARGING:
        axp173_fg.i_batt = axp173_adc_read_raw(ADC_CHARGE_CURRENT);
        if(axp173_fg.v_batt != INT_MIN && axp173_fg.i_batt != INT_MIN)
            axp173_fg.p_batt = axp173_fg.v_batt * axp173_fg.i_batt;
        else
            axp173_fg.p_batt = 0;
        break;

    case AXP173_BATT_FULL:
    default:
        axp173_fg.i_batt = 0;
        axp173_fg.p_batt = 0;
        break;
    }

    /* Read coulomb counters */
    uint32_t charge, discharge;
    axp173_cc_read(&charge, &discharge);
    axp173_fg.charge = (int32_t)(charge - discharge);
}

void fuelgauge_tgt_shutdown(void)
{
    /* Nothing to do here */
}

int fuelgauge_tgt_battery_status(void)
{
    switch(axp173_fg.batt_status) {
    case AXP173_BATT_DISCHARGING:
    default:
        return FUELGAUGE_BATTERY_DISCHARGING;
    case AXP173_BATT_CHARGING:
        return FUELGAUGE_BATTERY_CHARGING;
    case AXP173_BATT_FULL:
        return FUELGAUGE_BATTERY_FULL;
    }
}

long fuelgauge_tgt_read_voltage(void)
{
    if(axp173_fg.v_batt == INT_MIN)
        return -1;

    return axp173_fg.v_batt;
}

long fuelgauge_tgt_read_current(void)
{
    if(axp173_fg.i_batt == INT_MIN)
        return -1;

    return axp173_fg.i_batt;
}

long fuelgauge_tgt_read_power(void)
{
    if(axp173_fg.p_batt == INT_MIN)
        return -1;

    return axp173_fg.p_batt;
}

long fuelgauge_tgt_read_temperature(void)
{
    if(axp173_fg.temp == INT_MIN)
        return -1;

    return axp173_fg.temp;
}

long fuelgauge_tgt_read_charge(void)
{
    return axp173_fg.charge;
}

void fuelgauge_tgt_clear_charge(void)
{
    axp173_cc_clear();
}

long fuelgauge_tgt_convert_voltage(long value)
{
    return axp173_adc_conv_raw(ADC_BATTERY_VOLTAGE, value);
}

long fuelgauge_tgt_convert_current(long value)
{
    return axp173_adc_conv_raw(ADC_DISCHARGE_CURRENT, value);
}

long fuelgauge_tgt_convert_power(long value)
{
    /* ADC gives microwatts; round to milliwatts */
    long conv = axp173_adc_conv_raw(ADC_BATTERY_POWER, value);
    long ret = conv / 1000;
    long rem = conv % 1000;
    if(rem < 500)
        return ret;
    else
        return ret + 1;
}

long fuelgauge_tgt_convert_temperature(long value)
{
    /* Assume internal temperature ~= battery temperature, since an
     * actual battery temperature probe may not be present.
     */
    return axp173_adc_conv_raw(ADC_INTERNAL_TEMP, value);
}

long fuelgauge_tgt_convert_charge(long value)
{
    /* The multiplication can easily overflow 32 bits if the battery
     * capacity and ADC rate are high. Use 64-bit math to avoid this. */
    int64_t x = value;
    x *= 32768;
    x /= (axp173_fg.adc_sample_rate * 3600);
    return x;
}

#define CHARGE_LIMIT_BITS 15
#define CHARGE_LIMIT_MAX  ((1 << CHARGE_LIMIT_BITS) - 1)
#define CHARGE_LIMIT_MIN  (-(1 << CHARGE_LIMIT_BITS))

static uint16_t serialize_charge_limit(long v)
{
    /* Convert to mAh for storage */
    v = MIN(v, CHARGE_LIMIT_MAX);
    v = MAX(v, CHARGE_LIMIT_MIN);
    v += (1 << CHARGE_LIMIT_BITS);
    return (uint16_t)v;
}

static long deserialize_charge_limit(uint16_t v)
{
    long x = v;
    x -= (1 << CHARGE_LIMIT_BITS);
    return x;
}

void fuelgauge_tgt_save_data(unsigned int bits, const fuelgauge_data* fgd)
{
    if(!(bits & FUELGAUGE_DATA_CHARGE_LIMITS))
        return;

    uint16_t cmin = serialize_charge_limit(fgd->charge_min);
    uint16_t cmax = serialize_charge_limit(fgd->charge_max);

    /* AXP173 multi-byte writes require the register address prefixed
     * before each byte, so 6 data bytes + 5 address bytes in the buffer.
     * The first register address is provided by i2c_reg_write(). */
    uint8_t buf[11];
    buf[0] = cmin & 0xff;
    buf[1] = 0x07;
    buf[2] = (cmin >> 8) & 0xff;
    buf[3] = 0x08;
    buf[4] = cmax & 0xff;
    buf[5] = 0x09;
    buf[6] = (cmax >> 8) & 0xff;
    buf[7] = 0x0a;
    /* TODO: could use a checksum/ECC here instead of magic bytes */
    buf[8] = 'R';
    buf[9] = 0x0b;
    buf[10] = 'b';

    i2c_reg_write(AXP173_BUS, AXP173_ADDR, 0x06, 11, &buf[0]);
}

unsigned int fuelgauge_tgt_load_data(fuelgauge_data* fgd)
{
    uint8_t buf[6];
    int rc = i2c_reg_read(AXP173_BUS, AXP173_ADDR, 0x06, 6, &buf[0]);
    if(rc != I2C_STATUS_OK)
        return 0;

    if(buf[4] != 'R' || buf[5] != 'b')
        return 0;

    uint16_t cmin = buf[0] | (buf[1] << 8);
    uint16_t cmax = buf[2] | (buf[3] << 8);
    fgd->charge_min = deserialize_charge_limit(cmin);
    fgd->charge_max = deserialize_charge_limit(cmax);

    return FUELGAUGE_DATA_CHARGE_LIMITS;
}
