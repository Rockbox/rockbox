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
#include "cpu.h"
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
#include "mp3_playback.h"
#include "usb.h"
#include "powermgmt.h"
#include "backlight.h"
#include "lcd.h"
#include "rtc.h"
#ifdef CONFIG_TUNER
#include "fmradio.h"
#endif

/*
 * Define DEBUG_FILE to create a csv (spreadsheet) with battery information
 * in it (one sample per minute).  This is only for very low level debug.
 */
#undef  DEBUG_FILE
#if defined(DEBUG_FILE) && defined(HAVE_CHARGE_CTRL)
#include "file.h"
#define DEBUG_FILE_NAME   "/powermgmt.csv"
#define DEBUG_MESSAGE_LEN 133
static char debug_message[DEBUG_MESSAGE_LEN];
#define DEBUG_STACK ((0x1000)/sizeof(long))
static int fd;          /* write debug information to this file */
#else
#define DEBUG_STACK 0
#endif

long last_event_tick;

void reset_poweroff_timer(void)
{
    last_event_tick = current_tick;
}

#ifdef SIMULATOR /***********************************************************/

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

#else /* not SIMULATOR ******************************************************/

static const int poweroff_idle_timeout_value[15] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 30, 45, 60
};

static const short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
/* voltages (centivolt) of 0%, 10%, ... 100% when charging disabled */
{
#if CONFIG_BATTERY == BATT_LIION2200
    /* measured values */
    { 260, 285, 295, 303, 311, 320, 330, 345, 360, 380, 400 }
#elif CONFIG_BATTERY == BATT_3AAA
    /* measured values */
    { 280, 325, 341, 353, 364, 374, 385, 395, 409, 427, 475 }, /* alkaline */
    { 310, 355, 363, 369, 372, 374, 376, 378, 380, 386, 405 }  /* NiMH */
#else /* NiMH */
    /* original values were taken directly after charging, but it should show
       100% after turning off the device for some hours, too */
    { 450, 481, 491, 497, 503, 507, 512, 514, 517, 525, 540 } 
                                            /* orig. values: ...,528,560 */
#endif
};

static int battery_type = 0;

#if defined(HAVE_CHARGE_CTRL) || CONFIG_BATTERY == BATT_LIION2200
charge_state_type charge_state;     /* charging mode */
int  charge_timer;                  /* charge timer (minutes, dec to zero) */
#endif

#ifdef HAVE_CHARGING
/* Flag that the charger has been plugged in */
static bool charger_was_inserted = false;   /* for power off logic */
static bool charger_power_is_on;            /* for car adapter mode */
#endif

/* Power history: power_history[0] is the newest sample */
unsigned short power_history[POWER_HISTORY_LEN];

#ifdef HAVE_CHARGE_CTRL

int long_delta;                     /* long term delta battery voltage */
int short_delta;                    /* short term delta battery voltage */

char power_message[POWER_MESSAGE_LEN] = "";    /* message that's shown in
                                                  debug menu */
static char charge_restart_level = CHARGE_RESTART_HI;
                                               /* percentage at which charging
                                                  starts */
int powermgmt_last_cycle_startstop_min = 0;    /* how many minutes ago was the
                                                  charging started or
                                                  stopped? */
int powermgmt_last_cycle_level = 0;            /* which level had the
                                                  batteries at this time? */
bool trickle_charge_enabled = true;
int  trickle_sec = 0;                          /* how many seconds should the
                                                  charger be enabled per
                                                  minute for trickle
                                                  charging? */
/* voltages (centivolt) of 0%, 10%, ... 100% when charging enabled */
static const short percent_to_volt_charge[11] =
{
    /* values guessed, see
       http://www.seattlerobotics.org/encoder/200210/LiIon2.pdf until someone
       measures voltages over a charging cycle */
    476, 544, 551, 556, 561, 564, 566, 576, 582, 584, 585 /* NiMH */
};

#endif /* HAVE_CHARGE_CTRL || CONFIG_BATTERY == BATT_LIION2200 */

/*
 * Average battery voltage and charger voltage, filtered via a digital
 * exponential filter.
 */
unsigned int battery_centivolts;/* filtered battery voltage, centvolts */
static unsigned int avgbat;     /* average battery voltage */
#define BATT_AVE_SAMPLES	32  /* filter constant / @ 2Hz sample rate */

int battery_capacity = BATTERY_CAPACITY_MIN; /* only a default value */

/* battery level (0-100%) of this minute, updated once per minute */
static int battery_percent = -1;

static bool car_adapter_mode_enabled = false;

static char power_stack[DEFAULT_STACK_SIZE + DEBUG_STACK];
static const char power_thread_name[] = "power";

static int  poweroff_timeout = 0;
static long last_charge_time = 0;
int powermgmt_est_runningtime_min = -1;

static bool sleeptimer_active = false;
static unsigned long sleeptimer_endtick;

#ifdef HAVE_CHARGE_CTRL

void enable_deep_discharge(bool on)
{
    charge_restart_level = on ? CHARGE_RESTART_LO : CHARGE_RESTART_HI;
}

void enable_trickle_charge(bool on)
{
    trickle_charge_enabled = on;
}
#endif /* HAVE_CHARGE_CTRL */

#if BATTERY_TYPES_COUNT > 1
void set_battery_type(int type)
{
    if (type != battery_type) {
        battery_type = type;
        battery_percent = -1; /* reset on type change */
    }
}
#endif

void set_battery_capacity(int capacity)
{
    battery_capacity = capacity;
    if (battery_capacity > BATTERY_CAPACITY_MAX)
        battery_capacity = BATTERY_CAPACITY_MAX;
    if (battery_capacity < BATTERY_CAPACITY_MIN)
        battery_capacity = BATTERY_CAPACITY_MIN;
}

int battery_time(void)
{
    return powermgmt_est_runningtime_min;
}

/* look into the percent_to_volt_* table and get a realistic battery level
       percentage */
int voltage_to_percent(int voltage, const short* table)
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
    int level;

    level = battery_centivolts;
    if(level > BATTERY_LEVEL_FULL)
        level = BATTERY_LEVEL_FULL;

    if(level < BATTERY_LEVEL_EMPTY)
        level = BATTERY_LEVEL_EMPTY;

#ifdef HAVE_CHARGE_CTRL
    if (charge_state == DISCHARGING) {
        level = voltage_to_percent(level, 
                percent_to_volt_discharge[battery_type]);
    }
    else if (charge_state == CHARGING) {
        level = voltage_to_percent(level, percent_to_volt_charge);
    }
    else {/* in trickle charge, the battery is by definition 100% full */
        battery_percent = level = 100;
    }
#else
    /* always use the discharge table */
    level = voltage_to_percent(level,
            percent_to_volt_discharge[battery_type]);
#endif

#ifndef HAVE_MMC  /* this adjustment is only needed for HD based */
    if (battery_percent == -1) { /* first run of this procedure */
        /* The battery voltage is usually a little lower directly after
           turning on, because the disk was used heavily. Raise it by 5. % */
        battery_percent = (level > 95) ? 100 : level + 5;
    }
    else 
#endif
    {
        /* the level is allowed to be -1 of the last value when usb not
           connected and to be -3 of the last value when usb is connected */
        if (usb_inserted()) {
            if (level < battery_percent - 3)
                level = battery_percent - 3;
        }
        else {
            if (level < battery_percent - 1)
                level = battery_percent - 1;
        }
        battery_percent = level;
    }
}

/* Returns battery level in percent */
int battery_level(void)
{
#ifdef HAVE_CHARGE_CTRL
    if ((charge_state == CHARGING) && (battery_percent == 100))
        return 99;
#endif
    return battery_percent;
}

/* Tells if the battery level is safe for disk writes */
bool battery_level_safe(void)
{
    return battery_centivolts > BATTERY_LEVEL_DANGEROUS;
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
    int  mpeg_stat = mpeg_status();
#ifdef HAVE_CHARGING
    bool charger_is_inserted = charger_inserted();
#endif

#ifdef HAVE_CHARGING
    /* The was_inserted thing prevents the unit to shut down immediately
       when the charger is extracted */
    if(charger_is_inserted || charger_was_inserted)
    {
        last_charge_time = current_tick;
    }
    charger_was_inserted = charger_is_inserted;
#endif
    
    if(timeout &&
#ifdef CONFIG_TUNER
       (radio_get_status() != FMRADIO_PLAYING) &&
#endif
       !usb_inserted() &&
       ((mpeg_stat == 0) ||
        ((mpeg_stat == (MPEG_STATUS_PLAY | MPEG_STATUS_PAUSE)) &&
         !sleeptimer_active)))
    {
        if(TIME_AFTER(current_tick, last_event_tick + timeout) &&
           TIME_AFTER(current_tick, last_disk_activity + timeout) &&
           TIME_AFTER(current_tick, last_charge_time + timeout))
        {
            shutdown_hw();
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
#ifdef HAVE_CHARGING
                if(charger_is_inserted)
                {
                    DEBUGF("Sleep timer timeout. Stopping...\n");
                    set_sleep_timer(0);
                    backlight_off(); /* Nighty, nighty... */
                }
                else
#endif
                {
                    DEBUGF("Sleep timer timeout. Shutting off...\n");
                    /* Make sure that the disk isn't spinning when
                       we cut the power */
                    while(ata_disk_is_active())
                       sleep(HZ);
                    shutdown_hw();
                }
            }
        }
    }
}

void set_car_adapter_mode(bool setting)
{
    car_adapter_mode_enabled = setting;
}

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
 * Estimate how much current we are drawing just to run.
 */
static int runcurrent(void)
{
    int current;

    current = CURRENT_NORMAL;
    if(usb_inserted()) {
        current = CURRENT_USB;
    }
#ifdef HAVE_CHARGE_CTRL
    if ((backlight_get_timeout() == 1) || /* LED always on */
        (charger_inserted() && backlight_get_on_when_charging())) {
        current += CURRENT_BACKLIGHT;
    }
#else
    if (backlight_get_timeout() == 1) { /* LED always on */
        current += CURRENT_BACKLIGHT;
    }
#endif

    return(current);
}


/* Check to see whether or not we've received an alarm in the last second */
#ifdef HAVE_ALARM_MOD
static void power_thread_rtc_process(void) 
{
    if (rtc_check_alarm_flag()) {
        rtc_enable_alarm(false);
    }
}
#endif

/*
 * This function is called to do the relativly long sleep waits from within the
 * main power_thread loop while at the same time servicing any other periodic
 * functions in the power thread which need to be called at a faster periodic
 * rate than the slow periodic rate of the main power_thread loop.
 *
 * While we are waiting for the time to expire, we average the battery
 * voltages.
 */
static void power_thread_sleep(int ticks)
{
    int small_ticks;
#ifdef HAVE_CHARGING
    unsigned int tmp;
    bool charger_plugged;
#endif

#ifdef HAVE_CHARGING
    charger_plugged = charger_inserted();
#endif
    while (ticks > 0) {
        small_ticks = MIN(HZ/2, ticks);
        sleep(small_ticks);
        ticks -= small_ticks;

#ifdef HAVE_CHARGING
        car_adapter_mode_processing();
#endif
#ifdef HAVE_ALARM_MOD
        power_thread_rtc_process();
#endif

        /*
         * Do a digital exponential filter.  We don't sample the battery if
         * the disk is spinning unless we are in USB mode (the disk will most
         * likely always be spinning in USB mode).
         * If the charging voltage is greater than 0x3F0 charging isn't active
         * and that voltage isn't valid.
         */
        if (!ata_disk_is_active() || usb_inserted()) {
            avgbat = avgbat - (avgbat / BATT_AVE_SAMPLES) +
                     adc_read(ADC_UNREG_POWER);
            /*
             * battery_centivolts is the centivolt-scaled filtered battery value.
             */
            battery_centivolts = ((avgbat / BATT_AVE_SAMPLES) * BATTERY_SCALE_FACTOR) / 10000;
        }

#ifdef HAVE_CHARGING
        /* If the charger was plugged in, exit now so we can start charging */
        if(!charger_plugged && charger_inserted())
            return;
#endif
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
    short *phps, *phpd;         /* power history rotation pointers */
#if CONFIG_BATTERY == BATT_LIION2200
    int charging_current;
#endif
#ifdef HAVE_CHARGE_CTRL
    int charge_max_time_now = 0;  /* max. charging duration, calculated at
                                     beginning of charging */
#endif

    /* initialize the voltages for the exponential filter */
    
    avgbat = adc_read(ADC_UNREG_POWER) * BATT_AVE_SAMPLES;
    battery_centivolts    = ((avgbat / BATT_AVE_SAMPLES) * BATTERY_SCALE_FACTOR) / 10000;

#if defined(DEBUG_FILE) && defined(HAVE_CHARGE_CTRL)
    fd = -1;
#endif
            
    while (1)
    {
        /* rotate the power history */
        phpd = &power_history[POWER_HISTORY_LEN - 1];
        phps = phpd - 1;
        for (i = 0; i < POWER_HISTORY_LEN-1; i++)
            *phpd-- = *phps--;
        
        /* insert new value at the start, in centivolts 8-) */
        power_history[0] = battery_centivolts;

        /* update battery level every minute */
        battery_level_update();

        /* calculate estimated remaining running time */
        /* discharging: remaining running time */
        /* charging:    remaining charging time */

        powermgmt_est_runningtime_min = battery_level() *
            battery_capacity / 100 * 60 / runcurrent();
#if MEM == 8 /* assuming 192 kbps, the running time is 22% longer with 8MB */
        powermgmt_est_runningtime_min =
            powermgmt_est_runningtime_min * 122 / 100;
#endif /* MEM == 8 */

#ifdef HAVE_CHARGE_CTRL
        /*
         * If we are charging, the "runtime" is estimated time till the battery
         * is recharged.
         */
        // TBD: use real charging current estimate
        if (charge_state == CHARGING) {
            powermgmt_est_runningtime_min = (100 - battery_level()) *
                 battery_capacity / 100 * 60 / (CURRENT_MAX_CHG - runcurrent());
        }
#endif /* HAVE_CHARGE_CTRL */

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
                charge_state = TRICKLE;
            } else {
                charge_state = CHARGING;
            }
        } else {
            charge_state = DISCHARGING;
        }
#endif /* # if CONFIG_BATTERY == BATT_LIION2200 */

#ifdef HAVE_CHARGE_CTRL

        if (charger_inserted()) {
            /*
             * Time to start charging again?
             * 1) If we are currently discharging but trickle is enabled,
             *    the charger must have just been plugged in.
             * 2) If our battery level falls below the restart level, charge!
             */
            if (((charge_state == DISCHARGING) && trickle_charge_enabled) ||
                (battery_level() < charge_restart_level)) {

                /*
                 * If the battery level is nearly charged, just trickle.
                 * If the battery is in between, top-off and then trickle.
                 */
                if(battery_percent > charge_restart_level) {
                    powermgmt_last_cycle_level = battery_percent;
                    powermgmt_last_cycle_startstop_min = 0;
                    if(battery_percent >= 95) {
                        trickle_sec  = START_TRICKLE_SEC;
                        charge_state = TRICKLE;
                    } else {
                        trickle_sec  = START_TOPOFF_SEC;
                        charge_state = TOPOFF;
                    }
                } else {
                    /*
                     * Start the charger full strength
                     */
                    i = CHARGE_MAX_TIME_1500 * battery_capacity / 1500;
                    charge_max_time_now =
                        i * (100 + 35 - battery_percent) / 100;
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
                    powermgmt_last_cycle_level = battery_percent;
                    powermgmt_last_cycle_startstop_min = 0;
                    trickle_sec  = 60;
                    charge_state = CHARGING;
                }
            }
            if (charge_state == CHARGING) {
                /* charger inserted and enabled 100% of the time */
                trickle_sec = 60; /* 100% on */

                snprintf(power_message, POWER_MESSAGE_LEN,
                         "Chg %dm, max %dm", powermgmt_last_cycle_startstop_min,
                         charge_max_time_now);
                /*
                 * Sum the deltas over the last X minutes so we can do our
                 *   end-of-charge logic based on the battery level change.
                 */
                long_delta = short_delta = 999999;
                if (powermgmt_last_cycle_startstop_min > CHARGE_MIN_TIME) {
                    short_delta = power_history[0] -
                                  power_history[CHARGE_END_NEGD - 1];
                }
                if (powermgmt_last_cycle_startstop_min > CHARGE_END_ZEROD) {
                /*
                 * Scan the history: if we have a big delta in the middle of
                 * our history, the long term delta isn't a valid end-of-charge
                 * indicator.
                 */
                    long_delta = power_history[0] -
                                 power_history[CHARGE_END_ZEROD - 1];
                    for(i = 0; i < CHARGE_END_ZEROD; i++) {
                        if(((power_history[i] - power_history[i+1]) >  5) ||
                           ((power_history[i] - power_history[i+1]) < -5)) {
                            long_delta = 888888;
                            break;
                        }
                    }
                }

                /*
                 * End of charge criteria (any qualify):
                 * 1) Charged a long time
                 * 2) DeltaV went negative for a short time
                 * 3) DeltaV was close to zero for a long time
                 * Note: short_delta and long_delta are centivolts
                 */
                if ((powermgmt_last_cycle_startstop_min > charge_max_time_now) ||
                    (short_delta < -5) || (long_delta  <  5))
                {
                    if (powermgmt_last_cycle_startstop_min > charge_max_time_now) {
                        DEBUGF("power: powermgmt_last_cycle_startstop_min > charge_max_time_now, "
                               "enough!\n");
                        /* have charged too long and deltaV detection did not
                           work! */
                        snprintf(power_message, POWER_MESSAGE_LEN,
                                 "Chg tmout %d min", charge_max_time_now);
                    } else {
                        if(short_delta < -5) {
                            DEBUGF("power: short-term negative"
                                   " delta, enough!\n");
                            snprintf(power_message, POWER_MESSAGE_LEN,
                                     "end negd %d %dmin", short_delta,
                                     powermgmt_last_cycle_startstop_min);
                        } else {
                            DEBUGF("power: long-term small "
                                   "positive delta, enough!\n");
                            snprintf(power_message, POWER_MESSAGE_LEN,
                                     "end lowd %d %dmin", long_delta,
                                     powermgmt_last_cycle_startstop_min);
                        }
                    }
                    /* Switch to trickle charging.  We skip the top-off
                       since we've effectively done the top-off operation
                       already since we charged for the maximum full
                       charge time.  For trickle charging, we use 0.05C */
                    powermgmt_last_cycle_level = battery_percent;
                    powermgmt_last_cycle_startstop_min = 0;
                    if (trickle_charge_enabled) {
                        trickle_sec  = START_TRICKLE_SEC;
                        charge_state = TRICKLE;
                    } else {
                        /* If we don't trickle charge, we discharge */
                        trickle_sec  = 0; /* off */
                        charge_state = DISCHARGING;
                    }
                }
            }
            else if (charge_state > CHARGING)  /* top off or trickle */
            {
                /* Time to switch from topoff to trickle? Note that we don't
                 * adjust trickle_sec: it will get adjusted down by the
                 * charge level adjustment in the loop and will drift down
                 * from the topoff level to the trickle level.
                 */
                if ((charge_state == TOPOFF) &&
                    (powermgmt_last_cycle_startstop_min > TOPOFF_MAX_TIME))
                {
                    powermgmt_last_cycle_level = battery_percent;
                    powermgmt_last_cycle_startstop_min = 0;
                    charge_state = TRICKLE;
                }

                /* Adjust trickle charge time.  I considered setting the level
                 * higher if the USB is plugged in, but it doesn't appear to
                 * be necessary and will generate more heat [gvb].
                 */
                if(((charge_state == TOPOFF)  && (battery_centivolts > TOPOFF_VOLTAGE)) ||
                   ((charge_state == TRICKLE) && (battery_centivolts > TRICKLE_VOLTAGE)))
                { /* charging too much */
                    if(trickle_sec > 0)
                        trickle_sec--;
                }
                else { /* charging too little */
                    if(trickle_sec < 60)
                        trickle_sec++;
                }

            } else if (charge_state == DISCHARGING) {
                trickle_sec = 0;
                /* the charger is enabled here only in one case: if it was
                   turned on at boot time (power_init) */
                /* turn it off now */
                if (charger_enabled)
                    charger_enable(false);
            }
        } else {
            if (charge_state != DISCHARGING) {
                /* charger not inserted but was enabled */
                DEBUGF("power: charger disconnected, disabling\n");

                charger_enable(false);
                powermgmt_last_cycle_level = battery_percent;
                powermgmt_last_cycle_startstop_min = 0;
                trickle_sec = 0;
                charge_state = DISCHARGING;
                snprintf(power_message, POWER_MESSAGE_LEN, "Charger: discharge");
            }
        }
        powermgmt_last_cycle_startstop_min++;
        
#endif /* HAVE_CHARGE_CTRL*/

        /* sleep for a minute */

#ifdef HAVE_CHARGE_CTRL
        if(trickle_sec > 0) {
            charger_enable(true);
            power_thread_sleep(HZ * trickle_sec);
        }
        if(trickle_sec < 60)
            charger_enable(false);
        power_thread_sleep(HZ * (60 - trickle_sec));
#else
        power_thread_sleep(HZ * 60);
#endif

#if defined(DEBUG_FILE) && defined(HAVE_CHARGE_CTRL)
        if((fd < 0) && !usb_inserted()) {
            fd = open(DEBUG_FILE_NAME, O_WRONLY | O_APPEND | O_CREAT);
            snprintf(debug_message, DEBUG_MESSAGE_LEN,
            "cycle_min, bat_centivolts, bat_percent, chgr, chg_state, trickle_sec\n");
            write(fd, debug_message, strlen(debug_message));
            fsync(fd);
        } else if((fd >= 0) && !usb_inserted()) {
            snprintf(debug_message, DEBUG_MESSAGE_LEN, "%d, %d, %d, %d, %d, %d\n",
                powermgmt_last_cycle_startstop_min, battery_centivolts,
                battery_percent, charger_inserted(), charge_state, trickle_sec);
            write(fd, debug_message, strlen(debug_message));
            fsync(fd);
        } else if((fd >= 0) && usb_inserted()) {
            /* NOTE: It is probably already TOO LATE to close the file */
            close(fd);
            fd = -1;
        }
#endif
        handle_auto_poweroff();
    }
}

void powermgmt_init(void)
{
    power_init();
    
    /* init history to 0 */
    memset(power_history, 0x00, sizeof(power_history));

#ifdef HAVE_CHARGING
    charger_power_is_on = charger_inserted();
#endif
    
    create_thread(power_thread, power_stack, sizeof(power_stack),
                  power_thread_name);
}

#endif /* SIMULATOR */

/* Various hardware housekeeping tasks relating to shutting down the jukebox */
void shutdown_hw(void) 
{
#ifndef SIMULATOR
#if defined(DEBUG_FILE) && defined(HAVE_CHARGE_CTRL)
    if(fd > 0) {
        close(fd);
        fd = 0;
    }
#endif
    mpeg_stop();
    ata_flush();
    ata_spindown(1);
    while(ata_disk_is_active())
        sleep(HZ/10);

    mp3_shutdown();
#if CONFIG_KEYPAD == ONDIO_PAD
    backlight_off();
    sleep(1);
    lcd_set_contrast(0);
#endif
    power_off();
#endif
}
