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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "system.h"
#include <sprintf.h>
#include "debug.h"
#include "storage.h"
#include "adc.h"
#include "power.h"
#include "powermgmt.h"

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    4750
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    4400
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    /* original values were taken directly after charging, but it should show
       100% after turning off the device for some hours, too */
    { 4500, 4810, 4910, 4970, 5030, 5070, 5120, 5140, 5170, 5250, 5400 }
                                            /* orig. values: ...,5280,5600 */
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    /* values guessed, see
       http://www.seattlerobotics.org/encoder/200210/LiIon2.pdf until someone
       measures voltages over a charging cycle */
    4760, 5440, 5510, 5560, 5610, 5640, 5660, 5760, 5820, 5840, 5850 /* NiMH */
};

#define BATTERY_SCALE_FACTOR 6620
/* full-scale ADC readout (2^10) in millivolt */

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    return (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) >> 10;
}

/** Charger control **/
#ifdef CHARGING_DEBUG_FILE
#include "file.h"
#define DEBUG_FILE_NAME "/powermgmt.csv"
#define DEBUG_MESSAGE_LEN 133
static char debug_message[DEBUG_MESSAGE_LEN];
static int fd = -1;          /* write debug information to this file */
static int wrcount = 0;
#endif /* CHARGING_DEBUG_FILE */

/*
 * For a complete description of the charging algorithm read
 * docs/CHARGING_ALGORITHM.
 */
int long_delta;                     /* long term delta battery voltage */
int short_delta;                    /* short term delta battery voltage */
bool disk_activity_last_cycle = false;  /* flag set to aid charger time
                                         * calculation */
char power_message[POWER_MESSAGE_LEN] = ""; /* message that's shown in
                                           debug menu */
                                        /* percentage at which charging
                                           starts */
int powermgmt_last_cycle_startstop_min = 0; /* how many minutes ago was the
                                           charging started or
                                           stopped? */
int powermgmt_last_cycle_level = 0;     /* which level had the
                                           batteries at this time? */
int trickle_sec = 0;                    /* how many seconds should the
                                           charger be enabled per
                                           minute for trickle
                                           charging? */
int pid_p = 0;                      /* PID proportional term */
int pid_i = 0;                      /* PID integral term */

static unsigned int target_voltage = TRICKLE_VOLTAGE; /* desired topoff/trickle
                                                       * voltage level */
static int charge_max_time_idle = 0; /* max. charging duration, calculated at
                                      * beginning of charging */
static int charge_max_time_now = 0; /* max. charging duration including
                                     * hdd activity */
static int minutes_disk_activity = 0; /* count minutes of hdd use during
                                       * charging */
static int last_disk_activity = CHARGE_END_LONGD + 1; /* last hdd use x mins ago */

#ifdef CHARGING_DEBUG_FILE
static void debug_file_close(void)
{
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

static void debug_file_log(void)
{
    if (usb_inserted()) {
        /* It is probably too late to close the file but we can try... */
        debug_file_close();
    }
    else if (fd < 0) {
        fd = open(DEBUG_FILE_NAME, O_WRONLY | O_APPEND | O_CREAT);

        if (fd >= 0) {
            snprintf(debug_message, DEBUG_MESSAGE_LEN,
            "cycle_min, bat_millivolts, bat_percent, chgr_state"
            " ,charge_state, pid_p, pid_i, trickle_sec\n");
            write(fd, debug_message, strlen(debug_message));
            wrcount = 99;   /* force a flush */
        }
    }
    else {
        snprintf(debug_message, DEBUG_MESSAGE_LEN,
                "%d, %d, %d, %d, %d, %d, %d, %d\n",
            powermgmt_last_cycle_startstop_min, battery_voltage(),
            battery_level(), charger_input_state, charge_state,
            pid_p, pid_i, trickle_sec);
        write(fd, debug_message, strlen(debug_message));
        wrcount++;
    }
}

static void debug_file_sync(void)
{
    /*
     * If we have a lot of pending writes or if the disk is spining,
     * fsync the debug log file.
     */
    if (wrcount > 10 || (wrcount > 0 && storage_disk_is_active())) {
        if (fd >= 0)
            fsync(fd);

        wrcount = 0;
    }
}
#else /* !CHARGING_DEBUG_FILE */
#define debug_file_close()
#define debug_file_log()
#define debug_file_sync()
#endif /* CHARGING_DEBUG_FILE */

/*
 * Do tasks that should be done every step.
 */
static void do_frequent_tasks(void)
{
    if (storage_disk_is_active()) {
        /* flag hdd use for charging calculation */
        disk_activity_last_cycle = true;
    }

    debug_file_sync();
}

/*
 * The charger was just plugged in.  If the battery level is
 * nearly charged, just trickle.  If the battery is low, start
 * a full charge cycle.  If the battery level is in between,
 * top-off and then trickle.
 */
static void charger_plugged(void)
{
    int battery_percent = battery_level();

    pid_p = 0;
    pid_i = 0;
    powermgmt_last_cycle_level = battery_percent;
    powermgmt_last_cycle_startstop_min = 0;

    snprintf(power_message, POWER_MESSAGE_LEN, "Charger plugged in");

    if (battery_percent > START_TOPOFF_CHG) {

        if (battery_percent >= START_TRICKLE_CHG) {
            charge_state = TRICKLE;
            target_voltage = TRICKLE_VOLTAGE;
        }
        else {
            charge_state = TOPOFF;
            target_voltage = TOPOFF_VOLTAGE;
        }
    }
    else {
        /*
         * Start the charger full strength
         */
        int i = CHARGE_MAX_MIN_1500 * get_battery_capacity() / 1500;
        charge_max_time_idle = i * (100 + 35 - battery_percent) / 100;

        if (charge_max_time_idle > i)
            charge_max_time_idle = i;

        charge_max_time_now = charge_max_time_idle;

        snprintf(power_message, POWER_MESSAGE_LEN,
                 "ChgAt %d%% max %dm", battery_percent,
                 charge_max_time_now);

        /*
         * Enable the charger after the max time calc is done,
         * because battery_level depends on if the charger is
         * on.
         */
        DEBUGF("power: charger inserted and battery"
               " not full, charging\n");
        trickle_sec  = 60;
        long_delta   = short_delta = 999999;
        charge_state = CHARGING;
    }
}

/*
 * The charger was just unplugged.
 */
static void charger_unplugged(void)
{
    DEBUGF("power: charger disconnected, disabling\n");

    charger_enable(false);
    powermgmt_last_cycle_level = battery_level();
    powermgmt_last_cycle_startstop_min = 0;
    trickle_sec  = 0;
    pid_p        = 0;
    pid_i        = 0;
    charge_state = DISCHARGING;
    snprintf(power_message, POWER_MESSAGE_LEN, "Charger: discharge");
}

static void charging_step(void)
{
    int i;

    /* alter charge time max length with extra disk use */
    if (disk_activity_last_cycle) {
        minutes_disk_activity++;
        charge_max_time_now = charge_max_time_idle +
                              minutes_disk_activity*2 / 5;
        disk_activity_last_cycle = false;
        last_disk_activity = 0;
    }
    else {
        last_disk_activity++;
    }

    /*
     * Check the delta voltage over the last X minutes so we can do
     * our end-of-charge logic based on the battery level change
     * (no longer use minimum time as logic for charge end has 50
     * minutes minimum charge built in).
     */
    if (powermgmt_last_cycle_startstop_min > CHARGE_END_SHORTD) {
        short_delta = power_history[0] -
                      power_history[CHARGE_END_SHORTD - 1];
    }

    if (powermgmt_last_cycle_startstop_min > CHARGE_END_LONGD) {
        /*
         * Scan the history: the points where measurement is taken need to
         * be fairly static. Check prior to short delta 'area'. Also only
         * check first and last 10 cycles (delta in middle OK).
         */
        long_delta = power_history[0] -
                     power_history[CHARGE_END_LONGD - 1];

        for (i = CHARGE_END_SHORTD; i < CHARGE_END_SHORTD + 10; i++)
        {
            if ((power_history[i] - power_history[i+1]) >  50 ||
                (power_history[i] - power_history[i+1]) < -50) {
                long_delta = 777777;
                break;
            }
        }

        for (i = CHARGE_END_LONGD - 11; i < CHARGE_END_LONGD - 1 ; i++)
        {
            if ((power_history[i] - power_history[i+1]) >  50 ||
                (power_history[i] - power_history[i+1]) < -50) {
                long_delta = 888888;
                break;
            }
        }
    }

    snprintf(power_message, POWER_MESSAGE_LEN,
             "Chg %dm, max %dm", powermgmt_last_cycle_startstop_min,
             charge_max_time_now);

    /*
     * End of charge criteria (any qualify):
     * 1) Charged a long time
     * 2) DeltaV went negative for a short time ( & long delta static)
     * 3) DeltaV was negative over a longer period (no disk use only)
     *
     * Note: short_delta and long_delta are millivolts
     */
    if (powermgmt_last_cycle_startstop_min >= charge_max_time_now ||
        (short_delta <= -50 && long_delta < 50) ||
        (long_delta  < -20 && last_disk_activity > CHARGE_END_LONGD)) {

        int battery_percent = battery_level();

        if (powermgmt_last_cycle_startstop_min > charge_max_time_now) {
            DEBUGF("power: powermgmt_last_cycle_startstop_min > charge_max_time_now, "
                   "enough!\n");
            /*
             * Have charged too long and deltaV detection did not
             * work!
             */
             snprintf(power_message, POWER_MESSAGE_LEN,
                     "Chg tmout %d min", charge_max_time_now);
            /*
             * Switch to trickle charging.  We skip the top-off
             * since we've effectively done the top-off operation
             * already since we charged for the maximum full
             * charge time.
             */
            powermgmt_last_cycle_level = battery_percent;
            powermgmt_last_cycle_startstop_min = 0;
            charge_state = TRICKLE;

            /*
             * Set trickle charge target to a relative voltage instead
             * of an arbitrary value - the fully charged voltage may
             * vary according to ambient temp, battery condition etc.
             * Trickle target is -0.15v from full voltage acheived.
             * Topup target is -0.05v from full voltage.
             */
            target_voltage = power_history[0] - 150;

        }
        else {
            if(short_delta <= -5) {
                DEBUGF("power: short-term negative"
                       " delta, enough!\n");
                snprintf(power_message, POWER_MESSAGE_LEN,
                         "end negd %d %dmin", short_delta,
                         powermgmt_last_cycle_startstop_min);
                target_voltage = power_history[CHARGE_END_SHORTD - 1] - 50;
            }
            else {
                DEBUGF("power: long-term small "
                       "positive delta, enough!\n");
                snprintf(power_message, POWER_MESSAGE_LEN,
                         "end lowd %d %dmin", long_delta,
                         powermgmt_last_cycle_startstop_min);
                target_voltage = power_history[CHARGE_END_LONGD - 1] - 50;
            }

            /*
             * Switch to top-off charging.
             */
            powermgmt_last_cycle_level = battery_percent;
            powermgmt_last_cycle_startstop_min = 0;
            charge_state = TOPOFF;
        }
    }
}

static void topoff_trickle_step(void)
{
    unsigned int millivolts;

    /*
     *Time to switch from topoff to trickle?
     */
    if (charge_state == TOPOFF &&
        powermgmt_last_cycle_startstop_min > TOPOFF_MAX_MIN) {

        powermgmt_last_cycle_level = battery_level();
        powermgmt_last_cycle_startstop_min = 0;
        charge_state = TRICKLE;
        target_voltage = target_voltage - 100;
    }
    /*
     * Adjust trickle charge time (proportional and integral terms).
     * Note: I considered setting the level higher if the USB is
     * plugged in, but it doesn't appear to be necessary and will
     * generate more heat [gvb].
     */
    millivolts = battery_voltage();

    pid_p = ((signed)target_voltage - (signed)millivolts) / 5;
    if (pid_p <= PID_DEADZONE && pid_p >= -PID_DEADZONE)
        pid_p = 0;

    if ((unsigned)millivolts < target_voltage) {
        if (pid_i < 60)
            pid_i++; /* limit so it doesn't "wind up" */
    }
    else {
        if (pid_i > 0)
            pid_i--; /* limit so it doesn't "wind up" */
    }

    trickle_sec = pid_p + pid_i;

    if (trickle_sec > 60)
        trickle_sec = 60;

    if (trickle_sec < 0)
        trickle_sec = 0;
}

void charging_algorithm_step(void)
{
    static int pwm_counter = 0; /* PWM total cycle in steps */
    static int pwm_duty = 0;    /* PWM duty cycle in steps */

    switch (charger_input_state)
    {
    case CHARGER_PLUGGED:
        charger_plugged();
        break;

    case CHARGER_UNPLUGGED:
        charger_unplugged();
        break;

    case CHARGER:
    case NO_CHARGER:
        do_frequent_tasks();

        if (pwm_counter > 0) {
            if (pwm_duty > 0 && --pwm_duty <= 0)
                charger_enable(false); /* Duty cycle expired */

            if (--pwm_counter > 0)
                return;

            /* PWM cycle is complete */
            powermgmt_last_cycle_startstop_min++;
            debug_file_log();
        }
        break;
    }

    switch (charge_state)
    {
    case CHARGING:
        charging_step();
        break;

    case TOPOFF:
    case TRICKLE:
        topoff_trickle_step();        
        break;

    case DISCHARGING:
    default:
        break;
    }

    /* If 100%, ensure pwm_on never expires and briefly disables the
     * charger. */
    pwm_duty = (trickle_sec < 60) ? trickle_sec*2 : 0;
    pwm_counter = 60*2;
    charger_enable(trickle_sec > 0);
}

#ifdef CHARGING_DEBUG_FILE
void charging_algorithm_close(void)
{
    debug_file_close();
}
#endif /* CHARGING_DEBUG_FILE */

/* Returns true if the unit is charging the batteries. */
bool charging_state(void)
{
    return charge_state == CHARGING;
}
