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
#include "tsc2100.h"
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
    short tsadc = tsc2100_readreg(TSADC_PAGE, TSADC_ADDRESS);
    short adscm = (tsadc&TSADC_ADSCM_MASK)>>TSADC_ADSCM_SHIFT;
    if (adscm == 0xb) /* battery is available */
    {
        current_voltage = tsc2100_readreg(0, 5); /* BAT1 */
        tsc2100_readreg(0, 6); /* BAT2 */
        tsc2100_readreg(0, 7); /* AUX */
        /* reset the TSC2100 to read touches */
        tsadc &= ~(TSADC_PSTCM|TSADC_ADST|TSADC_ADSCM_MASK);
        tsadc |= TSADC_PSTCM|(0x2<<TSADC_ADSCM_SHIFT);
        tsc2100_writereg(TSADC_PAGE, TSADC_ADDRESS, tsadc);
        tsc2100_writereg(TSSTAT_PAGE, TSSTAT_ADDRESS, 2<<TSSTAT_PINTDAV_SHIFT);
    }
}
    
/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    static unsigned last_tick = 0;
    short tsadc = tsc2100_readreg(TSADC_PAGE, TSADC_ADDRESS);
    if (TIME_BEFORE(last_tick+2*HZ, current_tick))
    {
        tsadc &= ~(TSADC_PSTCM|TSADC_ADST|TSADC_ADSCM_MASK);
        tsadc |= 0xb<<TSADC_ADSCM_SHIFT;
        tsc2100_writereg(TSADC_PAGE, TSADC_ADDRESS, tsadc&(~(1<<15)));
        tsc2100_writereg(TSSTAT_PAGE, TSSTAT_ADDRESS, 2<<TSSTAT_PINTDAV_SHIFT);
        last_tick = current_tick;
    }
    else
        read_battery_inputs();
    return current_voltage;
}

