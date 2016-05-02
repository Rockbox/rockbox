/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "adc-target.h"
#include "system.h"
#include "kernel.h"
#include "powermgmt-target.h"
#include "lradc-imx233.h"
#include "power-imx233.h"

/* Virtual channels */
#define IMX233_ADC_BATTERY      -1 /* Battery voltage (mV) */
#define IMX233_ADC_DIE_TEMP     -2 /* Die temperature (°C) */
#define IMX233_ADC_VDDIO        -3 /* VddIO voltage (mV) */
#if IMX233_SUBTARGET >= 3700
#define IMX233_ADC_VDD5V        -4 /* Vdd5V voltage (mV) */
#endif
#ifdef IMX233_BATT_TEMP_SENSOR
#define IMX233_ADC_BATT_TEMP    -5 /* Battery temperature (°C) */
#endif

static const char *imx233_adc_channel_name[NUM_ADC_CHANNELS] =
{
    [ADC_BATTERY] = "Battery(raw)",
    [ADC_DIE_TEMP] = "Die temp(°C)",
    [ADC_VDDIO] = "VddIO(mV)",
#if IMX233_SUBTARGET >= 3700
    [ADC_VDD5V] = "Vdd5V(mV)",
#endif
#ifdef IMX233_BATT_TEMP_SENSOR
    [ADC_BATT_TEMP] = "Battery temp(raw)",
#endif
};

static int imx233_adc_mapping[NUM_ADC_CHANNELS] =
{
    [ADC_BATTERY] = IMX233_ADC_BATTERY,
    [ADC_DIE_TEMP] = IMX233_ADC_DIE_TEMP,
    [ADC_VDDIO] = IMX233_ADC_VDDIO,
#if IMX233_SUBTARGET >= 3700
    [ADC_VDD5V] = IMX233_ADC_VDD5V,
#endif
#ifdef IMX233_BATT_TEMP_SENSOR
    [ADC_BATT_TEMP] = IMX233_ADC_BATT_TEMP,
#endif
};

void adc_init(void)
{
}

static short adc_read_physical_ex(int virt)
{
    imx233_lradc_clear_channel(virt);
    imx233_lradc_kick_channel(virt);
    imx233_lradc_wait_channel(virt);
    return imx233_lradc_read_channel(virt);
}

static short adc_read_physical(int src, bool div2)
{
    int virt = imx233_lradc_acquire_channel(src, TIMEOUT_BLOCK);
    // divide by two for wider ranger
    imx233_lradc_setup_source(virt, div2, src);
    imx233_lradc_setup_sampling(virt, false, 0);
    int val = adc_read_physical_ex(virt);
    imx233_lradc_release_channel(virt);
    return val;
}

static short adc_read_virtual(int c)
{
    switch(c)
    {
        case IMX233_ADC_BATTERY:
            return imx233_lradc_read_battery_voltage();
        case IMX233_ADC_VDDIO:
            /* VddIO pin has a builtin 2:1 divide */
            return adc_read_physical(LRADC_SRC_VDDIO, true) * 2;
#if IMX233_SUBTARGET >= 3700
        case IMX233_ADC_VDD5V:
            /* Vdd5V pin has a builtin 4:1 divide */
            return adc_read_physical(LRADC_SRC_5V, true) * 4;
#endif
        case IMX233_ADC_DIE_TEMP:
        {
#if IMX233_SUBTARGET >= 3700
            // don't block on second channel otherwise we might deadlock !
            int nmos_chan = imx233_lradc_acquire_channel(LRADC_SRC_NMOS_THIN, TIMEOUT_BLOCK);
            int pmos_chan = imx233_lradc_acquire_channel(LRADC_SRC_PMOS_THIN, TIMEOUT_NOBLOCK);
            int val = 0;
            if(pmos_chan >= 0)
            {
                val = imx233_lradc_sense_die_temperature(nmos_chan, pmos_chan) - 273;
                imx233_lradc_release_channel(pmos_chan);
            }
            imx233_lradc_release_channel(nmos_chan);
#else
            int min, max, val;
            if(imx233_power_sense_die_temperature(&min, &max) < 0)
                val = -1;
            else
                val = (max + min) / 2;
#endif
            return val;
        }
#ifdef IMX233_BATT_TEMP_SENSOR
        case IMX233_ADC_BATT_TEMP:
        {
            int virt = imx233_lradc_acquire_channel(IMX233_BATT_TEMP_SENSOR, TIMEOUT_BLOCK);
            int val = imx233_lradc_sense_ext_temperature(virt, IMX233_BATT_TEMP_SENSOR);
            imx233_lradc_release_channel(virt);
            return val;
        }
#endif
        default:
            return 0;
    }
}

unsigned short adc_read(int channel)
{
    int c = imx233_adc_mapping[channel];
    if(c < 0)
        return adc_read_virtual(c);
    else
        return adc_read_physical(c, true);
}

const char *adc_name(int channel)
{
    return imx233_adc_channel_name[channel];
}
