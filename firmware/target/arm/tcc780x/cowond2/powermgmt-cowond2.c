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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "adc.h"
#include "powermgmt.h"
#include "kernel.h"

unsigned short current_voltage = 3910;
const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    0
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    0
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 100, 300, 400, 500, 600, 700, 800, 900, 1000, 1200, 1320 },
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    100, 300, 400, 500, 600, 700, 800, 900, 1000, 1200, 1320,
};

void read_battery_inputs(void)
{
    #warning function not implemented
}
    
/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    #warning function not implemented
    return 0;
}

