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
#include "pcf50606.h"

unsigned short current_voltage = 3910;

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    /* FIXME: calibrate value */
    3380
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    /* FIXME: calibrate value */
    3300
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    /* FIXME: calibrate values. Table is "inherited" from iPod-PCF / H100 */
    { 3370, 3650, 3700, 3740, 3780, 3820, 3870, 3930, 4000, 4080, 4160 }
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    /* FIXME: calibrate values. Table is "inherited" from iPod-PCF / H100 */
    3370, 3650, 3700, 3740, 3780, 3820, 3870, 3930, 4000, 4080, 4160
};
#endif /* CONFIG_CHARGING */

#define BATTERY_SCALE_FACTOR 6000
/* full-scale ADC readout (2^10) in millivolt */

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    static unsigned last_tick = 0;

    if (TIME_BEFORE(last_tick+HZ, current_tick))
    {
        int adc_val, irq_status;
        unsigned char buf[2];

        irq_status = disable_interrupt_save(IRQ_FIQ_DISABLED);
        pcf50606_write(PCF5060X_ADCC2, 0x1);
        pcf50606_read_multiple(PCF5060X_ADCS1, buf, 2);
        restore_interrupt(irq_status);

        adc_val = (buf[0]<<2) | (buf[1] & 3); //ADCDAT1H+ADCDAT1L
        current_voltage = (adc_val * BATTERY_SCALE_FACTOR) >> 10;

        last_tick = current_tick;
    }

    return current_voltage;
}

