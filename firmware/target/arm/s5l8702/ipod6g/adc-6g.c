/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: adc-s5l8700.c 21775 2009-07-11 14:12:02Z bertrik $
 *
 * Copyright (C) 2009 by Bertrik Sikken
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

#include "config.h"

#include "inttypes.h"
#include "s5l8702.h"
#include "adc.h"
#include "adc-target.h"
#include "pmu-target.h"
#include "kernel.h"


/* MS_TO_TICKS converts a milisecond time period into the
 * corresponding amount of ticks. If the time period cannot
 * be accurately measured in ticks it will round up.
 */
#define MS_PER_HZ (1000/HZ)
#define MS_TO_TICKS(x) (((x)+MS_PER_HZ-1)/MS_PER_HZ)

static const struct pmu_adc_channel adc_channels[] =
{
    [ADC_BATTERY] =
    {
        .name = "Battery",
        .adcc1 = PCF5063X_ADCC1_MUX_BATSNS_SUBTR
               | PCF5063X_ADCC1_AVERAGE_4
               | PCF5063X_ADCC1_RES_10BIT,
    },

    [ADC_USBDATA] =
    {
        .name = "USB D+/D-",
        .adcc1 = PCF5063X_ADCC1_MUX_ADCIN2_RES
               | PCF5063X_ADCC1_AVERAGE_16
               | PCF5063X_ADCC1_RES_10BIT,
        .adcc3 = PCF5063X_ADCC3_RES_DIV_THREE,
    },

    [ADC_ACCESSORY] =
    {
        .name = "Accessory",
        .adcc1 = PCF5063X_ADCC1_MUX_ADCIN1
               | PCF5063X_ADCC1_AVERAGE_16
               | PCF5063X_ADCC1_RES_10BIT,
        .adcc2 = PCF5063X_ADCC2_RATIO_ADCIN1
               | PCF5063X_ADCC2_RATIOSETTL_10US,
        .adcc3 = PCF5063X_ADCC3_ACCSW_EN,
        .bias_dly = MS_TO_TICKS(50),
    },
};

const char *adc_name(int channel)
{
    return adc_channels[channel].name;
}

unsigned short adc_read_millivolts(int channel)
{
    const struct pmu_adc_channel *ch = &adc_channels[channel];
    return pmu_adc_raw2mv(ch, pmu_read_adc(ch));
}

/* Returns battery voltage [millivolts] */
unsigned int adc_read_battery_voltage(void)
{
    return adc_read_millivolts(ADC_BATTERY);
}

/* Returns USB D+/D- voltage from ADC [millivolts] */
unsigned int adc_read_usbdata_voltage(bool dp)
{
    unsigned int mvolts;
    int gpio = dp ? 0xb0300 : 0xb0200; /* select D+/D- */
    GPIOCMD = gpio | 0xf; /* route to ADCIN2 */
    mvolts = adc_read_millivolts(ADC_USBDATA);
    GPIOCMD = gpio | 0xe; /* unroute */
    return mvolts;
}

/* Returns resistor connected to "Accessory identify" pin [ohms] */
#define IAP_DEVICE_RESISTOR 100000  /* ohms */
int adc_read_accessory_resistor(void)
{
    int raw = pmu_read_adc(&adc_channels[ADC_ACCESSORY]);
    return (1023-raw) ? raw * IAP_DEVICE_RESISTOR / (1023-raw)
                      : -1 /* open circuit */;
}


/* API functions */
unsigned short adc_read(int channel)
{
    return pmu_read_adc(&adc_channels[channel]);
}

void adc_init(void)
{
}
