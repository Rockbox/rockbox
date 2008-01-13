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

/* FIXME: All voltages copied from H10/Tatung Elio. This will need changing
   proper power management. */

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3450 
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3400
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3480, 3550, 3590, 3610, 3630, 3650, 3700, 3760, 3800, 3910, 3990 },
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    3480, 3550, 3590, 3610, 3630, 3650, 3700, 3760, 3800, 3910, 3990 
};
#endif /* CONFIG_CHARGING */

#define BATTERY_SCALE_FACTOR 6003
/* full-scale ADC readout (2^10) in millivolt */

/* adc readout
 * max with charger connected: 690
 * max fully charged:          682
 * min just before shutdown:   570
 */

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    /* work around the inital (false) high readout */
    int readout=adc_read(ADC_UNREG_POWER);
    return (readout>700) ? 3480 : (readout * BATTERY_SCALE_FACTOR) >> 10;
}
