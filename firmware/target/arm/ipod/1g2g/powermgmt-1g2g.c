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
#include "hwcompat.h"

/* FIXME: Properly calibrate values. Current values "inherited" from
 * iriver H100 */

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3380
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3020
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3370, 3650, 3700, 3740, 3780, 3820, 3870, 3930, 4000, 4080, 4160 }
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    3540, 3860, 3930, 3980, 4000, 4020, 4040, 4080, 4130, 4180, 4230
};
#endif /* CONFIG_CHARGING */

#define BATTERY_SCALE_FACTOR_1G 4200
#define BATTERY_SCALE_FACTOR_2G 6630
/* full-scale ADC readout (2^8) in millivolt */

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    unsigned adcval = adc_read(ADC_UNREG_POWER);

    if ((IPOD_HW_REVISION >> 16) == 1)
        return (adcval * BATTERY_SCALE_FACTOR_1G) >> 8;
    else
        return (adcval * BATTERY_SCALE_FACTOR_2G) >> 8;
}
