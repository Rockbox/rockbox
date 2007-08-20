/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Heikki Hannikainen, Uwe Freese
 * Revisions copyright (C) 2005 by Gerald Van Baren
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

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
#ifdef IRIVER_H10
    3760
#elif defined IRIVER_H10_5GB
    3720
#endif
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
#ifdef IRIVER_H10
    3650
#elif defined IRIVER_H10_5GB
    3650
#endif
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
#ifdef IRIVER_H10
    { 3760, 3800, 3850, 3870, 3900, 3950, 4020, 4070, 4110, 4180, 4240 }
#elif defined IRIVER_H10_5GB
    { 3720, 3740, 3800, 3820, 3840, 3880, 3940, 4020, 4060, 4150, 4240 }
#endif
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
#ifdef IRIVER_H10
    3990, 4030, 4060, 4080, 4100, 4120, 4150, 4180, 4220, 4260, 4310
#elif defined IRIVER_H10_5GB
    /* TODO: Not yet calibrated */
    3880, 3920, 3960, 4000, 4060, 4100, 4150, 4190, 4240, 4280, 4330
#endif
};

#define BATTERY_SCALE_FACTOR 4800
/* full-scale ADC readout (2^10) in millivolt */

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    return (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) >> 10;
}
