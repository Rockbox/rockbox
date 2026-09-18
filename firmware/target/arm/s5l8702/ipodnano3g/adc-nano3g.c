/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#include "s5l87xx.h"
#include "adc.h"
#include "adc-target.h"
#include "pmu-target.h"
#include "kernel.h"

/* Only the battery input is known: the original firmware selects it with
 * 0x24 and converts with 2500 mV + raw * 2000 mV / 1023 (measured: 4062 mV
 * on battery, 4110 mV charging). The USB data and accessory inputs are not
 * identified yet and read as 0. */
static const struct pmu_adc_channel adc_channels[] =
{
    [ADC_BATTERY] =
    {
        .name = "Battery",
        .mux = 0x24,
        .samples = 4,
        .offset_mv = 2500,
        .span_mv = 2000,
    },
    [ADC_USBDATA] =
    {
        .name = "USB data",
    },
    [ADC_ACCESSORY] =
    {
        .name = "Accessory",
    },
};

unsigned short adc_read_millivolts(int channel)
{
    const struct pmu_adc_channel *ch = &adc_channels[channel];

    if (!ch->samples)
        return 0;
    return pmu_adc_raw2mv(ch, pmu_read_adc(ch));
}

/* Returns battery voltage [millivolts] */
unsigned int adc_read_battery_voltage(void)
{
    return adc_read_millivolts(ADC_BATTERY);
}

/* API functions */
unsigned short adc_read(int channel)
{
    const struct pmu_adc_channel *ch = &adc_channels[channel];

    return ch->samples ? pmu_read_adc(ch) : 0;
}

int adc_read_accessory_resistor(void)
{
    return 0;
}

unsigned int adc_read_usbdata_voltage(bool dp)
{
    (void)dp;
    return 0;
}

const char *adc_name(int channel)
{
    return adc_channels[channel].name;
}

void adc_init(void)
{
}
