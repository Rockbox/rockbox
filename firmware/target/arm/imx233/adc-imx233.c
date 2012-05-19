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

#include "adc-imx233.h"

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
    int virt = imx233_lradc_acquire_channel(TIMEOUT_BLOCK);
    // divide by two for wider ranger
    imx233_lradc_setup_channel(virt, div2, false, 0, src);
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
            return adc_read_physical(HW_LRADC_CHANNEL_VDDIO, false);
        case IMX233_ADC_VDD5V:
            /* Vdd5V pin has a builtin 4:1 divide */
            return adc_read_physical(HW_LRADC_CHANNEL_5V, false) * 2;
        case IMX233_ADC_DIE_TEMP:
        {
            // don't block on second channel otherwise we might deadlock !
            int nmos_chan = imx233_lradc_acquire_channel(TIMEOUT_BLOCK);
            int pmos_chan = imx233_lradc_acquire_channel(TIMEOUT_NOBLOCK);
            int val = 0;
            if(pmos_chan >= 0)
            {
                val = imx233_lradc_sense_die_temperature(nmos_chan, pmos_chan) - 273;
                imx233_lradc_release_channel(pmos_chan);
            }
            imx233_lradc_release_channel(nmos_chan);
            return val;
        }
#ifdef IMX233_BATT_TEMP_SENSOR
        case IMX233_ADC_BATT_TEMP:
        {
            int virt = imx233_lradc_acquire_channel(TIMEOUT_BLOCK);
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
