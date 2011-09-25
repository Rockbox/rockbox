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
#include "adc.h"
#include "adc-target.h"
#include "system.h"
#include "adc-imx233.h"

/* dedicate two channels to temperature sensing
 * dedicate channel 7 to battery
 * and channel 6 to vddio */
static int pmos_chan, nmos_chan;
static int battery_chan, vddio_chan;

void adc_init(void)
{
    imx233_lradc_init();
    battery_chan = 7;
    imx233_lradc_reserve_channel(7);
    vddio_chan = 6;
    imx233_lradc_reserve_channel(6);
    
    pmos_chan = imx233_lradc_acquire_channel(0);
    if(pmos_chan < 0) panicf("No LRADC channel for PMOS !");
    nmos_chan = imx233_lradc_acquire_channel(0);
    if(nmos_chan < 0) panicf("No LRADC channel for NMOS !");

    // setup them
    imx233_lradc_setup_channel(battery_chan, false, false, 0, HW_LRADC_CHANNEL_BATTERY);
    imx233_lradc_setup_channel(vddio_chan, false, false, 0, HW_LRADC_CHANNEL_VDDIO);
    imx233_lradc_setup_channel(nmos_chan, false, false, 0, HW_LRADC_CHANNEL_NMOS_THIN);
    imx233_lradc_setup_channel(pmos_chan, false, false, 0, HW_LRADC_CHANNEL_PMOS_THIN);
}

int adc_read_physical_ex(int virt)
{
    imx233_lradc_clear_channel(virt);
    imx233_lradc_kick_channel(virt);
    imx233_lradc_wait_channel(virt);
    int v = imx233_lradc_read_channel(virt);
    return v;
}

int adc_read_physical(int src)
{
    int virt = imx233_lradc_acquire_channel(TIMEOUT_BLOCK);
    // divide by two for wider ranger
    imx233_lradc_setup_channel(virt, false, false, 0, src);
    int val = adc_read_physical_ex(virt);
    imx233_lradc_release_channel(virt);
    return val;
}

unsigned short adc_read_virtual(int c)
{
    switch(c)
    {
        case IMX233_ADC_BATTERY:
            return adc_read_physical_ex(battery_chan);
        case IMX233_ADC_VDDIO:
            return adc_read_physical_ex(vddio_chan);
        case IMX233_ADC_DIE_TEMP:
            // do kelvin to celsius conversion
            return imx233_lradc_sense_die_temperature(nmos_chan, pmos_chan) - 273;
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
        return adc_read_physical(c);
}
