/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
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
#include "powermgmt.h"
#include "tsc2100.h"
#include "kernel.h"

unsigned short current_bat2 = 4100;
unsigned short current_aux = 4100;
static unsigned short current_voltage = 4100;

/* This specifies the battery level that writes are still safe */
const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3600
};

/* Below this the player cannot be considered to operate reliably */
const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3580
};

/* Right now these are linear translations, it would be good to model them
 * appropriate to the actual battery curve.
 */

/* 6.10 format */

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3600, 3650, 3700, 3750, 3800, 3850, 3900, 3950, 4000, 4090, 4150 },
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    4000, 4105, 4210, 4315, 4420, 4525, 4630, 4735, 4840, 4945, 5050,
};
    
/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    short bat1, bat2, aux;
    static unsigned last_tick = 0;
    short tsadc;
    
    tsadc=tsc2100_readreg(TSADC_PAGE, TSADC_ADDRESS);
    
    /* Set the TSC2100 to read voltages if not busy with pen */
    if(!(tsadc & TSADC_PSTCM))
    {
        tsc2100_set_mode(true, 0x0B);
        last_tick = current_tick;
    }
    
    if(tsc2100_read_volt(&bat1, &bat2, &aux))
    { 
        /* Calculation was:
         *  (val << 10) / 4096 * 6 * 2.5 
         */
        current_voltage = (short)( (int) (bat1 * 15) >> 2 );
        current_bat2    = (short)( (bat2 * 15) >> 2 );
        current_aux     = (short)( (aux  * 15) >> 2 );
    }
        
    return current_voltage;
}

