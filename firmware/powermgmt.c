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
#ifdef CONFIG_TUNER
#include "fmradio.h"
#endif

long last_event_tick;

void reset_poweroff_timer(void)
{
    last_event_tick = current_tick;
}

#ifdef SIMULATOR

int battery_level(void)
{
    return 75;
}

int battery_time(void)
{
    return 500;
}

bool battery_level_safe(void)
{
    return battery_level() >= 10;
}

void set_poweroff_timeout(int timeout)
{
    (void)timeout;
}

void set_battery_capacity(int capacity)
{
  (void)capacity;
}

void set_car_adapter_mode(bool setting)
{
    (void)setting;
}

#else /* not SIMULATOR */

int battery_capacity = BATTERY_CAPACITY_MIN; /* only a default value */
int battery_level_cached = -1; /* battery level of this minute, updated once
                                  per minute */
static bool car_adapter_mode_enabled = false;

static const int poweroff_idle_timeout_value[15] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 30, 45, 60
};

static const int percent_to_volt_decharge[11] =
/* voltages (centivolt) of 0%, 10%, ... 100% when charging disabled */
{
#if CONFIG_BATTERY == BATT_LIION2200
    /* measured values */
    260, 285, 295, 303, 311, 320, 330, 345, 360, 380, 400
#elif CONFIG_BATTERY == BATT_3AAA_ALKALINE
    /* taken from a textbook alkaline discharge graph, not measured */
    270, 303, 324, 336, 348, 357, 366, 378, 390, 408, 450
#else /* NiMH */
    /* original values were taken directly after charging, but it should show
       100% after turning off the device for some hours, too */
    450, 481, 491, 497, 503, 507, 512, 514, 517, 525, 540 /* orig. values:
                                                             ...,528,560 */
#endif
};

void set_battery_capacity(int capacity)
{
    battery_capacity = capacity;
    if (battery_capacity > BATTERY_CAPACITY_MAX)
        battery_capacity = BATTERY_CAPACITY_MAX;
    if (battery_capacity < BATTERY_CAPACITY_MIN)
        battery_capacity = BATTERY_CAPACITY_MIN;
}

#if defined(HAVE_CHARGE_CTRL) || CONFIG_BATTERY == BATT_LIION2200
int charge_state = 0;                          /* at the beginning, the
                                                  charger does nothing */
#endif

#ifdef HAVE_CHARGE_CTRL

char power_message[POWER_MESSAGE_LEN] = "";    /* message that's shown in
                                                  debug menu */
char charge_restart_level = CHARGE_RESTART_HI; /* level at which charging
                                                  starts */
int powermgmt_last_cycle_startstop_min = 25;   /* how many minutes ago was the
                                                  charging started or
                                                  stopped? */
int powermgmt_last_cycle_level = 0;            /* which level had the
                                                  batteries at this time? */
bool trickle_charge_enabled = true;
int trickle_sec = 0;                           /* how many seconds should the
                                                  charger be enabled per
                                                  minute for trickle
                                                  charging? */
static const int percent_to_volt_charge[11] = 
/* voltages (centivolt) of 0%, 10%, ... 100% when charging enabled */
{
    /* values guessed, see
       http://www.seattlerobotics.org/encoder/200210/LiIon2.pdf until someone
       measures voltages over a charging cycle */
    476, 544, 551, 556, 561, 564, 566, 576, 582, 584, 585 /* NiMH */
};

void enable_trickle_charge(bool on)
{
    trickle_charge_enabled = on;
}
#endif /* HAVE_CHARGE_CTRL */

static char power_stack[DEFAULT_STACK_SIZE];
static const char power_thread_name[] = "power";

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

/* look into the percent_to_volt_* table and get a realistic battery level
       percentage */
int voltage_to_percent(int voltage, const int* table)
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
                + (voltage - table[i]) *
                10 / (table[i+1] - table[i]); /* 1th */
        }
}

/* update battery level, called only once per minute */
void battery_level_update(void)
{
    int level = 0;
    int c = 0;
    int i;
    
    /* calculate maximum over last 3 minutes (skip empty samples) */
    for (i = 0; i < 3; i++)
        if (power_history[POWER_HISTORY_LEN-1-i] > c)
            c = power_history[POWER_HISTORY_LEN-1-i];
        
    if (c)
        level = c;
    else /* history was empty, get a fresh sample */
        level = (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) / 10000;

    if(level > BATTERY_LEVEL_FULL)
        level = BATTERY_LEVEL_FULL;

    if(level < BATTERY_LEVEL_EMPTY)
        level = BATTERY_LEVEL_EMPTY;

#ifdef HAVE_CHARGE_CTRL
    if (charge_state == 0) { /* decharge */
        level = voltage_to_percent(level, percent_to_volt_decharge);
    }
    else if (charge_state == 1) { /* charge */
        level = voltage_to_percent(level, percent_to_volt_charge);
    }
    else {/* in trickle charge, the battery is per definition 100% full */
        battery_level_cached = level = 100;
    }
#else
    level = voltage_to_percent(level, percent_to_volt_decharge);
    /* always use the decharge table */
#endif

#ifndef HAVE_MMC  /* this adjustment is only needed for HD based */
    if (battery_level_cached == -1) { /* first run of this procedure */
        /* the battery voltage is usually a little lower directly after
           turning on, because the disk was used heavily raise it by 5 % */
        battery_level_cached = (level > 95) ? 100 : level + 5;
    }
    else 
#endif
    {
        /* the level is allowed to be -1 of the last value when usb not
           connected and to be -3 of the last value when usb is connected */
        if (usb_inserted()) {
            if (level < battery_level_cached - 3)
                level = battery_level_cached - 3;
        }
        else {
            if (level < battery_level_cached - 1)
                level = battery_level_cached - 1;
        }
        battery_level_cached = level;
    }
}

/* Returns battery level in percent */
int battery_level(void)
{
#ifdef HAVE_CHARGE_CTRL
    if ((charge_state==1) && (battery_level_cached==100))
        return 99;
#endif
    return battery_level_cached;
}

/* Tells if the battery level is safe for disk writes */
bool battery_level_safe(void)
{
    /* I'm pretty sure we don't want an average over a long time here */
    if (power_history[POWER_HISTORY_LEN-1])
        return power_history[POWER_HISTORY_LEN-1] > BATTERY_LEVEL_DANGEROUS;
    else
        return adc_read(ADC_UNREG_POWER) > (BATTERY_LEVEL_DANGEROUS * 10000) /
            BATTERY_SCALE_FACTOR;
}

void set_poweroff_timeout(int timeout)
{
    poweroff_timeout = timeout;
}

void set_sleep_timer(int seconds)
{
    if(seconds) {
        sleeptimer_active = true;
        sleeptimer_endtick = current_tick + seconds * HZ;
    }
    else {
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
#ifdef CONFIG_TUNER
       !fmradio_get_status() &&
#endif
       !usb_inserted() &&
       (mpeg_stat == 0 ||
        ((mpeg_stat == (MPEG_STATUS_PLAY | MPEG_STATUS_PAUSE)) &&
         !sleeptimer_active)))
    {
        if(TIME_AFTER(current_tick, last_event_tick + timeout) &&
           TIME_AFTER(current_tick, last_disk_activity + timeout) &&
           TIME_AFTER(current_tick, last_charge_time + timeout))
        {
            power_off();
        }
    }
    else
    {
        /* Handle sleeptimer */
        if(sleeptimer_active && !usb_inserted())
        {
            if(TIME_AFTER(current_tick, sleeptimer_endtick))
            {
                mpeg_stop();
                if(charger_is_inserted)
                {
                    DEBUGF("Sleep timer timeout. Stopping...\n");
                    set_sleep_timer(0);
                    backlight_off(); /* Nighty, nighty... */
                }
                else
                {
                    DEBUGF("Sleep timer timeout. Shutting off...\n");
                    /* Make sure that the disk isn't spinning when
                       we cut the power */
                    while(ata_disk_is_active())
                       sleep(HZ);
                    power_off();
                }
            }
        }
    }
}

void set_car_adapter_mode(bool setting)
{
    car_adapter_mode_enabled = setting;
}

static bool charger_power_is_on;

#ifdef HAVE_CHARGING
static void car_adapter_mode_processing(void)
{
    static bool waiting_to_resume_play = false;
    static long play_resume_time;
    
    if (car_adapter_mode_enabled) {

        if (waiting_to_resume_play) {
            if (TIME_AFTER(current_tick, play_resume_time)) {
                if (mpeg_status() & MPEG_STATUS_PAUSE) {
                    mpeg_resume(); 
                }
                waiting_to_resume_play = false;
            }
        }
        else {
            if (charger_power_is_on) {

                /* if external power was turned off */
                if (!charger_inserted()) { 

                    charger_power_is_on = false;

                    /* if playing */
                    if ((mpeg_status() & MPEG_STATUS_PLAY) &&
                        !(mpeg_status() & MPEG_STATUS_PAUSE)) {
                        mpeg_pause(); 
                    }
                }
            }
            else { 
                /* if external power was turned on */
                if (charger_inserted()) { 

                    charger_power_is_on = true;

                    /* if paused */
                    if (mpeg_status() & MPEG_STATUS_PAUSE) {
                        /* delay resume a bit while the engine is cranking */
                        play_resume_time = current_tick + HZ*5;
                        waiting_to_resume_play = true;
                    }
                }
            }
        }
    }
}
#endif

/*
 * This function is called to do the relativly long sleep waits from within the
 * main power_thread loop while at the same time servicing any other periodic
 * functions in the power thread which need to be called at a faster periodic
 * rate than the slow periodic rate of the main power_thread loop
 */
static void power_thread_sleep(int ticks)
{
#ifdef HAVE_CHARGING
    while (ticks > 0) {
        int small_ticks = MIN(HZ/2, ticks);
        sleep(small_ticks);
        ticks -= small_ticks;

        car_adapter_mode_processing();
    }
#else
    sleep(ticks);  /* no fast-processing functions, sleep the whole time */
#endif
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
#if CONFIG_BATTERY == BATT_LIION2200
    int charging_current;
#endif
#ifdef HAVE_CHARGE_CTRL
    int delta;
    int charged_time = 0;
    int charge_max_time_now = 0;  /* max. charging duration, calculated at
                                     beginning of charging */
    int charge_pause = 0;         /* no charging pause at the beginning */
    int trickle_time = 0;         /* how many minutes trickle charging already? */
#endif
    
    while (1)
    {
        /* never read power while disk is spinning, unless in USB mode */
        if (ata_disk_is_active() && !usb_inserted()) {
            sleep(HZ * 2);
            continue;
        }
        
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
            power_thread_sleep(HZ*POWER_AVG_SLEEP);
        }
        avg = avg / ((ok_samples) ? ok_samples : spin_samples);

        /* rotate the power history */
        for (i = 0; i < POWER_HISTORY_LEN-1; i++)
            power_history[i] = power_history[i+1];
        
        /* insert new value in the end, in centivolts 8-) */
        power_history[POWER_HISTORY_LEN-1] =
            (avg * BATTERY_SCALE_FACTOR) / 10000;
        
        /* update battery level every minute, ignoring first 15 minutes after
           start charge/decharge */
#ifdef HAVE_CHARGE_CTRL
        if ((powermgmt_last_cycle_startstop_min > 25) || (charge_state > 1))
#endif
            battery_level_update();

        /* calculate estimated remaining running time */
        /* decharging: remaining running time */
        /* charging:   remaining charging time */

#ifdef HAVE_CHARGE_CTRL
        if (charge_state == 1)
            powermgmt_est_runningtime_min = (100 - battery_level()) *
                battery_capacity / 100 * 60 / CURRENT_CHARGING;
        else {
            current = usb_inserted() ? CURRENT_USB : CURRENT_NORMAL;
            if ((backlight_get_timeout() == 1) ||
                (charger_inserted() && backlight_get_on_when_charging()))
                /* LED always on or LED on when charger connected */
                current += CURRENT_BACKLIGHT;
            powermgmt_est_runningtime_min = battery_level() *
                battery_capacity / 100 * 60 / current;
#if MEM == 8 /* assuming 192 kbps, the running time is 22% longer with 8MB */
            powermgmt_est_runningtime_min =
                powermgmt_est_runningtime_min * 122 / 100;
#endif /* MEM == 8 */
        }
#else
        current = usb_inserted() ? CURRENT_USB : CURRENT_NORMAL;
        if (backlight_get_timeout() == 1) /* LED always on */
            current += CURRENT_BACKLIGHT;
        powermgmt_est_runningtime_min = battery_level() *
            battery_capacity / 100 * 60 / current;
#if MEM == 8 /* assuming 192 kbps, the running time is 22% longer with 8MB */
        powermgmt_est_runningtime_min =
            powermgmt_est_runningtime_min * 122 / 100;
#endif /* MEM == 8 */
#endif /* HAVE_CHARGE_CONTROL */

#if CONFIG_BATTERY == BATT_LIION2200
        /* We use the information from the ADC_EXT_POWER ADC channel, which
           tells us the charging current from the LTC1734. When DC is
           connected (either via the external adapter, or via USB), we try
           to determine if it is actively charging or only maintaining the
           charge. My tests show that ADC readings is below about 0x80 means
           that the LTC1734 is only maintaining the charge. */
        if(charger_inserted()) {
            charging_current = adc_read(ADC_EXT_POWER);
            if(charging_current < 0x80) {
                charge_state = 2; /* Trickle */
            } else {
                charge_state = 1; /* Charging */
            }
        } else {
            charge_state = 0; /* Not charging */
        }
#else
#ifdef HAVE_CHARGE_CTRL

        if (charge_pause > 0)
            charge_pause--;
        
        if (charger_inserted()) {
            if (charge_state == 1) {
                /* charger inserted and enabled */
                charged_time++;
                snprintf(power_message, POWER_MESSAGE_LEN,
                         "Chg %dm max %dm", charged_time, charge_max_time_now);

                if (charged_time > charge_max_time_now) {
                    DEBUGF("power: charged_time > charge_max_time_now, "
                           "enough!\n");
                    /* have charged too long and deltaV detection did not
                       work! */
                    powermgmt_last_cycle_level = battery_level();
                    powermgmt_last_cycle_startstop_min = 0;
                    charger_enable(false);
                    snprintf(power_message, POWER_MESSAGE_LEN,
                             "Chg tmout %d min", charge_max_time_now);
                    /* disable charging for several hours from this point,
                       just to be sure */
                    charge_pause = CHARGE_PAUSE_LEN;
                    /* no trickle charge here, because the charging cycle
                       didn't end the right way */
                    charge_state = 0; /* 0: decharging/charger off, 1: charge,
                                         2: top-off, 3: trickle */
                } else {
                    if (charged_time > CHARGE_MIN_TIME) {
                        /* have charged continuously over the minimum charging
                           time, so we monitor for deltaV going
                           negative. Multiply thingsby 100 to get more
                           accuracy without floating point arithmetic.
                           power_history[] contains centivolts so after
                           multiplying by 100 the deltas are in tenths of
                           millivolts (delta of 5 is 0.0005 V).
                         */
                        delta =
                            ( power_history[POWER_HISTORY_LEN-1] * 100
                              + power_history[POWER_HISTORY_LEN-2] * 100
                              - power_history[POWER_HISTORY_LEN-1-
                                              CHARGE_END_NEGD+1] * 100
                              - power_history[POWER_HISTORY_LEN-1-
                                              CHARGE_END_NEGD] * 100 )
                            / CHARGE_END_NEGD / 2;
                        
                        if (delta < -100) { /* delta < -10 mV */
                            DEBUGF("power: short-term negative"
                                   " delta, enough!\n");
                            powermgmt_last_cycle_level = battery_level();
                            powermgmt_last_cycle_startstop_min = 0;
                            charger_enable(false);
                            snprintf(power_message, POWER_MESSAGE_LEN,
                                     "end negd %d %dmin", delta, charged_time);
                            /* disable charging for several hours from this
                               point, just to be sure */
                            charge_pause = CHARGE_PAUSE_LEN;
                            /* enable trickle charging */
                            if (trickle_charge_enabled) {
                                trickle_sec = CURRENT_NORMAL * 60 /
                                    CURRENT_CHARGING;
                                /* first guess, maybe consider if LED
                                   backlight is on, disk is active,... */
                                trickle_time = 0;
                                charge_state = 2; /* 0: decharging/charger
                                                     off, 1: charge,
                                                     2: top-off, 3: trickle */
                            } else {
                                charge_state = 0; /* 0: decharging/charger
                                                     off, 1: charge,
                                                     2: top-off, 3: trickle */
                            }
                        } else {
                            /* if we didn't disable the charger in the
                               previous test, check for low positive delta */
                            delta =
                                ( power_history[POWER_HISTORY_LEN-1] * 100
                                  + power_history[POWER_HISTORY_LEN-2] * 100
                                  - power_history[POWER_HISTORY_LEN-1-
                                                  CHARGE_END_ZEROD+1] * 100
                                  - power_history[POWER_HISTORY_LEN-1-
                                                  CHARGE_END_ZEROD] * 100 )
                                / CHARGE_END_ZEROD / 2;
                            
                            if (delta < 1) { /* delta < 0.1 mV */
                                DEBUGF("power: long-term small "
                                       "positive delta, enough!\n");
                                powermgmt_last_cycle_level = battery_level();
                                powermgmt_last_cycle_startstop_min = 0;
                                charger_enable(false);
                                snprintf(power_message, POWER_MESSAGE_LEN,
                                         "end lowd %d %dmin",
                                         delta, charged_time);
                                /* disable charging for several hours from
                                   this point, just to be sure */
                                charge_pause = CHARGE_PAUSE_LEN;
                                /* enable trickle charging */
                                if (trickle_charge_enabled) {
                                    trickle_sec =
                                        CURRENT_NORMAL * 60 / CURRENT_CHARGING;
                                    /* first guess, maybe consider if LED
                                       backlight is on, disk is active,... */
                                    trickle_time = 0;
                                    charge_state = 2;
                                    /* 0: decharging/charger off, 1: charge,
                                       2: top-off, 3: trickle */
                                } else {
                                    charge_state = 0;
                                    /* 0: decharging/charger off, 1: charge,
                                       2: top-off, 3: trickle */
                                }
                            }
                        }
                    }
                }
            }
            else if (charge_state > 1) { /* top off or trickle? */
                /* adjust trickle charge time */
                if (  ((charge_state == 2) &&
                       (power_history[POWER_HISTORY_LEN-1] > TOPOFF_VOLTAGE))
                   || ((charge_state == 3) &&
                       (power_history[POWER_HISTORY_LEN-1] >
                        TRICKLE_VOLTAGE)) ) { /* charging too much */
                    trickle_sec--;
                }
                else { /* charging too less */
                    trickle_sec++;
                }
                    
                if (trickle_sec > 24)
                    trickle_sec = 24;

                if (trickle_sec < 1)
                    trickle_sec = 1;

                /* charge the calculated amount of seconds */                
                charger_enable(true);
                power_thread_sleep(HZ * trickle_sec);
                charger_enable(false);             

                /* trickle charging long enough? */
                
                if (trickle_time++ > TRICKLE_MAX_TIME + TOPOFF_MAX_TIME) {
                    trickle_sec = 0; /* show in debug menu that trickle is
                                        off */
                    charge_state = 0; /* 0: decharging/charger off, 1: charge,
                                         2: top-off, 3: trickle */
                    powermgmt_last_cycle_startstop_min = 0;
                }

                if ((charge_state == 2) &&
                    (trickle_time > TOPOFF_MAX_TIME)) /* change state? */
                    charge_state = 3; /* 0: decharging/charger off, 1: charge,
                                         2: top-off, 3: trickle */
            
            } else { /* charge_state == 0 */

                /* the charger is enabled here only in one case: if it was
                   turned on at boot time (power_init) */
                /* turn it off now */
                if (charger_enabled)
                    charger_enable(false);
            }

            /* Start new charge cycle? This must be possible also in
               trickle/top-off, because when usb connected, */
            /* the trickle charge amount may not be enough */
            
            if ((charge_state == 0) || (charge_state > 1))
                /* if battery is not full, enable charging */
                /* make sure charging starts if 1%-lazyness in
                   battery_level_update() is too slow */
                if (    (battery_level() < charge_restart_level)
                        || (power_history[POWER_HISTORY_LEN-1] <
                            BATTERY_LEVEL_DANGEROUS)) {
                    if (charge_pause) {
                        DEBUGF("power: batt level < restart level,"
                               " but charge pause, not enabling\n");
                        snprintf(power_message, POWER_MESSAGE_LEN,
                                 "chg pause %d min", charge_pause);
                    } else {
                        /* calculate max charge time depending on current
                           battery level */
                        /* take 35% more because some more energy is used for
                           heating up the battery */
                        i = CHARGE_MAX_TIME_1500 * battery_capacity / 1500;
                        charge_max_time_now =
                            i * (100 + 35 - battery_level()) / 100;
                        if (charge_max_time_now > i) {
                            charge_max_time_now = i;
                        }
                        snprintf(power_message, POWER_MESSAGE_LEN,
                                 "ChgAt %d%% max %dm", battery_level(),
                                 charge_max_time_now);

                        /* enable the charger after the max time calc is done,
                           because battery_level depends on if the charger is
                           on */
                        DEBUGF("power: charger inserted and battery"
                               " not full, enabling\n");
                        powermgmt_last_cycle_level = battery_level();
                        powermgmt_last_cycle_startstop_min = 0;
                        charged_time = 0;

                        charger_enable(true);
                        charge_state = 1; /* 0: decharging/charger off, 1:
                                             charge, 2: top-off, 3: trickle */
                        /* clear the power history so that we don't use values
                           before discharge for the long-term delta
                         */
                        for (i = 0; i < POWER_HISTORY_LEN-1; i++)
                            power_history[i] =
                                power_history[POWER_HISTORY_LEN-1];
                    }
                }

        } else {
            /* charger not inserted */
            if (charge_state > 0) {
                /* charger not inserted but was enabled */
                DEBUGF("power: charger disconnected, disabling\n");
                powermgmt_last_cycle_level = battery_level();
                powermgmt_last_cycle_startstop_min = 0;
                /* show in debug menu that trickle is off */
                trickle_sec = 0;
                charger_enable(false);
                charge_state = 0; /* 0: decharging/charger off, 1: charge, 2:
                                     top-off, 3: trickle */
                snprintf(power_message, POWER_MESSAGE_LEN, "Charger disc");
            }
            /* charger not inserted and disabled, so we're discharging */
        }
        powermgmt_last_cycle_startstop_min++;
        
#endif /* HAVE_CHARGE_CTRL*/
#endif /* # if CONFIG_BATTERY == BATT_LIION2200 */
        
        /* sleep for roughly a minute */
#ifdef HAVE_CHARGE_CTRL
        i = 60 - trickle_sec - POWER_AVG_N * POWER_AVG_SLEEP;
#else
        i = 60 - POWER_AVG_N * POWER_AVG_SLEEP;
#endif
        if (i > 0)
            power_thread_sleep(HZ*(i));

        handle_auto_poweroff();
    }
}

void powermgmt_init(void)
{
    power_init();
    
    /* init history to 0 */
    memset(power_history, 0x00, sizeof(power_history));

#if 0
    /* initialize the history with a single sample to prevent level
       flickering during the first minute of execution */
    power_history[POWER_HISTORY_LEN-1] =
        (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) / 10000;
    /* calculate the first battery level */
    battery_level_update();
    /* calculate the remaining time to that the info screen displays something
       useful */
    powermgmt_est_runningtime_min =
        battery_level() * battery_capacity / 100 * 60 / CURRENT_NORMAL;
#if MEM == 8 /* assuming 192 kbps, the running time is 22% longer with 8MB */
    powermgmt_est_runningtime_min = powermgmt_est_runningtime_min * 122 / 100;
#endif
    
#ifdef HAVE_CHARGE_CTRL
    snprintf(power_message, POWER_MESSAGE_LEN, "Powermgmt started");
    
    /* if the battery is nearly empty, start charging immediately */
    if (power_history[POWER_HISTORY_LEN-1] < BATTERY_LEVEL_DANGEROUS)
        charger_enable(true);
#endif
#endif

    charger_power_is_on = charger_inserted();
    
    create_thread(power_thread, power_stack, sizeof(power_stack),
                  power_thread_name);
}

#endif /* SIMULATOR */

