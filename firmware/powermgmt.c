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
#include "ata.h"
#include "power.h"
#include "button.h"
#include "ata.h"
#include "mpeg.h"
#include "usb.h"
#include "powermgmt.h"
#include "backlight.h"

#ifdef SIMULATOR

int battery_level(void)
{
    return 100;
}

int battery_time(void)
{
    return 500;
}

bool battery_level_safe(void)
{
    return true;
}

void set_poweroff_timeout(int timeout)
{
    (void)timeout;
}

#else /* not SIMULATOR */

static int poweroff_idle_timeout_value[15] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 30, 45, 60
};

static int percent_to_volt_nocharge[11] = /* voltages (centivolt) of 0%, 10%, ... 100% when charging disabled */
{
    450, 481, 491, 497, 503, 507, 512, 514, 517, 528, 560
};

int battery_capacity = 1500;                 /* only a default value */

void set_battery_capacity(int capacity)
{
    battery_capacity = capacity;
    if ((battery_capacity > BATTERY_CAPACITY_MAX) || (battery_capacity < 1500))
        battery_capacity = 1500;
}

#ifdef HAVE_CHARGE_CTRL

char power_message[POWER_MESSAGE_LEN] = "";
char charge_restart_level = CHARGE_RESTART_HI;

int powermgmt_last_cycle_startstop_min = 20; /* how many minutes ago was the charging started or stopped? */
int powermgmt_last_cycle_level = 0;          /* which level had the batteries at this time? */
bool trickle_charge_enabled = true;
int trickle_sec = 0;                         /* how many seconds should the charger be enabled per minute for trickle charging? */
int charge_state = 0;                        /* at the beginning, the charger does nothing */

static int percent_to_volt_charge[11] = /* voltages (centivolt) of 0%, 10%, ... 100% when charging enabled */
{
    476, 544, 551, 556, 561, 564, 566, 576, 582, 584, 585
};

void enable_trickle_charge(bool on)
{
    trickle_charge_enabled = on;
}
#endif /* HAVE_CHARGE_CTRL */

int battery_lazyness[20] = /* how does the battery react when plugging in/out the charger */
{
    0, 17, 31, 42, 52, 60, 67, 72, 77, 81, 84, 87, 89, 91, 92, 94, 95, 95, 96, 97  
};

static char power_stack[DEFAULT_STACK_SIZE];
static char power_thread_name[] = "power";

static int poweroff_timeout = 0;
static long last_charge_time = 0;
int powermgmt_est_runningtime_min = -1;

static bool sleeptimer_active = false;
static unsigned long sleeptimer_endtick;

unsigned short power_history[POWER_HISTORY_LEN];


int battery_time(void)
{
    return powermgmt_est_runningtime_min;
}

/* look into the percent_to_volt_* table and get a realistic battery level percentage */
int voltage_to_percent(int voltage, int* table)
{
    if (voltage <= table[0])
        return 0;
    else
    if (voltage >= table[10])
        return 100;
    else {    
        /* search nearest value */
        int i = 0;
        while ((i < 10) && (table[i+1] < voltage))
            i++;
        /* interpolate linear between the smaller and greater value */
        return i * 10 /* 10th */
               + (voltage - table[i]) * 10 / (table[i+1] - table[i]); /* 1th */
    }
}

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

    if(level > BATTERY_LEVEL_FULL)
        level = BATTERY_LEVEL_FULL;

    if(level < BATTERY_LEVEL_EMPTY)
        level = BATTERY_LEVEL_EMPTY;

    /* level now stores the voltage in centivolts */
    /* let's calculate a percentage now with using the voltage arrays */

#ifdef HAVE_CHARGE_CTRL
    if (powermgmt_last_cycle_startstop_min < 20) {
        /* the batteries are lazy, so take a value between the result of the two table lookups */
        if (charge_state == 1)
            level = (voltage_to_percent(level, percent_to_volt_charge)
                     * battery_lazyness[powermgmt_last_cycle_startstop_min]
                     + voltage_to_percent(level, percent_to_volt_nocharge)
                     * (100 - battery_lazyness[powermgmt_last_cycle_startstop_min])) / 100;
        else
            level = (voltage_to_percent(level, percent_to_volt_nocharge)
                     * battery_lazyness[powermgmt_last_cycle_startstop_min]
                     + voltage_to_percent(level, percent_to_volt_charge)
                     * (100 - battery_lazyness[powermgmt_last_cycle_startstop_min])) / 100;
    } else {
        if (charge_state == 1)
            level = voltage_to_percent(level, percent_to_volt_charge);
        else
            level = voltage_to_percent(level, percent_to_volt_nocharge);
    }
#else
    level = voltage_to_percent(level, percent_to_volt_nocharge); /* always use the nocharge table */
#endif

    return level;
}

/* Tells if the battery level is safe for disk writes */
bool battery_level_safe(void)
{
    /* I'm pretty sure we don't want an average over a long time here */
    if (power_history[POWER_HISTORY_LEN-1])
        return power_history[POWER_HISTORY_LEN-1] > BATTERY_LEVEL_DANGEROUS;
    else
        return adc_read(ADC_UNREG_POWER) > (BATTERY_LEVEL_DANGEROUS * 10000) / BATTERY_SCALE_FACTOR;
}

void set_poweroff_timeout(int timeout)
{
    poweroff_timeout = timeout;
}

void set_sleep_timer(int seconds)
{
    if(seconds)
    {
        sleeptimer_active = true;
        sleeptimer_endtick = current_tick + seconds * HZ;
    }
    else
    {
        sleeptimer_active = false;
        sleeptimer_endtick = 0;
    }
}

int get_sleep_timer(void)
{
    if(sleeptimer_active)
        return (sleeptimer_endtick - current_tick) / HZ;
    else
        return 0;
}

/* We shut off in the following cases:
   1) The unit is idle, not playing music
   2) The unit is playing music, but is paused

   We do not shut off in the following cases:
   1) The USB is connected
   2) The charger is connected
   3) We are recording, or recording with pause
*/
static void handle_auto_poweroff(void)
{
    long timeout = poweroff_idle_timeout_value[poweroff_timeout]*60*HZ;
    int mpeg_stat = mpeg_status();
    bool charger_is_inserted = charger_inserted();
    static bool charger_was_inserted = false;

    /* The was_inserted thing prevents the unit to shut down immediately
       when the charger is extracted */
    if(charger_is_inserted || charger_was_inserted)
    {
        last_charge_time = current_tick;
    }
    charger_was_inserted = charger_is_inserted;
    
    if(timeout &&
       !usb_inserted() &&
       (mpeg_stat == 0 ||
        mpeg_stat == (MPEG_STATUS_PLAY | MPEG_STATUS_PAUSE)))
    {
        if(TIME_AFTER(current_tick, last_keypress + timeout) &&
           TIME_AFTER(current_tick, last_disk_activity + timeout) &&
           TIME_AFTER(current_tick, last_charge_time + timeout))
            power_off();
    }
    else
    {
        /* Handle sleeptimer */
        if(sleeptimer_endtick &&
           !usb_inserted())
        {
            if(TIME_AFTER(current_tick, sleeptimer_endtick))
            {
                if(charger_is_inserted)
                {
                    DEBUGF("Sleep timer timeout. Rebooting...\n");
                    system_reboot();
                }
                else
                {
                    DEBUGF("Sleep timer timeout. Shutting off...\n");
                    power_off();
                }
            }
        }
    }
}

/*
 * This power thread maintains a history of battery voltage
 * and implements a charging algorithm.
 * For a complete description of the charging algorithm read
 * docs/CHARGING_ALGORITHM.
 */


static void power_thread(void)
{
    int i;
    int avg, ok_samples, spin_samples;
    int current = 0;
#ifdef HAVE_CHARGE_CTRL
    int delta;
    int charged_time = 0;
    int charge_max_time_now = 0;
    int charge_pause = 0;         /* no charging pause at the beginning */
    int trickle_time = 0;         /* how many minutes trickle charging already? */
#endif
    
    while (1)
    {
        /* Make POWER_AVG measurements and calculate an average of that to
         * reduce the effect of backlights/disk spinning/other variation.
         */
        ok_samples = spin_samples = avg = 0;
        for (i = 0; i < POWER_AVG_N; i++) {
            if (ata_disk_is_active()) {
                if (!ok_samples) {
                    /* if we don't have any good non-disk-spinning samples,
                     * we take a sample anyway in case the disk is going
                     * to spin all the time.
                     */
                    avg += adc_read(ADC_UNREG_POWER);
                    spin_samples++;
                }
            } else {
                if (spin_samples) /* throw away disk-spinning samples */
                    spin_samples = avg = 0;
                avg += adc_read(ADC_UNREG_POWER);
                ok_samples++;
            }
            sleep(HZ*POWER_AVG_SLEEP);
        }
        avg = avg / ((ok_samples) ? ok_samples : spin_samples);
        
        /* rotate the power history */
        for (i = 0; i < POWER_HISTORY_LEN-1; i++)
            power_history[i] = power_history[i+1];
        
        /* insert new value in the end, in centivolts 8-) */
        power_history[POWER_HISTORY_LEN-1] = (avg * BATTERY_SCALE_FACTOR) / 10000;
        
        /* calculate estimated remaining running time */
        /* not charging: remaining running time */
        /* charging:     remaining charging time */

#ifdef HAVE_CHARGE_CTRL
        if (charge_state == 1)
            /* if taking the nocharge battery level, charging lasts 30% longer than the value says */
            /* so consider it because there's the battery lazyness inside the the battery_level */
            if (powermgmt_last_cycle_startstop_min < 20) {
                i = (100 - battery_lazyness[powermgmt_last_cycle_startstop_min]) * 30 / 100 ; /* 0..30 */
                powermgmt_est_runningtime_min = (100 - battery_level()) * battery_capacity / 100 * (100 + i) / 100 * 60 / CURRENT_CHARGING;
            } else {
                powermgmt_est_runningtime_min = (100 - battery_level()) * battery_capacity / 100 * 60 / CURRENT_CHARGING;
            }
        else {
            current = CURRENT_NORMAL;
            if ((backlight_get_timeout() == 1) || (charger_inserted() && backlight_get_on_when_charging()))
                                                       /* LED always on or LED on when charger connected */
                current += CURRENT_BACKLIGHT;
            powermgmt_est_runningtime_min = battery_level() * battery_capacity / 100 * 60 / current;
        }
#else
        current = CURRENT_NORMAL;
        if (backlight_get_timeout() == 1) /* LED always on */
            current += CURRENT_BACKLIGHT;
        powermgmt_est_runningtime_min = battery_level() * battery_capacity / 100 * 60 / current;
#endif
        
#ifdef HAVE_CHARGE_CTRL

        if (charge_pause > 0)
            charge_pause--;
        
        if (charger_inserted()) {
            if (charge_state == 1) {
                /* charger inserted and enabled */
                charged_time++;
                snprintf(power_message, POWER_MESSAGE_LEN, "Chg %dm max %dm", charged_time, charge_max_time_now);

                if (charged_time > charge_max_time_now) {
                    DEBUGF("power: charged_time > charge_max_time_now, enough!\n");
                    /* have charged too long and deltaV detection did not work! */
                    powermgmt_last_cycle_level = battery_level();
                    powermgmt_last_cycle_startstop_min = 0;
                    charger_enable(false);
                    snprintf(power_message, POWER_MESSAGE_LEN, "Chg tmout %d min", charge_max_time_now);
                    /* disable charging for several hours from this point, just to be sure */
                    charge_pause = CHARGE_PAUSE_LEN;
                    /* no trickle charge here, because the charging cycle didn't end the right way */
                    charge_state = 0; /* 0: decharging/charger off, 1: charge, 2: top-off, 3: trickle */
                } else {
                    if (charged_time > CHARGE_MIN_TIME) {
                        /* have charged continuously over the minimum charging time,
                         * so we monitor for deltaV going negative. Multiply things
                         * by 100 to get more accuracy without floating point arithmetic.
                         * power_history[] contains centivolts so after multiplying by 100
                         * the deltas are in tenths of millivolts (delta of 5 is
                         * 0.0005 V).
                         */
                        delta = ( power_history[POWER_HISTORY_LEN-1] * 100
                        	+ power_history[POWER_HISTORY_LEN-2] * 100
                        	- power_history[POWER_HISTORY_LEN-1-CHARGE_END_NEGD+1] * 100
                                - power_history[POWER_HISTORY_LEN-1-CHARGE_END_NEGD] * 100 )
                                / CHARGE_END_NEGD / 2;
                        
                        if (delta < -100) { /* delta < -10 mV */
                            DEBUGF("power: short-term negative delta, enough!\n");
                            powermgmt_last_cycle_level = battery_level();
                            powermgmt_last_cycle_startstop_min = 0;
                            charger_enable(false);
                            snprintf(power_message, POWER_MESSAGE_LEN, "end negd %d %dmin", delta, charged_time);
                            /* disable charging for several hours from this point, just to be sure */
                            charge_pause = CHARGE_PAUSE_LEN;
                            /* enable trickle charging */
                            if (trickle_charge_enabled) {
                                trickle_sec = CURRENT_NORMAL * 60 / CURRENT_CHARGING; /* first guess, maybe consider if LED backlight is on, disk is active,... */
                                trickle_time = 0;
                                charge_state = 2; /* 0: decharging/charger off, 1: charge, 2: top-off, 3: trickle */
                            } else {
                                charge_state = 0; /* 0: decharging/charger off, 1: charge, 2: top-off, 3: trickle */
                            }
                        } else {
                            /* if we didn't disable the charger in the previous test, check for low positive delta */
                            delta = ( power_history[POWER_HISTORY_LEN-1] * 100
                                    + power_history[POWER_HISTORY_LEN-2] * 100
                                    - power_history[POWER_HISTORY_LEN-1-CHARGE_END_ZEROD+1] * 100
                                    - power_history[POWER_HISTORY_LEN-1-CHARGE_END_ZEROD] * 100 )
                                    / CHARGE_END_ZEROD / 2;
                            
                            if (delta < 1) { /* delta < 0.1 mV */
                                DEBUGF("power: long-term small positive delta, enough!\n");
                                powermgmt_last_cycle_level = battery_level();
                                powermgmt_last_cycle_startstop_min = 0;
                                charger_enable(false);
                                snprintf(power_message, POWER_MESSAGE_LEN, "end lowd %d %dmin", delta, charged_time);
                                /* disable charging for several hours from this point, just to be sure */
                                charge_pause = CHARGE_PAUSE_LEN;
                                /* enable trickle charging */
                                if (trickle_charge_enabled) {
                                    trickle_sec = CURRENT_NORMAL * 60 / CURRENT_CHARGING; /* first guess, maybe consider if LED backlight is on, disk is active,... */
                                    trickle_time = 0;
                                    charge_state = 2; /* 0: decharging/charger off, 1: charge, 2: top-off, 3: trickle */
                                } else {
                                    charge_state = 0; /* 0: decharging/charger off, 1: charge, 2: top-off, 3: trickle */
                                }
                            }
                        }
                    }
                }
            } else if (charge_state > 1) { /* top off or trickle? */
                /* adjust trickle charge time */
                if (  ((charge_state == 2) && (power_history[POWER_HISTORY_LEN-1] > TOPOFF_VOLTAGE))
                   || ((charge_state == 3) && (power_history[POWER_HISTORY_LEN-1] > TRICKLE_VOLTAGE)) ) { /* charging too much */
                    trickle_sec--;
                } else { /* charging too less */
                    trickle_sec++;
                }
                    
                if (trickle_sec > 24) trickle_sec = 24;
                if (trickle_sec < 1)  trickle_sec = 1;

                /* charge the calculated amount of seconds */                
                charger_enable(true);
                sleep(HZ * trickle_sec);
                charger_enable(false);             

                /* trickle charging long enough? */
                
                if (trickle_time++ > TRICKLE_MAX_TIME + TOPOFF_MAX_TIME) {
                    trickle_sec = 0; /* show in debug menu that trickle is off */
                    charge_state = 0; /* 0: decharging/charger off, 1: charge, 2: top-off, 3: trickle */
                }

                if ((charge_state == 2) && (trickle_time > TOPOFF_MAX_TIME)) /* change state? */
                    charge_state = 3; /* 0: decharging/charger off, 1: charge, 2: top-off, 3: trickle */
            
            } else { /* charge_state == 0 */

                /* the charger is enabled here only in one case: if it was turned on at boot time (power_init) */
                /* turn it off now */
                if (charger_enabled)
                    charger_enable(false);

                /* if battery is not full, enable charging */
                if (battery_level() < charge_restart_level) {
                    if (charge_pause) {
                        DEBUGF("power: batt level < restart level, but charge pause, not enabling\n");
                        snprintf(power_message, POWER_MESSAGE_LEN, "chg pause %d min", charge_pause);
                    } else {
                        /* calculate max charge time depending on current battery level */
                        /* take 35% more because battery level is not linear */
                        i = CHARGE_MAX_TIME_1500 * battery_capacity / 1500;
                        charge_max_time_now = i * (100 + 35 - battery_level()) / 100;
                        if (charge_max_time_now > i) {
                          charge_max_time_now = i;
                        }
                        snprintf(power_message, POWER_MESSAGE_LEN, "ChgAt %d%% max %dm", battery_level(), charge_max_time_now);

                        /* enable the charger after the max time calc is done, because battery_level */
                        /* depends on if the charger is on */
                        DEBUGF("power: charger inserted and battery not full, enabling\n");
                        powermgmt_last_cycle_level = battery_level();
                        powermgmt_last_cycle_startstop_min = 0;
                        charged_time = 0;

                        charger_enable(true);
                        charge_state = 1; /* 0: decharging/charger off, 1: charge, 2: top-off, 3: trickle */
                        /* clear the power history so that we don't use values before
                         * discharge for the long-term delta
                         */
                        for (i = 0; i < POWER_HISTORY_LEN-1; i++)
                            power_history[i] = power_history[POWER_HISTORY_LEN-1];
                    }
                }
            }
        } else {
            /* charger not inserted */
            if (charge_state > 0) {
                /* charger not inserted but was enabled */
                DEBUGF("power: charger disconnected, disabling\n");
                powermgmt_last_cycle_level = battery_level();
                /* if the user only charged some minutes, we don't really have */
                /* a battery level that's usual for charging */
                /* the next line prevents the battery level being too low when the charger is connected for only some minutes */
                powermgmt_last_cycle_startstop_min = (powermgmt_last_cycle_startstop_min > 10) ? 0 : 20;
                /* show in debug menu that trickle is off */
                trickle_sec = 0;
                charger_enable(false);
                charge_state = 0; /* 0: decharging/charger off, 1: charge, 2: top-off, 3: trickle */
                snprintf(power_message, POWER_MESSAGE_LEN, "Charger disc");
            }
            /* charger not inserted and disabled, so we're discharging */
        }
        powermgmt_last_cycle_startstop_min++;
        
#endif /* HAVE_CHARGE_CTRL*/
        
        /* sleep for roughly a minute */
#ifdef HAVE_CHARGE_CTRL
        i = 60 - trickle_sec - POWER_AVG_N * POWER_AVG_SLEEP;
#else
        i = 60 - POWER_AVG_N * POWER_AVG_SLEEP;
#endif
        if (i > 0)
          sleep(HZ*(i));

        handle_auto_poweroff();
    }
}

void power_init(void)
{
    /* init history to 0 */
    memset(power_history, 0x00, sizeof(power_history));
    /* initialize the history with a single sample to prevent level
       flickering during the first minute of execution */
    power_history[POWER_HISTORY_LEN-1] = (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) / 10000;
    /* calculate the remaining time to that the info screen displays something useful */
    powermgmt_est_runningtime_min = battery_level() * battery_capacity / 100 * 60 / CURRENT_NORMAL;

#ifdef HAVE_CHARGE_CTRL
    snprintf(power_message, POWER_MESSAGE_LEN, "Powermgmt started");
    
    /* if the battery is nearly empty, start charging immediately */
    if (power_history[POWER_HISTORY_LEN-1] < BATTERY_LEVEL_DANGEROUS)
        charger_enable(true);
#endif
    create_thread(power_thread, power_stack, sizeof(power_stack), power_thread_name);
}

#endif /* SIMULATOR */

