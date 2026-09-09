/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

#include "power.h"
#include "powermgmt.h"
#include "adc.h"
#include "system.h"
#include <limits.h>
#include "kernel.h"
#include "axp-2101.h"
#include "usb-x1600.h"
#include "button-target.h"
#include <stdint.h>
#ifdef HAVE_CW2015
# include "cw2015.h"
#endif
#ifdef HAVE_USB_CHARGING_ENABLE
# include "usb_core.h"
#endif

#include "i2c-ingenic.h"
#include "x1600/sadc.h"
#include "x1600/cpm.h"

/* Single-cell Li-ion battery, rated to 4.35 V. State of charge comes from
 * the CW2015. */

/* Low-battery thresholds from X1000; not calibrated for the R1. */
unsigned short battery_level_disksafe = 3470;
unsigned short battery_level_shutoff = 3400;

/* Placeholder voltage curves from Eros Q Native. Calibrate against an R1
 * battery discharge before relying on voltage-derived percentages. */
/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
unsigned short percent_to_volt_discharge[11] =
{
    3400, 3477, 3540, 3578, 3617, 3674, 3771, 3856, 3936, 4016, 4117
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
unsigned short percent_to_volt_charge[11] =
{
    3400, 3477, 3540, 3578, 3617, 3674, 3771, 3856, 3936, 4016, 4117
};

/* AXP2101 rail voltages from the vendor configuration. Known consumers are
 * noted below. */
#define R1_MV_DCDC1     3300
#define R1_MV_DCDC2     1100
#define R1_MV_DCDC3     1800
#define R1_MV_DCDC4     1200
#define R1_MV_ALDO1     3300
#define R1_MV_ALDO2     3300
#define R1_MV_ALDO3     1100
#define R1_MV_ALDO4     3300
#define R1_MV_BLDO1     2800
#define R1_MV_BLDO2     2800
#define R1_MV_DLDO1     3300
#define R1_MV_DLDO2     1250

/* ADC voltage scaling depends on the board reference and dividers. */

int adc_read_mv(int channel)
{
    return ADC_TO_MV(adc_read(channel));
}

/* Keep failed samples from changing the cable or charging state. */
static volatile bool power_present;
static volatile bool battery_charging;
static int power_cookie;
static const uint8_t power_regs[] = {
    AXP2101_REG_PMU_STATUS2, AXP2101_REG_ADC_VBUS_H,
};
static uint8_t power_status;
static uint8_t power_vbus[2];

static void power_update_vbus(int mv)
{
    if(mv == INT_MIN)
        return;

    if(mv > 4000)
        power_present = true;
    else if(mv < 1500)
        power_present = false;

#if defined(HAVE_USBSTACK) && !defined(USB_NONE)
    x1600_usb_update_detect(power_present);
#endif
}

static void power_status_cb(int status, i2c_descriptor* desc)
{
    (void)desc;
    if(status == I2C_STATUS_OK)
        battery_charging = ((power_status >> 5) & 3) == 1;
}

static void power_vbus_cb(int status, i2c_descriptor* desc)
{
    (void)desc;
    if(status == I2C_STATUS_OK) {
        int raw = ((power_vbus[0] & 0x3f) << 8) | power_vbus[1];
        power_update_vbus(axp2101_adc_conv_raw(AXP2101_ADC_VBUS_VOLTAGE, raw));
    }
}

static i2c_descriptor power_desc[] = {
    {
        .slave_addr = AXP_PMU_ADDR,
        .bus_cond = I2C_START | I2C_STOP,
        .tran_mode = I2C_READ,
        .buffer = {(void*)&power_regs[0], &power_status},
        .count = {1, 1},
        .callback = power_status_cb,
    },
    {
        .slave_addr = AXP_PMU_ADDR,
        .bus_cond = I2C_START | I2C_STOP,
        .tran_mode = I2C_READ,
        .buffer = {(void*)&power_regs[1], power_vbus},
        .count = {1, 2},
        .callback = power_vbus_cb,
    },
};

static int power_poll(struct timeout* tmo)
{
    (void)tmo;
    for(unsigned i = 0; i < ARRAYLEN(power_desc); ++i)
        i2c_async_queue(AXP_PMU_BUS, TIMEOUT_NOBLOCK, I2C_Q_ONCE,
                        power_cookie + i, &power_desc[i]);
    return HZ/4;
}

/* Power */

void power_init(void)
{
    i2c_ingenic_set_freq(AXP_PMU_BUS, I2C_FREQ_400K);
    axp2101_init();
#ifdef HAVE_CW2015
    cw2015_init();
#endif

    /* Enable the ADCs we care about */
    axp2101_adc_set_enabled(
        (1 << AXP2101_ADC_VBAT_VOLTAGE) |
        (1 << AXP2101_ADC_VBUS_VOLTAGE) |
        (1 << AXP2101_ADC_VSYS_VOLTAGE) |
        (1 << AXP2101_ADC_DIE_TEMPERATURE));

    /* axp2101_supply_set_voltage() also enables the rail */
    axp2101_supply_set_voltage(AXP2101_SUPPLY_DCDC1, R1_MV_DCDC1);
    axp2101_supply_set_voltage(AXP2101_SUPPLY_DCDC2, R1_MV_DCDC2);
    axp2101_supply_set_voltage(AXP2101_SUPPLY_DCDC3, R1_MV_DCDC3);
    axp2101_supply_set_voltage(AXP2101_SUPPLY_DCDC4, R1_MV_DCDC4);
    axp2101_supply_set_voltage(AXP2101_SUPPLY_ALDO3, R1_MV_ALDO3);

    /* Rails with a known consumer in this port */
    axp2101_supply_set_voltage(AXP2101_SUPPLY_ALDO2, R1_MV_ALDO2); /* touch */
    axp2101_supply_set_voltage(AXP2101_SUPPLY_ALDO4, R1_MV_ALDO4); /* remote */
    axp2101_supply_set_voltage(AXP2101_SUPPLY_BLDO1, R1_MV_BLDO1); /* LCD VCC */
    axp2101_supply_set_voltage(AXP2101_SUPPLY_BLDO2, R1_MV_BLDO2); /* LCD IO */

    /* aldo1, dldo1 and dldo2 are populated but it is unknown what they feed.
     * The vendor leaves them off at boot, so this does too. */

    /* The driver leaves charge-voltage, termination-current and DCDC3 PWM
     * settings unchanged. */

    /* Initial charging-current limit: 500 mA. */
    axp2101_set_charge_current(500);

    /* Let the rails settle before anything talks to the panel or DAC */
    mdelay(20);

    int status = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR,
                               AXP2101_REG_PMU_STATUS2);
    if(status >= 0)
        battery_charging = ((status >> 5) & 3) == 1;
    power_update_vbus(axp2101_adc_read(AXP2101_ADC_VBUS_VOLTAGE));

    static struct timeout power_tmo;
    power_cookie = i2c_async_reserve_cookies(AXP_PMU_BUS, ARRAYLEN(power_desc));
    timeout_register(&power_tmo, power_poll, HZ/4, 0);
}

#ifdef HAVE_USB_CHARGING_ENABLE
void usb_charging_maxcurrent_change(int maxcurrent)
{
    axp2101_set_charge_current(maxcurrent);
}
#endif

void power_off(void)
{
    axp2101_power_off();
    while(1);
}

unsigned int power_input_status(void)
{
    return power_present ? POWER_INPUT_USB_CHARGER : POWER_INPUT_NONE;
}

bool charging_state(void)
{
    return power_present && battery_charging;
}

int _battery_voltage(void)
{
    /* Use the PMU battery-voltage reading. */
    static int last_mv = 3700; /* Fallback until the first successful reading. */
    int mv = axp2101_adc_read(AXP2101_ADC_VBAT_VOLTAGE);

    /* Retain the last valid voltage on I2C failure to avoid a false low-battery shutdown. */
    if(mv != INT_MIN)
        last_mv = mv;

    return last_mv;
}

#if defined(HAVE_CW2015) && (CONFIG_BATTERY_MEASURE & PERCENTAGE_MEASURE) != 0
int _battery_level(void)
{
    return cw2015_get_soc();
}
#endif

#if defined(HAVE_CW2015) && (CONFIG_BATTERY_MEASURE & TIME_MEASURE) != 0
int _battery_time(void)
{
    return cw2015_get_rrt();
}
#endif
