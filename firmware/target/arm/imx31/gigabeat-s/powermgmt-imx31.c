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

/* FIXME: This is just the Gigabeat F/X file with a different name... */

#include "config.h"
#include "adc.h"
#include "powermgmt.h"

/**
 * Fixed-point natural log
 * taken from http://www.quinapalus.com/efunc.html
 *  "The code assumes integers are at least 32 bits long. The (positive)
 *   argument and the result of the function are both expressed as fixed-point
 *   values with 16 fractional bits, although intermediates are kept with 28
 *   bits of precision to avoid loss of accuracy during shifts."
 */
static long flog(int x)
{
    long t,y;

    y=0xa65af;
    if(x<0x00008000) x<<=16,              y-=0xb1721;
    if(x<0x00800000) x<<= 8,              y-=0x58b91;
    if(x<0x08000000) x<<= 4,              y-=0x2c5c8;
    if(x<0x20000000) x<<= 2,              y-=0x162e4;
    if(x<0x40000000) x<<= 1,              y-=0x0b172;
    t=x+(x>>1); if((t&0x80000000)==0) x=t,y-=0x067cd;
    t=x+(x>>2); if((t&0x80000000)==0) x=t,y-=0x03920;
    t=x+(x>>3); if((t&0x80000000)==0) x=t,y-=0x01e27;
    t=x+(x>>4); if((t&0x80000000)==0) x=t,y-=0x00f85;
    t=x+(x>>5); if((t&0x80000000)==0) x=t,y-=0x007e1;
    t=x+(x>>6); if((t&0x80000000)==0) x=t,y-=0x003f8;
    t=x+(x>>7); if((t&0x80000000)==0) x=t,y-=0x001fe;
    x=0x80000000-x;
    y-=x>>15;
    return y;
}


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
    /* Toshiba Gigabeat Li Ion 830mAH figured from discharge curve */
    { 3480, 3550, 3590, 3610, 3630, 3650, 3700, 3760, 3800, 3910, 3990 },
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    /* Toshiba Gigabeat Li Ion 830mAH */
    3480, 3550, 3590, 3610, 3630, 3650, 3700, 3760, 3800, 3910, 3990
};

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    /* ADC reading 0-1023 = 2400mV-4700mV */
    return ((adc_read(ADC_BATTERY) * 2303) >> 10) + 2400;
}

/* Returns battery charge current from ADC [milliamps] */
int battery_adc_charge_current(void)
{
    /* Positive reading = charger to battery
     * Negative reading = battery to charger terminal
     * ADC reading -512-511 = -2875mA-2875mA */
    unsigned int value = adc_read(ADC_CHARGER_CURRENT);
    return (((int)value << 22) >> 22) * 2881 >> 9;
}

/* Returns battery temperature from ADC [deg-C] */
unsigned int battery_adc_temp(void)
{
    unsigned int value = adc_read(ADC_BATTERY_TEMP);
    /* E[volts] = value * 2.3V / 1023
     * R[ohms] = E/I = E[volts] / 0.00002[A] (Thermistor bias current source)
     *
     * Steinhart-Hart thermistor equation (sans "C*ln^2(R)" term because it
     * has negligible effect):
     * [A + B*ln(R) + D*ln^3(R)] = 1 / T[°K]
     *
     * Coeffients that fit experimental data (one thermistor so far, one run):
     * A = 0.0013002631685462800
     * B = 0.0002000841932612330
     * D = 0.0000000640446750919
     *
     * Fixed-point output matches the floating-point version for each ADC
     * value.
     */
     int R = 2070000 * value;
     long long ln = flog(R) + 83196;
     long long t0 = 425890304133ll;
     long long t1 = 1000000*ln;
     long long t3 = ln*ln*ln / 13418057;
     return ((32754211579494400ll / (t0 + t1 + t3)) - 27315) / 100;
}
