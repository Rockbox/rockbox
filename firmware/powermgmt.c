/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Heikki Hannikainen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "sh7034.h"
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "debug.h"
#include "panic.h"
#include "adc.h"
#include "string.h"
#include "sprintf.h"
#include "power.h"
#include "powermgmt.h"

#ifdef SIMULATOR

int battery_level(void)
{
    return 100;
}

bool battery_level_safe(void)
{
    return true;
}

#else /* not SIMULATOR */

static char power_stack[DEFAULT_STACK_SIZE];
static char power_thread_name[] = "power";

unsigned short power_history[POWER_HISTORY_LEN];
#ifdef HAVE_CHARGE_CTRL
char power_message[POWER_MESSAGE_LEN] = "";
char charge_restart_level = CHARGE_RESTART_HI;
#endif

/* Returns battery level in percent */
int battery_level(void)
{
    int level = 0;
    int c = 0;
    int i;
    
    /* calculate average over last 3 minutes (skip empty samples) */
    for (i = 0; i < 3; i++)
        if (power_history[POWER_HISTORY_LEN-1-i]) {
            level += power_history[POWER_HISTORY_LEN-1-i];
            c++;
        }
        
    if (c)
        level = level / c;  /* avg */
    else /* history was empty, get a fresh sample */
        level = (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) / 10000;

#ifdef HAVE_CHARGE_CTRL    
    if (charger_enabled)
    	level -= 10;  /* the charger raises the voltage 0.05-0.2v */
#endif
    
    if(level > BATTERY_LEVEL_FULL)
        level = BATTERY_LEVEL_FULL;

    if(level < BATTERY_LEVEL_EMPTY)
        level = BATTERY_LEVEL_EMPTY;

    return ((level-BATTERY_LEVEL_EMPTY) * 100) / BATTERY_RANGE;
}

/* Tells if the battery level is safe for disk writes */
bool battery_level_safe(void)
{
    /* I'm pretty sure we don't want an average over a long time here */
    return adc_read(ADC_UNREG_POWER) > (BATTERY_LEVEL_DANGEROUS * 10000) / BATTERY_SCALE_FACTOR;
}

/*
 * This power thread maintains a history of battery voltage
 * and should, in the future, enable charging when it's needed
 * and power is available, and disable it when the battery is full.
 * Battery 'fullness' can be determined by the voltage drop, see:
 *
 * http://www.nimhbattery.com/nimhbattery-faq.htm questions 3 & 4
 * http://www.powerpacks-uk.com/Charging%20NiMh%20Batteries.htm
 * http://www.angelfire.com/electronic/hayles/charge1.html (soft start idea)
 * http://www.powerstream.com/NiMH.htm (discouraging)
 * http://www.panasonic.com/industrial/battery/oem/images/pdf/nimhchar.pdf
 * http://www.duracell.com/oem/Pdf/others/nimh_5.pdf (discharging)
 * http://www.duracell.com/oem/Pdf/others/nimh_6.pdf (charging)
 *
 * Charging logic which we're starting with (by Linus, Hes, Bagder):
 *
 *  1) max 16 hrs charge time (just in negative delta detection fails)
 *  2) Stop at negative delta of 5 mins
 *  3) Stop at 15 mins of zero-delta or below
 *  4) minimum of 15 mins charge time before 2) is applied
 *  5) after end of charging, wait for charge go down 80%
 *     before charging again if in 'no-use overnight charging mode'
 *     and down to 10% if in 'fixed-location mains-powered usage mode'
 */

static void power_thread(void)
{
    int i;
    int avg;
#ifdef HAVE_CHARGE_CTRL
    int delta;
    int charged_time = 0;
#endif
    
    while (1)
    {
        DEBUGF("power_thread woke up\n");
        /* make POWER_AVG measurements and calculate an average of that to
         * reduce the effect of backlights/disk spinning/other variation
         */
        avg = 0;
        for (i = 0; i < POWER_AVG; i++) {
            avg += adc_read(ADC_UNREG_POWER);
            sleep(15);
        }
        avg = avg / POWER_AVG;
        
        /* rotate the power history */
        for (i = 0; i < POWER_HISTORY_LEN-1; i++)
            power_history[i] = power_history[i+1];
        
        /* insert new value in the end, in decivolts 8-) */
        power_history[POWER_HISTORY_LEN-1] = (avg * BATTERY_SCALE_FACTOR) / 10000;
        
#ifdef HAVE_CHARGE_CTRL
        if (charger_inserted()) {
            if (charger_enabled) {
                /* charger inserted and enabled */
                charged_time++;
                if (charged_time > CHARGE_MAX_TIME) {
                    DEBUGF("power: charged_time > CHARGE_MAX_TIME, enough!\n");
                    /* have charged too long and deltaV detection did not work! */
                    charger_enable(false);
                    snprintf(power_message, POWER_MESSAGE_LEN, "Chg tmout %d min", CHARGE_MAX_TIME);
                    /* Perhaps we should disable charging for several
                       hours from this point, just to be sure. */
                } else {
                    if (charged_time > CHARGE_MIN_TIME) {
                        /* have charged continuously over the minimum charging time,
                         * so we monitor for deltaV going negative. Multiply things
                         * by 100 to get more accuracy without floating point arithmetic.
                         * power_history[] contains decivolts.
                         */
                        delta = 0;
                        for (i = 0; i < CHARGE_END_NEGD; i++)
                            delta += power_history[POWER_HISTORY_LEN-1-i]*100 - power_history[POWER_HISTORY_LEN-1-i-1]*100;
                        delta = delta / CHARGE_END_NEGD;
                        
                        if (delta < -50) { /* delta < -0.3 V */
                            DEBUGF("power: short-term negative delta, enough!\n");
                            charger_enable(false);
                            snprintf(power_message, POWER_MESSAGE_LEN, "end negd %d %dmin", delta, charged_time);
                        } else {
                            /* if we didn't disable the charger in the previous test, check for low positive delta */
                            delta = 0;
                            for (i = 0; i < CHARGE_END_ZEROD; i++)
                                delta += power_history[POWER_HISTORY_LEN-1-i]*100 - power_history[POWER_HISTORY_LEN-1-i-1]*100;
                            delta = delta / CHARGE_END_ZEROD;
                            
                            if (delta <= 5) { /* delta of <= 0.005 V */
                                DEBUGF("power: long-term small positive delta, enough!\n");
                                charger_enable(false);
                                snprintf(power_message, POWER_MESSAGE_LEN, "end lowd %d %dmin", delta, charged_time);
                            }
                        }
                    }
                }
            } else {
                /* charged inserted but not enabled */
                /* if battery is not full, enable charging */
                if (battery_level() < charge_restart_level) {
                    DEBUGF("power: charger inserted and battery not full, enabling\n");
                    charger_enable(true);
                    charged_time = 0;
                    snprintf(power_message, POWER_MESSAGE_LEN, "Chg started at %d%%", battery_level());
                }
            }
        } else {
            /* charger not inserted */
            if (charger_enabled) {
                /* charger not inserted but was enabled */
                DEBUGF("power: charger disconnected, disabling\n");
                charger_enable(false);
                snprintf(power_message, POWER_MESSAGE_LEN, "Charger disc");
            }
            /* charger not inserted and disabled, so we're discharging */
        }
#endif /* HAVE_CHARGE_CTRL*/
        
        /* sleep for roughly a minute */
        sleep(HZ*(60-POWER_AVG*15));
    }
}

void power_init(void)
{
    /* init history to 0 */
    memset(power_history, 0x00, sizeof(power_history));

#ifdef HAVE_CHARGE_CTRL
    snprintf(power_message, POWER_MESSAGE_LEN, "Powermgmt started");
#endif
    create_thread(power_thread, power_stack, sizeof(power_stack), power_thread_name);
}

#endif /* SIMULATOR */

