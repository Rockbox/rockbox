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
#include "audio.h"
#include "mp3_playback.h"
#include "usb.h"
#include "powermgmt.h"
#include "backlight.h"
#include "lcd.h"
#include "rtc.h"
#ifdef CONFIG_TUNER
#include "fmradio.h"
#endif
#ifdef HAVE_UDA1380
#include "uda1380.h"
#elif defined(HAVE_TLV320)
#include "tlv320.h"
#elif defined(HAVE_WM8758)
#include "wm8758.h"
#elif defined(HAVE_WM8975)
#include "wm8975.h"
#endif
#ifdef HAVE_LCD_BITMAP
#include "font.h"
#endif
#include "logf.h"
#include "lcd-remote.h"
#ifdef SIMULATOR
#include <time.h>
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
static int wrcount;
#else
#define DEBUG_STACK 0
#endif

static int shutdown_timeout = 0;

#ifdef SIMULATOR /***********************************************************/

#define TIME2CHANGE     10              /* change levels every 10 seconds */
#define BATT_MINCVOLT   250             /* minimum centivolts of battery */
#define BATT_MAXCVOLT   450             /* maximum centivolts of battery */
#define BATT_MAXRUNTIME (10 * 60)       /* maximum runtime with full battery in minutes */

static unsigned int batt_centivolts = (unsigned int)BATT_MAXCVOLT;
static int batt_level = 100;            /* battery capacity level in percent */
static int batt_time = BATT_MAXRUNTIME; /* estimated remaining time in minutes */
static time_t last_change = 0;

static void battery_status_update(void)
{
    time_t          now;

    time(&now);
    if (last_change < (now - TIME2CHANGE)) {
        last_change = now;

        /* change the values: */
        batt_centivolts -= (unsigned int)(BATT_MAXCVOLT - BATT_MINCVOLT) / 11;
        if (batt_centivolts < (unsigned int)BATT_MINCVOLT)
            batt_centivolts = (unsigned int)BATT_MAXCVOLT;

        batt_level = 100 * (batt_centivolts - BATT_MINCVOLT) / (BATT_MAXCVOLT - BATT_MINCVOLT);
        batt_time = batt_level * BATT_MAXRUNTIME / 100;
    }
}

unsigned int battery_voltage(void)
{
    battery_status_update();
    return batt_centivolts;
}

int battery_level(void)
{
    battery_status_update();
    return batt_level;
}

int battery_time(void)
{
    battery_status_update();
    return batt_time;
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

void reset_poweroff_timer(void)
{
}


#else /* not SIMULATOR ******************************************************/

static const int poweroff_idle_timeout_value[15] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 30, 45, 60
};

static const unsigned int battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
#if CONFIG_BATTERY == BATT_LIION2200 /* FM Recorder, LiIon */
    280
#elif CONFIG_BATTERY == BATT_3AAA /* Ondio */
    310, 345    /* alkaline, NiHM */
#elif CONFIG_BATTERY == BATT_LIPOL1300 /* iRiver H1x0 */
    339
#else /* Player/recorder, NiMH */
    475
#endif
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
#elif CONFIG_BATTERY == BATT_LIPOL1300
    /* Below 337 the backlight starts flickering during HD access */
    /* Calibrated for Ionity 1900 mAh battery. If necessary, re-calibrate
     * for the 1300 mAh stock battery. */
//  { 337, 358, 365, 369, 372, 377, 383, 389, 397, 406, 413 }
    { 337, 366, 372, 374, 378, 381, 385, 392, 399, 408, 417 }
#else /* NiMH */
    /* original values were taken directly after charging, but it should show
       100% after turning off the device for some hours, too */
    { 450, 481, 491, 497, 503, 507, 512, 514, 517, 525, 540 }
                                            /* orig. values: ...,528,560 */
#endif
};

#ifdef HAVE_CHARGING
charger_input_state_type charger_input_state IDATA_ATTR;

/* voltages (centivolt) of 0%, 10%, ... 100% when charging enabled */
static const short percent_to_volt_charge[11] =
{
#if CONFIG_BATTERY == BATT_LIPOL1300
    /* Calibrated for 1900 mAh Ionity battery (estimated 90% charge when
     entering in trickle-charging). We will never reach 100%. */
    340, 390, 394, 399, 400, 404, 407, 413, 417, 422, 426
#else
    /* values guessed, see
       http://www.seattlerobotics.org/encoder/200210/LiIon2.pdf until someone
       measures voltages over a charging cycle */
    476, 544, 551, 556, 561, 564, 566, 576, 582, 584, 585 /* NiMH */
#endif
};
#endif /* HAVE_CHARGING */

#if defined(HAVE_CHARGE_CTRL) || \
    CONFIG_BATTERY == BATT_LIION2200 || \
    defined(HAVE_CHARGE_STATE)
charge_state_type charge_state;     /* charging mode */
#endif

#ifdef HAVE_CHARGE_CTRL
int long_delta;                     /* long term delta battery voltage */
int short_delta;                    /* short term delta battery voltage */
bool disk_activity_last_cycle = false;         /* flag set to aid charger time
                                                * calculation */
char power_message[POWER_MESSAGE_LEN] = "";    /* message that's shown in
                                                  debug menu */
                                               /* percentage at which charging
                                                  starts */
int powermgmt_last_cycle_startstop_min = 0;    /* how many minutes ago was the
                                                  charging started or
                                                  stopped? */
int powermgmt_last_cycle_level = 0;            /* which level had the
                                                  batteries at this time? */
int trickle_sec = 0;                           /* how many seconds should the
                                                  charger be enabled per
                                                  minute for trickle
                                                  charging? */
int pid_p = 0;                      /* PID proportional term */
int pid_i = 0;                      /* PID integral term */
#endif /* HAVE_CHARGE_CTRL */

/*
 * Average battery voltage and charger voltage, filtered via a digital
 * exponential filter.
 */
static unsigned int battery_centivolts;/* filtered battery voltage, centvolts */
static unsigned int avgbat;     /* average battery voltage (filtering) */
#define BATT_AVE_SAMPLES    32  /* filter constant / @ 2Hz sample rate */

/* battery level (0-100%) of this minute, updated once per minute */
static int battery_percent  = -1;
static int battery_capacity = BATTERY_CAPACITY_DEFAULT; /* default value, mAh */
static int battery_type     = 0;

/* Power history: power_history[0] is the newest sample */
unsigned short power_history[POWER_HISTORY_LEN];

static char power_stack[DEFAULT_STACK_SIZE + DEBUG_STACK];
static const char power_thread_name[] = "power";

static int poweroff_timeout = 0;
static int powermgmt_est_runningtime_min = -1;

static bool sleeptimer_active = false;
static long sleeptimer_endtick;

static long last_event_tick;

static void battery_status_update(void);
static int runcurrent(void);

unsigned int battery_voltage(void)
{
    return battery_centivolts;
}

void reset_poweroff_timer(void)
{
    last_event_tick = current_tick;
}

#if BATTERY_TYPES_COUNT > 1
void set_battery_type(int type)
{
    if (type != battery_type) {
        battery_type = type;
        battery_status_update();     /* recalculate the battery status */
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
    battery_status_update();     /* recalculate the battery status */
}

int battery_time(void)
{
    return powermgmt_est_runningtime_min;
}

/* Returns battery level in percent */
int battery_level(void)
{
    return battery_percent;
}

/* Tells if the battery level is safe for disk writes */
bool battery_level_safe(void)
{
    return battery_centivolts > battery_level_dangerous[battery_type];
}

void set_poweroff_timeout(int timeout)
{
    poweroff_timeout = timeout;
}

void set_sleep_timer(int seconds)
{
    if(seconds) {
        sleeptimer_active  = true;
        sleeptimer_endtick = current_tick + seconds * HZ;
    }
    else {
        sleeptimer_active  = false;
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

/* look into the percent_to_volt_* table and get a realistic battery level
       percentage */
static int voltage_to_percent(int voltage, const short* table)
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
            return (i * 10) /* Tens digit, 10% per entry */
                + (((voltage - table[i]) * 10)
                   / (table[i+1] - table[i])); /* Ones digit: interpolated */
        }
}

/* update battery level and estimated runtime, called once per minute or
 * when battery capacity / type settings are changed */
static void battery_status_update(void)
{
    int level;

#if defined(HAVE_CHARGE_CTRL) || defined(HAVE_CHARGE_STATE)
    if (charge_state == DISCHARGING) {
        level = voltage_to_percent(battery_centivolts,
                percent_to_volt_discharge[battery_type]);
    }
    else if (charge_state == CHARGING) {
        /* battery level is defined to be < 100% until charging is finished */
        level = MIN(voltage_to_percent(battery_centivolts,
                    percent_to_volt_charge), 99);
    }
    else { /* in topoff/trickle charge, the battery is by definition 100% full */
        level = 100;
    }
#else
    /* always use the discharge table */
    level = voltage_to_percent(battery_centivolts,
            percent_to_volt_discharge[battery_type]);
#endif

#ifndef HAVE_MMC  /* this adjustment is only needed for HD based */
    if (battery_percent == -1) { /* first run of this procedure */
        /* The battery voltage is usually a little lower directly after
           turning on, because the disk was used heavily. Raise it by 5. % */
        level = (level > 95) ? 100 : level + 5;
    }
#endif
    battery_percent = level;

    /* calculate estimated remaining running time */
    /* discharging: remaining running time */
    /* charging:    remaining charging time */
#if defined(HAVE_CHARGE_CTRL) || defined(HAVE_CHARGE_STATE)
    if (charge_state == CHARGING) {
        powermgmt_est_runningtime_min = (100 - level) * battery_capacity / 100
                                      * 60 / (CURRENT_MAX_CHG - runcurrent());
    }
    else
#endif
    {
        powermgmt_est_runningtime_min = level * battery_capacity / 100
                                      * 60 / runcurrent();
    }
}

/*
 * We shut off in the following cases:
 * 1) The unit is idle, not playing music
 * 2) The unit is playing music, but is paused
 *
 * We do not shut off in the following cases:
 * 1) The USB is connected
 * 2) The charger is connected
 * 3) We are recording, or recording with pause
 */
static void handle_auto_poweroff(void)
{
    long timeout = poweroff_idle_timeout_value[poweroff_timeout]*60*HZ;
    int  audio_stat = audio_status();

#ifdef HAVE_CHARGING
    /*
     * Inhibit shutdown as long as the charger is plugged in.  If it is
     * unplugged, wait for a timeout period and then shut down.
     */
    if(charger_input_state == CHARGER || audio_stat == AUDIO_STATUS_PLAY) {
        last_event_tick = current_tick;
    }
#endif

    if(timeout &&
#ifdef CONFIG_TUNER
       (!radio_powered()) &&
#endif
       !usb_inserted() &&
       ((audio_stat == 0) ||
        ((audio_stat == (AUDIO_STATUS_PLAY | AUDIO_STATUS_PAUSE)) &&
         !sleeptimer_active)))
    {
        if(TIME_AFTER(current_tick, last_event_tick    + timeout) &&
           TIME_AFTER(current_tick, last_disk_activity + timeout))
        {
            sys_poweroff();
        }
    }
    else
    {
        /* Handle sleeptimer */
        if(sleeptimer_active && !usb_inserted())
        {
            if(TIME_AFTER(current_tick, sleeptimer_endtick))
            {
                audio_stop();
#if defined(HAVE_CHARGING) && !defined(HAVE_POWEROFF_WHILE_CHARGING)
                if((charger_input_state == CHARGER) ||
                   (charger_input_state == CHARGER_PLUGGED))
                {
                    DEBUGF("Sleep timer timeout. Stopping...\n");
                    set_sleep_timer(0);
                    backlight_off(); /* Nighty, nighty... */
                }
                else
#endif
                {
                    DEBUGF("Sleep timer timeout. Shutting off...\n");
                    sys_poweroff();
                }
            }
        }
    }
}

/*
 * Estimate how much current we are drawing just to run.
 */
static int runcurrent(void)
{
    int current;

#if MEM == 8 && !defined(HAVE_MMC)
    /* assuming 192 kbps, the running time is 22% longer with 8MB */
    current = (CURRENT_NORMAL*100/122);
#else
    current = CURRENT_NORMAL;
#endif /* MEM == 8 */

    if(usb_inserted()
#if defined(HAVE_USB_POWER)
  #if (CURRENT_USB < CURRENT_NORMAL)
       || usb_powered()
  #else
       && !usb_powered()
  #endif
#endif
    )
    {
        current = CURRENT_USB;
    }

#if defined(CONFIG_BACKLIGHT) && !defined(BOOTLOADER)
    if (backlight_get_current_timeout() == 0) /* LED always on */
        current += CURRENT_BACKLIGHT;
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

    while (ticks > 0) {

#ifdef HAVE_CHARGING
        /*
         * Detect charger plugged/unplugged transitions.  On a plugged or
         * unplugged event, we return immediately, run once through the main
         * loop (including the subroutines), and end up back here where we
         * transition to the appropriate steady state charger on/off state.
         */
        if(charger_inserted()
#ifdef HAVE_USB_POWER
                || usb_powered()
#endif
                ) {
            switch(charger_input_state) {
                case NO_CHARGER:
                case CHARGER_UNPLUGGED:
                    charger_input_state = CHARGER_PLUGGED;
                    return;
                case CHARGER_PLUGGED:
                    queue_broadcast(SYS_CHARGER_CONNECTED, NULL);
                    charger_input_state = CHARGER;
                    break;
                case CHARGER:
                    break;
            }
        } else {    /* charger not inserted */
            switch(charger_input_state) {
                case NO_CHARGER:
                    break;
                case CHARGER_UNPLUGGED:
                    queue_broadcast(SYS_CHARGER_DISCONNECTED, NULL);
                    charger_input_state = NO_CHARGER;
                    break;
                case CHARGER_PLUGGED:
                case CHARGER:
                    charger_input_state = CHARGER_UNPLUGGED;
                    return;
            }
        }
#endif
#ifdef HAVE_CHARGE_STATE
        switch (charger_input_state) {
            case CHARGER_UNPLUGGED:
            case NO_CHARGER:
                charge_state = DISCHARGING;
                break;
            case CHARGER_PLUGGED:
            case CHARGER:
                if (charging_state()) {
                    charge_state = CHARGING;
                } else {
                    charge_state = DISCHARGING;
                }
                break;
        }

#endif /* HAVE_CHARGE_STATE */

        small_ticks = MIN(HZ/2, ticks);
        sleep(small_ticks);
        ticks -= small_ticks;

        /* If the power off timeout expires, the main thread has failed
           to shut down the system, and we need to force a power off */
        if(shutdown_timeout) {
            shutdown_timeout -= small_ticks;
            if(shutdown_timeout <= 0)
                power_off();
        }

#ifdef HAVE_ALARM_MOD
        power_thread_rtc_process();
#endif

        /*
         * Do a digital exponential filter.  We don't sample the battery if
         * the disk is spinning unless we are in USB mode (the disk will most
         * likely always be spinning in USB mode).
         */
        if (!ata_disk_is_active() || usb_inserted()) {
            avgbat = avgbat - (avgbat / BATT_AVE_SAMPLES) +
                     adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR;
            /*
             * battery_centivolts is the centivolt-scaled filtered battery value.
             */
            battery_centivolts = avgbat / BATT_AVE_SAMPLES / 10000;

            /* update battery status every time an update is available */
            battery_status_update();

        }
#ifdef HAVE_CHARGE_CTRL
        if (ata_disk_is_active()) {
            /* flag hdd use for charging calculation */
            disk_activity_last_cycle = true;
        }
#endif
#if defined(DEBUG_FILE) && defined(HAVE_CHARGE_CTRL)
        /*
         * If we have a lot of pending writes or if the disk is spining,
         * fsync the debug log file.
         */
        if((wrcount > 10) ||
           ((wrcount > 0) && ata_disk_is_active())) {
            fsync(fd);
            wrcount = 0;
        }
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
#ifdef HAVE_CHARGE_CTRL
    unsigned int target_voltage = TRICKLE_VOLTAGE;    /* desired topoff/trickle
                                       * voltage level */
    int charge_max_time_idle = 0;     /* max. charging duration, calculated at
                                       * beginning of charging */
    int charge_max_time_now = 0;      /* max. charging duration including
                                       * hdd activity */
    int minutes_disk_activity = 0;    /* count minutes of hdd use during
                                       * charging */
    int last_disk_activity = CHARGE_END_LONGD + 1; /* last hdd use x mins ago */
#endif

    /* initialize the voltages for the exponential filter */

    avgbat = adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR *
        BATT_AVE_SAMPLES;
    battery_centivolts = avgbat / BATT_AVE_SAMPLES / 10000;

#if defined(DEBUG_FILE) && defined(HAVE_CHARGE_CTRL)
    fd      = -1;
    wrcount = 0;
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

#if CONFIG_BATTERY == BATT_LIION2200
        /* We use the information from the ADC_EXT_POWER ADC channel, which
           tells us the charging current from the LTC1734. When DC is
           connected (either via the external adapter, or via USB), we try
           to determine if it is actively charging or only maintaining the
           charge. My tests show that ADC readings below about 0x80 means
           that the LTC1734 is only maintaining the charge. */
        if(charger_inserted()) {
            if(adc_read(ADC_EXT_POWER) < 0x80) {
                charge_state = TRICKLE;
            } else {
                charge_state = CHARGING;
            }
        } else {
            charge_state = DISCHARGING;
        }
#endif /* # if CONFIG_BATTERY == BATT_LIION2200 */

#ifdef HAVE_CHARGE_CTRL
        if (charger_input_state == CHARGER_PLUGGED) {
            pid_p = 0;
            pid_i = 0;
            snprintf(power_message, POWER_MESSAGE_LEN, "Charger plugged in");
            /*
             * The charger was just plugged in.  If the battery level is
             * nearly charged, just trickle.  If the battery is low, start
             * a full charge cycle.  If the battery level is in between,
             * top-off and then trickle.
             */
            if(battery_percent > START_TOPOFF_CHG) {
                powermgmt_last_cycle_level = battery_percent;
                powermgmt_last_cycle_startstop_min = 0;
                if(battery_percent >= START_TRICKLE_CHG) {
                    charge_state = TRICKLE;
                    target_voltage = TRICKLE_VOLTAGE;
                } else {
                    charge_state = TOPOFF;
                    target_voltage = TOPOFF_VOLTAGE;
                }
            } else {
                /*
                 * Start the charger full strength
                 */
                i = CHARGE_MAX_TIME_1500 * battery_capacity / 1500;
                charge_max_time_idle =
                    i * (100 + 35 - battery_percent) / 100;
                if (charge_max_time_idle > i) {
                    charge_max_time_idle = i;
                }
                charge_max_time_now = charge_max_time_idle;

                snprintf(power_message, POWER_MESSAGE_LEN,
                         "ChgAt %d%% max %dm", battery_level(),
                         charge_max_time_now);

                /* enable the charger after the max time calc is done,
                   because battery_level depends on if the charger is
                   on */
                DEBUGF("power: charger inserted and battery"
                       " not full, charging\n");
                powermgmt_last_cycle_level = battery_percent;
                powermgmt_last_cycle_startstop_min = 0;
                trickle_sec  = 60;
                long_delta   = short_delta = 999999;
                charge_state = CHARGING;
            }
        }
        if (charge_state == CHARGING) {
            /* alter charge time max length with extra disk use */
            if (disk_activity_last_cycle) {
                minutes_disk_activity++;
                charge_max_time_now = charge_max_time_idle +
                                     (minutes_disk_activity * 2 / 5);
                disk_activity_last_cycle = false;
                last_disk_activity = 0;
            } else {
                last_disk_activity++;
            }
            /*
             * Check the delta voltage over the last X minutes so we can do
             * our end-of-charge logic based on the battery level change.
             *(no longer use minimum time as logic for charge end has 50
             * minutes minimum charge built in)
             */
            if (powermgmt_last_cycle_startstop_min > CHARGE_END_SHORTD) {
                short_delta = power_history[0] -
                              power_history[CHARGE_END_SHORTD - 1];
            }

            if (powermgmt_last_cycle_startstop_min > CHARGE_END_LONGD) {
            /*
             * Scan the history: the points where measurement is taken need to
             * be fairly static. (check prior to short delta 'area')
             * (also only check first and last 10 cycles - delta in middle OK)
             */
                long_delta = power_history[0] -
                             power_history[CHARGE_END_LONGD - 1];

                for(i = CHARGE_END_SHORTD; i < CHARGE_END_SHORTD + 10; i++) {
                    if(((power_history[i] - power_history[i+1]) >  5) ||
                       ((power_history[i] - power_history[i+1]) < -5)) {
                        long_delta = 777777;
                        break;
                    }
                }
                 for(i = CHARGE_END_LONGD - 11; i < CHARGE_END_LONGD - 1 ; i++) {
                    if(((power_history[i] - power_history[i+1]) >  5) ||
                       ((power_history[i] - power_history[i+1]) < -5)) {
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
             * Note: short_delta and long_delta are centivolts
             */
            if ((powermgmt_last_cycle_startstop_min >= charge_max_time_now) ||
                (short_delta <= -5 && long_delta < 5 ) || (long_delta  < -2 &&
                last_disk_activity > CHARGE_END_LONGD)) {
                if (powermgmt_last_cycle_startstop_min > charge_max_time_now) {
                    DEBUGF("power: powermgmt_last_cycle_startstop_min > charge_max_time_now, "
                           "enough!\n");
                    /*
                     *have charged too long and deltaV detection did not
                     *work!
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
                     * set trickle charge target to a relative voltage instead
                     * of an arbitrary value - the fully charged voltage may
                     * vary according to ambient temp, battery condition etc
                     * trickle target is -0.15v from full voltage acheived
                     * topup target is -0.05v from full voltage
                     */
                    target_voltage = power_history[0] - 15;

                } else {
                    if(short_delta <= -5) {
                        DEBUGF("power: short-term negative"
                               " delta, enough!\n");
                        snprintf(power_message, POWER_MESSAGE_LEN,
                                 "end negd %d %dmin", short_delta,
                                 powermgmt_last_cycle_startstop_min);
                        target_voltage = power_history[CHARGE_END_SHORTD - 1]
                                         - 5;
                    } else {
                        DEBUGF("power: long-term small "
                               "positive delta, enough!\n");
                        snprintf(power_message, POWER_MESSAGE_LEN,
                                 "end lowd %d %dmin", long_delta,
                                 powermgmt_last_cycle_startstop_min);
                        target_voltage = power_history[CHARGE_END_LONGD - 1]
                                         - 5;
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
        else if (charge_state > CHARGING)  /* top off or trickle */
        {
            /*
             *Time to switch from topoff to trickle?
             */
            if ((charge_state == TOPOFF) &&
                (powermgmt_last_cycle_startstop_min > TOPOFF_MAX_TIME))
            {
                powermgmt_last_cycle_level = battery_percent;
                powermgmt_last_cycle_startstop_min = 0;
                charge_state = TRICKLE;
                target_voltage = target_voltage - 10;
            }
            /*
             * Adjust trickle charge time (proportional and integral terms).
             * Note: I considered setting the level higher if the USB is
             * plugged in, but it doesn't appear to be necessary and will
             * generate more heat [gvb].
             */

            pid_p = target_voltage - battery_centivolts;
            if((pid_p > PID_DEADZONE) || (pid_p < -PID_DEADZONE))
                pid_p = pid_p * PID_PCONST;
            else
                pid_p = 0;
            if(battery_centivolts < target_voltage) {
                if(pid_i < 60) {
                    pid_i++;        /* limit so it doesn't "wind up" */
                }
            } else {
                if(pid_i > 0) {
                    pid_i--;        /* limit so it doesn't "wind up" */
                }
            }

            trickle_sec = pid_p + pid_i;

            if(trickle_sec > 60) {
                trickle_sec = 60;
            }
            if(trickle_sec < 0) {
                trickle_sec = 0;
            }

        } else if (charge_state == DISCHARGING) {
            trickle_sec = 0;
            /*
             * The charger is enabled here only in one case: if it was
             * turned on at boot time (power_init).  Turn it off now.
             */
            if (charger_enabled)
                charger_enable(false);
        }

        if (charger_input_state == CHARGER_UNPLUGGED) {
            /*
             * The charger was just unplugged.
             */
            DEBUGF("power: charger disconnected, disabling\n");

            charger_enable(false);
            powermgmt_last_cycle_level = battery_percent;
            powermgmt_last_cycle_startstop_min = 0;
            trickle_sec  = 0;
            pid_p        = 0;
            pid_i        = 0;
            charge_state = DISCHARGING;
            snprintf(power_message, POWER_MESSAGE_LEN, "Charger: discharge");
        }

#endif /* end HAVE_CHARGE_CTRL */

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
        if(usb_inserted()) {
            if(fd >= 0) {
                /* It is probably too late to close the file but we can try...*/
                close(fd);
                fd = -1;
            }
        } else {
            if(fd < 0) {
                fd = open(DEBUG_FILE_NAME, O_WRONLY | O_APPEND | O_CREAT);
                if(fd >= 0) {
                    snprintf(debug_message, DEBUG_MESSAGE_LEN,
                    "cycle_min, bat_centivolts, bat_percent, chgr_state, charge_state, pid_p, pid_i, trickle_sec\n");
                    write(fd, debug_message, strlen(debug_message));
                    wrcount = 99;   /* force a flush */
                }
            }
            if(fd >= 0) {
                snprintf(debug_message, DEBUG_MESSAGE_LEN,
                        "%d, %d, %d, %d, %d, %d, %d, %d\n",
                    powermgmt_last_cycle_startstop_min, battery_centivolts,
                    battery_percent, charger_input_state, charge_state,
                    pid_p, pid_i, trickle_sec);
                write(fd, debug_message, strlen(debug_message));
                wrcount++;
            }
        }
#endif
        handle_auto_poweroff();

#ifdef HAVE_CHARGE_CTRL
        powermgmt_last_cycle_startstop_min++;
#endif
    }
}

void powermgmt_init(void)
{
    /* init history to 0 */
    memset(power_history, 0x00, sizeof(power_history));

    create_thread(power_thread, power_stack, sizeof(power_stack),
                  power_thread_name);
}

#endif /* SIMULATOR */

void sys_poweroff(void)
{
    logf("sys_poweroff()");
    /* If the main thread fails to shut down the system, we will force a
       power off after an 8 second timeout */
    shutdown_timeout = HZ*8;

    queue_post(&button_queue, SYS_POWEROFF, NULL);
}

/* Various hardware housekeeping tasks relating to shutting down the jukebox */
void shutdown_hw(void)
{
#ifndef SIMULATOR
#if defined(DEBUG_FILE) && defined(HAVE_CHARGE_CTRL)
    if(fd >= 0) {
        close(fd);
        fd = -1;
    }
#endif
    audio_stop();
#ifdef HAVE_LCD_BITMAP
    glyph_cache_save();
#endif
    ata_flush();
    ata_spindown(1);
    while(ata_disk_is_active())
        sleep(HZ/10);

    mp3_shutdown();
#ifdef HAVE_UDA1380
    uda1380_close();
#elif defined(HAVE_TLV320)
    tlv320_close();
#elif defined(HAVE_WM8758) || defined(HAVE_WM8975)
    wmcodec_close();
#endif
#if defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR)
    backlight_set_fade_out(0);
#endif
    backlight_off();
#if defined(IPOD_ARCH) && defined(HAVE_LCD_COLOR)
    /* Clear the screen and backdrop to
    remove ghosting effect on shutdown */
    lcd_set_backdrop(NULL);
    lcd_set_background(LCD_WHITE);
    lcd_clear_display();
    lcd_update();
    sleep(HZ/16);
#endif
    lcd_set_contrast(0);
#ifdef HAVE_REMOTE_LCD
    remote_backlight_off();
    lcd_remote_set_contrast(0);
#endif
    power_off();
#endif /* #ifndef SIMULATOR */
}
