/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Marcin Bukat
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

#include "button.h"
#include "system.h"
#include "kernel.h"
#include "axp-pmu.h"
#include "gpio-ingenic.h"
#include "i2c-async.h"
#include <stdbool.h>

/* Keypad layout:
 * - VOL+ (PB28), VOL- (PC1), POWER (PB31): GPIOs, active low
 * - LEFT/RIGHT/UP/DOWN/SELECT: resistor ladder on AXP192 GPIO0,
 *   sampled by the PMU ADC (regs 0x64/0x65, 12 bit, 0.5 mV/LSB)
 *
 * Thresholds are the raw ADC windows used by the OF adc_key driver.
 */

#define AXP192_REG_ADCRANGE         0x85
#define AXP192_REG_GPIO0ADC         0x64
#define AXP192_ADCEN2_GPIO0         0x08
#define AXP192_GPIO0_FUNC_ADC       0x04

/* Poll interval of the ladder; the AXP ADC runs at 100 Hz */
#define ADCKEY_POLL_TIME    (HZ/50)

struct adckey {
    int button;
    int min, max;
};

static const struct adckey adckeys[] = {
    {BUTTON_LEFT,      0,   50},
    {BUTTON_RIGHT,   950, 1150},
    {BUTTON_UP,     1750, 1950},
    {BUTTON_DOWN,   2500, 2700},
    {BUTTON_SELECT, 3050, 3250},
};

static uint8_t adckey_raw[2];
static volatile int adckey_btn = 0;

static int adckey_decode(void)
{
    int val = (adckey_raw[0] << 4) | (adckey_raw[1] & 0x0f);
    for(size_t i = 0; i < ARRAYLEN(adckeys); ++i)
        if(val >= adckeys[i].min && val < adckeys[i].max)
            return adckeys[i].button;

    return 0;
}

static void adckey_i2c_cb(int status, i2c_descriptor* desc)
{
    (void)desc;
    static int last = 0;

    if(status != I2C_STATUS_OK)
        return;

    /* require two identical samples in a row */
    int btn = adckey_decode();
    if(btn == last)
        adckey_btn = btn;
    last = btn;
}

static int adckey_tmo_cb(struct timeout* tmo)
{
    i2c_descriptor* d = (i2c_descriptor*)tmo->data;
    i2c_async_queue(AXP_PMU_BUS, TIMEOUT_NOBLOCK, I2C_Q_ADD, 0, d);
    return ADCKEY_POLL_TIME;
}

static void adckey_init(void)
{
    static struct timeout tmo;
    static const uint8_t adc_reg = AXP192_REG_GPIO0ADC;
    static i2c_descriptor desc = {
        .slave_addr = AXP_PMU_ADDR,
        .bus_cond = I2C_START | I2C_STOP,
        .tran_mode = I2C_READ,
        .buffer[0] = (void*)&adc_reg,
        .count[0] = 1,
        .buffer[1] = adckey_raw,
        .count[1] = 2,
        .callback = adckey_i2c_cb,
        .arg = 0,
        .next = NULL,
    };

    /* GPIO0 as ADC input, 0 - 2.0475 V range */
    i2c_reg_write1(AXP_PMU_BUS, AXP_PMU_ADDR,
                   AXP192_REG_GPIO0FUNCTION, AXP192_GPIO0_FUNC_ADC);
    i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                    AXP192_REG_ADCRANGE, 0x01, 0, NULL);

    /* Enable the GPIO0 ADC channel. Note axp_adc_set_enabled() rewrites
     * ADCENABLE2, so this must happen after power_init(). */
    i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                    AXP_REG_ADCENABLE2, 0, AXP192_ADCEN2_GPIO0, NULL);

    timeout_register(&tmo, &adckey_tmo_cb, ADCKEY_POLL_TIME, (intptr_t)&desc);
}

void button_init_device(void)
{
    adckey_init();
}

int button_read_device(void)
{
    int r = adckey_btn;

    if(!gpio_get_level(GPIO_BTN_VOL_UP))
        r |= BUTTON_VOLUP;
    if(!gpio_get_level(GPIO_BTN_VOL_DOWN))
        r |= BUTTON_VOLDOWN;
    if(!gpio_get_level(GPIO_BTN_POWER))
        r |= BUTTON_POWER;

    return r;
}

bool headphones_inserted(void)
{
    /* PC20, low when plugged in */
    return !gpio_get_level(GPIO_HP_DETECT);
}
