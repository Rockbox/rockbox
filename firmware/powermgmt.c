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
#include "audio.h"
#include "mp3_playback.h"
#include "usb.h"
#include "powermgmt.h"
#include "backlight.h"
#include "lcd.h"
#include "rtc.h"
#if CONFIG_TUNER
#include "fmradio.h"
#endif
#include "sound.h"
#ifdef HAVE_LCD_BITMAP
#include "font.h"
#endif
#if defined(HAVE_RECORDING) && (CONFIG_CODEC == SWCODEC)
#include "pcm_record.h"
#endif
#include "logf.h"
#include "lcd-remote.h"
#ifdef SIMULATOR
#include <time.h>
#endif

#if (defined(IAUDIO_X5) || defined(IAUDIO_M5)) && !defined (SIMULATOR)
#include "pcf50606.h"
#include "lcd-remote-target.h"
#endif

/*
 * Define DEBUG_FILE to create a csv (spreadsheet) with battery information
 * in it (one sample per minute).  This is only for very low level debug.
 */
#undef  DEBUG_FILE
#if defined(DEBUG_FILE) && (CONFIG_CHARGING == CHARGING_CONTROL)
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
#if CONFIG_CHARGING >= CHARGING_MONITOR
charge_state_type charge_state;     /* charging mode */
#endif

static void send_battery_level_event(void);
static int last_sent_battery_level = 100;

#if CONFIG_CHARGING
charger_input_state_type charger_input_state IDATA_ATTR;
#endif

#ifdef SIMULATOR /***********************************************************/

#define BATT_MINMVOLT   2500            /* minimum millivolts of battery */
#define BATT_MAXMVOLT   4500            /* maximum millivolts of battery */
#define BATT_MAXRUNTIME (10 * 60)       /* maximum runtime with full battery in minutes */

static unsigned int battery_millivolts = (unsigned int)BATT_MAXMVOLT;
static int battery_percent = 100;       /* battery capacity level in percent */
static int powermgmt_est_runningtime_min = BATT_MAXRUNTIME; /* estimated remaining time in minutes */

static void battery_status_update(void)
{
    static time_t last_change = 0;
    static bool charging = false;
    time_t now;

    time(&now);
    if (last_change < now)
    {
        last_change = now;

        /* change the values: */
        if (charging)
        {
            if (battery_millivolts >= BATT_MAXMVOLT)
            {
                /* Pretend the charger was disconnected */
                charging = false;
                queue_broadcast(SYS_CHARGER_DISCONNECTED, 0);
                last_sent_battery_level = 100;
            }
        }
        else
        {
            if (battery_millivolts <= BATT_MINMVOLT)
            {
                /* Pretend the charger was connected */
                charging = true;
                queue_broadcast(SYS_CHARGER_CONNECTED, 0);
                last_sent_battery_level = 0;
            }
        }
        if (charging)
            battery_millivolts += (BATT_MAXMVOLT - BATT_MINMVOLT) / 50;
        else
            battery_millivolts -= (BATT_MAXMVOLT - BATT_MINMVOLT) / 100;

        battery_percent = 100 * (battery_millivolts - BATT_MINMVOLT) / (BATT_MAXMVOLT - BATT_MINMVOLT);
        powermgmt_est_runningtime_min = battery_percent * BATT_MAXRUNTIME / 100;
    }
    send_battery_level_event();
}

void battery_read_info(int *voltage, int *level)
{
    battery_status_update();

    if (voltage)
        *voltage = battery_millivolts;

    if (level)
        *level = battery_percent;
}

unsigned int battery_voltage(void)
{
    battery_status_update();
    return battery_millivolts;
}

int battery_level(void)
{
    battery_status_update();
    return battery_percent;
}

int battery_time(void)
{
    battery_status_update();
    return powermgmt_est_runningtime_min;
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

#if BATTERY_TYPES_COUNT > 1
void set_battery_type(int type)
{
    (void)type;
}
#endif

void reset_poweroff_timer(void)
{
}

#ifdef HAVE_ACCESSORY_SUPPLY
void accessory_supply_set(bool enable)
{
    (void)enable;
}
#endif

#else /* not SIMULATOR ******************************************************/

#if CONFIG_CHARGING == CHARGING_CONTROL
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
#endif /* CONFIG_CHARGING == CHARGING_CONTROL */

/*
 * Average battery voltage and charger voltage, filtered via a digital
 * exponential filter.
 */
static unsigned int avgbat;     /* average battery voltage (filtering) */
static unsigned int battery_millivolts;/* filtered battery voltage, millivolts */

#ifdef HAVE_CHARGE_CTRL
#define BATT_AVE_SAMPLES    32  /* filter constant / @ 2Hz sample rate */
#else
#define BATT_AVE_SAMPLES   128  /* slw filter constant for all others */
#endif

/* battery level (0-100%) of this minute, updated once per minute */
static int battery_percent  = -1;
static int battery_capacity = BATTERY_CAPACITY_DEFAULT; /* default value, mAh */
#if BATTERY_TYPES_COUNT > 1
static int battery_type     = 0;
#else
#define battery_type 0
#endif

/* Power history: power_history[0] is the newest sample */
unsigned short power_history[POWER_HISTORY_LEN];

static char power_stack[DEFAULT_STACK_SIZE/2 + DEBUG_STACK];
static const char power_thread_name[] = "power";

static int poweroff_timeout = 0;
static int powermgmt_est_runningtime_min = -1;

static bool sleeptimer_active = false;
static long sleeptimer_endtick;

static long last_event_tick;

static int voltage_to_battery_level(int battery_millivolts);
static void battery_status_update(void);
static int runcurrent(void);

void battery_read_info(int *voltage, int *level)
{
    int millivolts = battery_adc_voltage();

    if (voltage)
        *voltage = millivolts;

    if (level)
        *level = voltage_to_battery_level(millivolts);
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

/* Returns filtered battery voltage [millivolts] */
unsigned int battery_voltage(void)
{
    return battery_millivolts;
}

/* Tells if the battery level is safe for disk writes */
bool battery_level_safe(void)
{
    return battery_millivolts > battery_level_dangerous[battery_type];
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

/* look into the percent_to_volt_* table and get a realistic battery level */
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
static int voltage_to_battery_level(int battery_millivolts)
{
    int level;

#if defined(CONFIG_CHARGER) \
 && (defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES))
    /* Checking for iriver is a temporary kludge.
     * This code needs rework/unification */
    if (charger_input_state == NO_CHARGER) {
        /* discharging. calculate new battery level and average with last */
        level = voltage_to_percent(battery_millivolts,
                percent_to_volt_discharge[battery_type]);
        if (level != (battery_percent - 1))
            level = (level + battery_percent + 1) / 2;
    }
    else if (charger_input_state == CHARGER_UNPLUGGED) {
        /* just unplugged. adjust filtered values */
        battery_millivolts -= percent_to_volt_charge[battery_percent/10] -
                              percent_to_volt_discharge[0][battery_percent/10];
        avgbat = battery_millivolts * 1000 * BATT_AVE_SAMPLES;
        level  = battery_percent;
    }
    else if (charger_input_state == CHARGER_PLUGGED) {
        /* just plugged in. adjust battery values */
        battery_millivolts += percent_to_volt_charge[battery_percent/10] -
                              percent_to_volt_discharge[0][battery_percent/10];
        avgbat = battery_millivolts * 1000 * BATT_AVE_SAMPLES;
        level  = MIN(12 * battery_percent / 10, 99);
    }
    else { /* charging. calculate new battery level */
        level = voltage_to_percent(battery_millivolts,
                percent_to_volt_charge);
    }
#elif CONFIG_CHARGING >= CHARGING_MONITOR
    if (charge_state == DISCHARGING) {
        level = voltage_to_percent(battery_millivolts,
                    percent_to_volt_discharge[battery_type]);
    }
    else if (charge_state == CHARGING) {
        /* battery level is defined to be < 100% until charging is finished */
        level = MIN(voltage_to_percent(battery_millivolts,
                    percent_to_volt_charge), 99);
    }
    else { /* in topoff/trickle charge, battery is by definition 100% full */
        level = 100;
    }
#else
    /* always use the discharge table */
    level = voltage_to_percent(battery_millivolts,
                percent_to_volt_discharge[battery_type]);
#endif

    return level;
}

static void battery_status_update(void)
{
    int level = voltage_to_battery_level(battery_millivolts);

    /* calculate estimated remaining running time */
    /* discharging: remaining running time */
    /* charging:    remaining charging time */
#if CONFIG_CHARGING >= CHARGING_MONITOR
    if (charge_state == CHARGING) {
        powermgmt_est_runningtime_min = (100 - level) * battery_capacity * 60
                                      / 100 / (CURRENT_MAX_CHG - runcurrent());
    }
    else
#elif CONFIG_CHARGING \
  && (defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES))
    /* Checking for iriver is a temporary kludge.
     * This code needs rework/unification */
    if (charger_inserted()) {
#ifdef IRIVER_H300_SERIES
        /* H300_SERIES use CURRENT_MAX_CHG for basic charge time (80%)
         * plus 110 min top off charge time */
        powermgmt_est_runningtime_min = ((100-level) * battery_capacity * 80
                                         /100 / CURRENT_MAX_CHG) + 110;
#else
        /* H100_SERIES scaled for 160 min basic charge time (80%) on
         * 1600 mAh battery plus 110 min top off charge time */
        powermgmt_est_runningtime_min = ((100 - level) * battery_capacity
                                         / 993) + 110;
#endif
        level = (level * 80) / 100;
        if (level > 72) { /* > 91% */
            int i = POWER_HISTORY_LEN;
            int d = 1;
#ifdef HAVE_CHARGE_STATE
            if (charge_state == DISCHARGING)
                d = -2;
#endif
            while ((i > 2) && (d > 0)) /* search zero or neg. delta */
                d = power_history[0] - power_history[--i];
            if ((((d == 0) && (i > 6)) || (d == -1)) && (i < 118)) {
                /* top off charging */
                level = MIN(80 + (i*19 / 113), 99);  /* show 81% .. 99% */
                powermgmt_est_runningtime_min = MAX(116 - i, 0);
            }
            else if ((d < 0) || (i > 117)) {
                /* charging finished */
                level = 100;
                powermgmt_est_runningtime_min = battery_capacity * 60
                                                / runcurrent();
            }
        }
    }
    else
#endif /* BATT_LIPOL1300 */
    {
        if ((battery_millivolts + 20) > percent_to_volt_discharge[0][0])
            powermgmt_est_runningtime_min = (level + battery_percent) * 60 *
                                         battery_capacity / 200 / runcurrent();

        else if (battery_millivolts <= battery_level_shutoff[0])
            powermgmt_est_runningtime_min = 0;

        else
            powermgmt_est_runningtime_min = (battery_millivolts -
                                             battery_level_shutoff[0]) / 2;
    }

    battery_percent = level;
    send_battery_level_event();
}

/*
 * We shut off in the following cases:
 * 1) The unit is idle, not playing music
 * 2) The unit is playing music, but is paused
 * 3) The battery level has reached shutdown limit
 *
 * We do not shut off in the following cases:
 * 1) The USB is connected
 * 2) The charger is connected
 * 3) We are recording, or recording with pause
 * 4) The radio is playing
 */
static void handle_auto_poweroff(void)
{
    long timeout = poweroff_timeout*60*HZ;
    int  audio_stat = audio_status();

#if CONFIG_CHARGING
    /*
     * Inhibit shutdown as long as the charger is plugged in.  If it is
     * unplugged, wait for a timeout period and then shut down.
     */
    if(charger_input_state == CHARGER || audio_stat == AUDIO_STATUS_PLAY) {
        last_event_tick = current_tick;
    }
#endif

#ifndef NO_LOW_BATTERY_SHUTDOWN
    /* switch off unit if battery level is too low for reliable operation */
    if(battery_millivolts < battery_level_shutoff[battery_type]) {
        if(!shutdown_timeout) {
            backlight_on();
            sys_poweroff();
        }
    }
#endif

    if(timeout &&
#if CONFIG_TUNER && !defined(BOOTLOADER)
       (!(get_radio_status() & FMRADIO_PLAYING)) &&
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
#if CONFIG_CHARGING && !defined(HAVE_POWEROFF_WHILE_CHARGING)
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

#if defined(HAVE_BACKLIGHT) && !defined(BOOTLOADER)
    if (backlight_get_current_timeout() == 0) /* LED always on */
        current += CURRENT_BACKLIGHT;
#endif

#if defined(HAVE_RECORDING) && defined(CURRENT_RECORD)
    if (audio_status() & AUDIO_STATUS_RECORD)
        current += CURRENT_RECORD;
#endif

#ifdef HAVE_SPDIF_POWER
    if (spdif_powered())
        current += CURRENT_SPDIF_OUT;
#endif

#ifdef HAVE_REMOTE_LCD
    if (remote_detect())
        current += CURRENT_REMOTE;
#endif

    return(current);
}


/* Check to see whether or not we've received an alarm in the last second */
#ifdef HAVE_RTC_ALARM
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

#if CONFIG_CHARGING
        /*
         * Detect charger plugged/unplugged transitions.  On a plugged or
         * unplugged event, we return immediately, run once through the main
         * loop (including the subroutines), and end up back here where we
         * transition to the appropriate steady state charger on/off state.
         */
        if(charger_inserted()
#ifdef HAVE_USB_POWER /* USB powered or USB inserted both provide power */
                || usb_powered()
#if CONFIG_CHARGING
                || (usb_inserted() && usb_charging_enabled())
#endif
#endif
                ) {
            switch(charger_input_state) {
                case NO_CHARGER:
                case CHARGER_UNPLUGGED:
                    charger_input_state = CHARGER_PLUGGED;
                    return;
                case CHARGER_PLUGGED:
                    queue_broadcast(SYS_CHARGER_CONNECTED, 0);
                    last_sent_battery_level = 0;
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
                    queue_broadcast(SYS_CHARGER_DISCONNECTED, 0);
                    last_sent_battery_level = 100;
                    charger_input_state = NO_CHARGER;
                    break;
                case CHARGER_PLUGGED:
                case CHARGER:
                    charger_input_state = CHARGER_UNPLUGGED;
                    return;
            }
        }
#endif
#if CONFIG_CHARGING == CHARGING_MONITOR
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

#endif /* CONFIG_CHARGING == CHARGING_MONITOR */

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

#ifdef HAVE_RTC_ALARM
        power_thread_rtc_process();
#endif

        /*
         * Do a digital exponential filter.  We don't sample the battery if
         * the disk is spinning unless we are in USB mode (the disk will most
         * likely always be spinning in USB mode).
         */
        if (!ata_disk_is_active() || usb_inserted()) {
            avgbat += battery_adc_voltage() - (avgbat / BATT_AVE_SAMPLES);
            /*
             * battery_millivolts is the millivolt-scaled filtered battery value.
             */
            battery_millivolts = avgbat / BATT_AVE_SAMPLES;

            /* update battery status every time an update is available */
            battery_status_update();
        }
        else if (battery_percent < 8) {
            /* If battery is low, observe voltage during disk activity.
             * Shut down if voltage drops below shutoff level and we are not
             * using NiMH or Alkaline batteries.
             */
            battery_millivolts = (battery_adc_voltage() +
                                  battery_millivolts + 1) / 2;

            /* update battery status every time an update is available */
            battery_status_update();

#ifndef NO_LOW_BATTERY_SHUTDOWN
            if (!shutdown_timeout &&
                (battery_millivolts < battery_level_shutoff[battery_type]))
                sys_poweroff();
            else
#endif
                avgbat += battery_millivolts - (avgbat / BATT_AVE_SAMPLES);
        }

#if CONFIG_CHARGING == CHARGING_CONTROL
        if (ata_disk_is_active()) {
            /* flag hdd use for charging calculation */
            disk_activity_last_cycle = true;
        }
#endif
#if defined(DEBUG_FILE) && (CONFIG_CHARGING == CHARGING_CONTROL)
        /*
         * If we have a lot of pending writes or if the disk is spining,
         * fsync the debug log file.
         */
        if((wrcount > 10) || ((wrcount > 0) && ata_disk_is_active())) {
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
#if CONFIG_CHARGING == CHARGING_CONTROL
    int i;
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

    /* Delay reading the first battery level */
#ifdef MROBE_100
    while(battery_adc_voltage()>4200)  /* gives false readings initially */
#endif
    sleep(HZ/100);

    /* initialize the voltages for the exponential filter */
    avgbat = battery_adc_voltage() + 15;

#ifndef HAVE_MMC  /* this adjustment is only needed for HD based */
        /* The battery voltage is usually a little lower directly after
           turning on, because the disk was used heavily. Raise it by 5% */
#ifdef HAVE_CHARGING
    if(!charger_inserted()) /* only if charger not connected */
#endif
        avgbat += (percent_to_volt_discharge[battery_type][6] -
                   percent_to_volt_discharge[battery_type][5]) / 2;
#endif /* not HAVE_MMC */

    avgbat = avgbat * BATT_AVE_SAMPLES;
    battery_millivolts = avgbat / BATT_AVE_SAMPLES;

#if CONFIG_CHARGING
    if(charger_inserted()) {
        battery_percent  = voltage_to_percent(battery_millivolts,
                           percent_to_volt_charge);
#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
        /* Checking for iriver is a temporary kludge. */
        charger_input_state = CHARGER;
#endif
    } else
#endif
    {   battery_percent  = voltage_to_percent(battery_millivolts,
                           percent_to_volt_discharge[battery_type]);
        battery_percent += (battery_percent < 100);
    }

#if defined(DEBUG_FILE) && (CONFIG_CHARGING == CHARGING_CONTROL)
    fd      = -1;
    wrcount = 0;
#endif

    while (1)
    {
        /* rotate the power history */
        memmove(power_history + 1, power_history,
                sizeof(power_history) - sizeof(power_history[0]));

        /* insert new value at the start, in millivolts 8-) */
        power_history[0] = battery_millivolts;

#if CONFIG_CHARGING == CHARGING_CONTROL
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
                    if(((power_history[i] - power_history[i+1]) >  50) ||
                       ((power_history[i] - power_history[i+1]) < -50)) {
                        long_delta = 777777;
                        break;
                    }
                }
                 for(i = CHARGE_END_LONGD - 11; i < CHARGE_END_LONGD - 1 ; i++) {
                    if(((power_history[i] - power_history[i+1]) >  50) ||
                       ((power_history[i] - power_history[i+1]) < -50)) {
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
             * Note: short_delta and long_delta are millivolts
             */
            if ((powermgmt_last_cycle_startstop_min >= charge_max_time_now) ||
                (short_delta <= -50 && long_delta < 50 ) || (long_delta  < -20 &&
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
                    target_voltage = power_history[0] - 150;

                } else {
                    if(short_delta <= -5) {
                        DEBUGF("power: short-term negative"
                               " delta, enough!\n");
                        snprintf(power_message, POWER_MESSAGE_LEN,
                                 "end negd %d %dmin", short_delta,
                                 powermgmt_last_cycle_startstop_min);
                        target_voltage = power_history[CHARGE_END_SHORTD - 1]
                                         - 50;
                    } else {
                        DEBUGF("power: long-term small "
                               "positive delta, enough!\n");
                        snprintf(power_message, POWER_MESSAGE_LEN,
                                 "end lowd %d %dmin", long_delta,
                                 powermgmt_last_cycle_startstop_min);
                        target_voltage = power_history[CHARGE_END_LONGD - 1]
                                         - 50;
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
        else if (charge_state != DISCHARGING)  /* top off or trickle */
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
                target_voltage = target_voltage - 100;
            }
            /*
             * Adjust trickle charge time (proportional and integral terms).
             * Note: I considered setting the level higher if the USB is
             * plugged in, but it doesn't appear to be necessary and will
             * generate more heat [gvb].
             */

            pid_p = ((signed)target_voltage - (signed)battery_millivolts) / 5;
            if((pid_p <= PID_DEADZONE) && (pid_p >= -PID_DEADZONE))
                pid_p = 0;

            if((unsigned) battery_millivolts < target_voltage) {
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

#endif /* CONFIG_CHARGING == CHARGING_CONTROL */

        /* sleep for a minute */

#if CONFIG_CHARGING == CHARGING_CONTROL
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

#if defined(DEBUG_FILE) && (CONFIG_CHARGING == CHARGING_CONTROL)
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
                    "cycle_min, bat_millivolts, bat_percent, chgr_state, charge_state, pid_p, pid_i, trickle_sec\n");
                    write(fd, debug_message, strlen(debug_message));
                    wrcount = 99;   /* force a flush */
                }
            }
            if(fd >= 0) {
                snprintf(debug_message, DEBUG_MESSAGE_LEN,
                        "%d, %d, %d, %d, %d, %d, %d, %d\n",
                    powermgmt_last_cycle_startstop_min, battery_millivolts,
                    battery_percent, charger_input_state, charge_state,
                    pid_p, pid_i, trickle_sec);
                write(fd, debug_message, strlen(debug_message));
                wrcount++;
            }
        }
#endif
        handle_auto_poweroff();

#if CONFIG_CHARGING == CHARGING_CONTROL
        powermgmt_last_cycle_startstop_min++;
#endif
    }
}

void powermgmt_init(void)
{
    /* init history to 0 */
    memset(power_history, 0x00, sizeof(power_history));
    create_thread(power_thread, power_stack, sizeof(power_stack), 0,
                  power_thread_name IF_PRIO(, PRIORITY_SYSTEM)
                  IF_COP(, CPU));
}

#endif /* SIMULATOR */

void sys_poweroff(void)
{
    logf("sys_poweroff()");
    /* If the main thread fails to shut down the system, we will force a
       power off after an 20 second timeout - 28 seconds if recording */
    if (shutdown_timeout == 0)
    {
#if (defined(IAUDIO_X5) || defined(IAUDIO_M5)) && !defined (SIMULATOR)
        pcf50606_reset_timeout(); /* Reset timer on first attempt only */
#endif
#ifdef HAVE_RECORDING
        if (audio_status() & AUDIO_STATUS_RECORD)
            shutdown_timeout += HZ*8;
#endif
        shutdown_timeout += HZ*20;
    }

    queue_broadcast(SYS_POWEROFF, 0);
}

void cancel_shutdown(void)
{
    logf("sys_cancel_shutdown()");

#if (defined(IAUDIO_X5) || defined(IAUDIO_M5)) && !defined (SIMULATOR)
    /* TODO: Move some things to target/ tree */
    if (shutdown_timeout)
        pcf50606_reset_timeout();
#endif

    shutdown_timeout = 0;
}

/* Various hardware housekeeping tasks relating to shutting down the jukebox */
void shutdown_hw(void)
{
#ifndef SIMULATOR
#if defined(DEBUG_FILE) && (CONFIG_CHARGING == CHARGING_CONTROL)
    if(fd >= 0) {
        close(fd);
        fd = -1;
    }
#endif
    audio_stop();
    if (battery_level_safe()) { /* do not save on critical battery */
#ifdef HAVE_LCD_BITMAP
        glyph_cache_save();
#endif
        if(ata_disk_is_active())
            ata_spindown(1);
    }
    while(ata_disk_is_active())
        sleep(HZ/10);

#if CONFIG_CODEC != SWCODEC
    mp3_shutdown();
#else
    audiohw_close();
#endif

    /* If HD is still active we try to wait for spindown, otherwise the
       shutdown_timeout in power_thread_sleep will force a power off */
    while(ata_disk_is_active())
        sleep(HZ/10);
#ifndef IAUDIO_X5
    lcd_set_contrast(0);
#endif /* IAUDIO_X5 */
#ifdef HAVE_REMOTE_LCD
    lcd_remote_set_contrast(0);
#endif

#ifdef HAVE_LCD_SHUTDOWN
    lcd_shutdown();
#endif

    /* Small delay to make sure all HW gets time to flush. Especially
       eeprom chips are quite slow and might be still writing the last
       byte. */
    sleep(HZ/4);
    power_off();
#endif /* #ifndef SIMULATOR */
}

/* Send system battery level update events on reaching certain significant
   levels.  This must be called after battery_percent has been updated. */
static void send_battery_level_event(void)
{
    static const int levels[] = { 5, 15, 30, 50, 0 };
    const int *level = levels;
    while (*level)
    {
        if (battery_percent <= *level && last_sent_battery_level > *level)
        { 
            last_sent_battery_level = *level;
            queue_broadcast(SYS_BATTERY_UPDATE, last_sent_battery_level);
            break;
        }
        level++;
    }
}
