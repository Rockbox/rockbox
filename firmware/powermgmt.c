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
#include "kernel.h"
#include "thread.h"
#include "debug.h"
#include "adc.h"
#include "string.h"
#include "storage.h"
#include "power.h"
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
#include "logf.h"
#include "lcd-remote.h"
#ifdef SIMULATOR
#include <time.h>
#endif

#if (defined(IAUDIO_X5) || defined(IAUDIO_M5)) && !defined (SIMULATOR)
#include "pcf50606.h"
#include "lcd-remote-target.h"
#endif

/** Shared by sim **/
int last_sent_battery_level = 100;
/* battery level (0-100%) */
int battery_percent = -1;
void send_battery_level_event(void);

#if CONFIG_CHARGING
/* State of the charger input as seen by the power thread */
enum charger_input_state_type charger_input_state;
/* Power inputs as seen by the power thread */
unsigned int power_thread_inputs;
#if CONFIG_CHARGING >= CHARGING_MONITOR
/* Charging state (mode) as seen by the power thread */
enum charge_state_type charge_state = DISCHARGING;
#endif
#endif /* CONFIG_CHARGING */

#ifndef SIMULATOR
static int shutdown_timeout = 0;
/*
 * Average battery voltage and charger voltage, filtered via a digital
 * exponential filter (aka. exponential moving average, scaled):
 * avgbat = y[n] = (N-1)/N*y[n-1] + x[n]. battery_millivolts = y[n] / N.
 */
static unsigned int avgbat;
/* filtered battery voltage, millivolts */
static unsigned int battery_millivolts;
/* default value, mAh */
static int battery_capacity = BATTERY_CAPACITY_DEFAULT;


#if BATTERY_TYPES_COUNT > 1
static int battery_type = 0;
#else
#define battery_type 0
#endif

/* Power history: power_history[0] is the newest sample */
unsigned short power_history[POWER_HISTORY_LEN];

static char power_stack[DEFAULT_STACK_SIZE/2 + POWERMGMT_DEBUG_STACK];
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
        if ((unsigned)type >= BATTERY_TYPES_COUNT)
            type = 0;

        battery_type = type;
        battery_status_update(); /* recalculate the battery status */
    }
}
#endif

void set_battery_capacity(int capacity)
{
    if (capacity > BATTERY_CAPACITY_MAX)
        capacity = BATTERY_CAPACITY_MAX;
    if (capacity < BATTERY_CAPACITY_MIN)
        capacity = BATTERY_CAPACITY_MIN;

    battery_capacity = capacity;

    battery_status_update(); /* recalculate the battery status */
}

int get_battery_capacity(void)
{
    return battery_capacity;
}

int battery_time(void)
{
    return powermgmt_est_runningtime_min;
}

/* Returns battery level in percent */
int battery_level(void)
{
#ifdef HAVE_BATTERY_SWITCH
    if ((power_input_status() & POWER_INPUT_BATTERY) == 0)
        return -1;
#endif
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
#if defined(NO_LOW_BATTERY_SHUTDOWN)
    return true;
#elif defined(HAVE_BATTERY_SWITCH)
    /* Cannot rely upon the battery reading to be valid and the
     * device could be powered externally. */
    return input_millivolts() > battery_level_dangerous[battery_type];
#else
    return battery_millivolts > battery_level_dangerous[battery_type];
#endif
}

void set_poweroff_timeout(int timeout)
{
    poweroff_timeout = timeout;
}

void set_sleep_timer(int seconds)
{
    if (seconds) {
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
    if (sleeptimer_active)
        return (sleeptimer_endtick - current_tick) / HZ;
    else
        return 0;
}

/* look into the percent_to_volt_* table and get a realistic battery level */
static int voltage_to_percent(int voltage, const short* table)
{
    if (voltage <= table[0]) {
        return 0;
    }
    else if (voltage >= table[10]) {
        return 100;
    }
    else {
        /* search nearest value */
        int i = 0;

        while (i < 10 && table[i+1] < voltage)
            i++;

        /* interpolate linear between the smaller and greater value */
        /* Tens digit, 10% per entry,  ones digit: interpolated */
        return i*10 + (voltage - table[i])*10 / (table[i+1] - table[i]);
    }
}

/* update battery level and estimated runtime, called once per minute or
 * when battery capacity / type settings are changed */
static int voltage_to_battery_level(int battery_millivolts)
{
    int level;

#if CONFIG_CHARGING >= CHARGING_MONITOR
    if (charging_state()) {
        /* battery level is defined to be < 100% until charging is finished */
        level = voltage_to_percent(battery_millivolts,
                                   percent_to_volt_charge);
        if (level > 99)
            level = 99;
    }
    else
#endif /* CONFIG_CHARGING >= CHARGING_MONITOR */
    {
        /* DISCHARGING or error state */
        level = voltage_to_percent(battery_millivolts,
                                   percent_to_volt_discharge[battery_type]);
    }

    return level;
}

static void battery_status_update(void)
{
    int level = voltage_to_battery_level(battery_millivolts);

    /* calculate estimated remaining running time */
#if CONFIG_CHARGING >= CHARGING_MONITOR
    if (charging_state()) {
        /* charging: remaining charging time */
        powermgmt_est_runningtime_min = (100 - level)*battery_capacity*60
                / 100 / (CURRENT_MAX_CHG - runcurrent());
    }
    else
#endif
    /* discharging: remaining running time */
    if ((battery_millivolts + 20) > percent_to_volt_discharge[0][0]) {
        powermgmt_est_runningtime_min = (level + battery_percent)*60
                * battery_capacity / 200 / runcurrent();
    }
    else if (battery_millivolts <= battery_level_shutoff[0]) {
        powermgmt_est_runningtime_min = 0;
    }
    else {
        powermgmt_est_runningtime_min =
            (battery_millivolts - battery_level_shutoff[0]) / 2;
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
    int audio_stat = audio_status();
    long tick = current_tick;

#if CONFIG_CHARGING
    /*
     * Inhibit shutdown as long as the charger is plugged in.  If it is
     * unplugged, wait for a timeout period and then shut down.
     */
    if (charger_input_state == CHARGER || audio_stat == AUDIO_STATUS_PLAY) {
        last_event_tick = current_tick;
    }
#endif

    if (!shutdown_timeout && query_force_shutdown()) {
        backlight_on();
        sys_poweroff();
    }

    if (timeout &&
#if CONFIG_TUNER
        !(get_radio_status() & FMRADIO_PLAYING) &&
#endif
        !usb_inserted() &&
        (audio_stat == 0 ||
         (audio_stat == (AUDIO_STATUS_PLAY | AUDIO_STATUS_PAUSE) &&
          !sleeptimer_active))) {

        if (TIME_AFTER(tick, last_event_tick + timeout) &&
            TIME_AFTER(tick, storage_last_disk_activity() + timeout)) {
            sys_poweroff();
        }
    }
    else if (sleeptimer_active) {
        /* Handle sleeptimer */
        if (TIME_AFTER(tick, sleeptimer_endtick)) {
            audio_stop();

            if (usb_inserted()
#if CONFIG_CHARGING && !defined(HAVE_POWEROFF_WHILE_CHARGING)
                || charger_input_state != NO_CHARGER
#endif
            ) {
                DEBUGF("Sleep timer timeout. Stopping...\n");
                set_sleep_timer(0);
                backlight_off(); /* Nighty, nighty... */
            }
            else {
                DEBUGF("Sleep timer timeout. Shutting off...\n");
                sys_poweroff();
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

#if MEM == 8 && !(defined(ARCHOS_ONDIOSP) || defined(ARCHOS_ONDIOFM))
    /* assuming 192 kbps, the running time is 22% longer with 8MB */
    current = CURRENT_NORMAL*100 / 122;
#else
    current = CURRENT_NORMAL;
#endif /* MEM == 8 */

#ifndef BOOTLOADER
    if (usb_inserted()
#ifdef HAVE_USB_POWER
    #if (CURRENT_USB < CURRENT_NORMAL)
       || usb_powered()
    #else
       && !usb_powered()
    #endif
#endif
    ) {
        current = CURRENT_USB;
    }

#if defined(HAVE_BACKLIGHT)
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
#endif /* BOOTLOADER */

    return current;
}


/* Check to see whether or not we've received an alarm in the last second */
#ifdef HAVE_RTC_ALARM
static void power_thread_rtc_process(void)
{
    if (rtc_check_alarm_flag())
        rtc_enable_alarm(false);
}
#endif

/* switch off unit if battery level is too low for reliable operation */
bool query_force_shutdown(void)
{
#if defined(NO_LOW_BATTERY_SHUTDOWN)
    return false;
#elif defined(HAVE_BATTERY_SWITCH)
    /* Cannot rely upon the battery reading to be valid and the
     * device could be powered externally. */
    return input_millivolts() < battery_level_shutoff[battery_type];
#else
    return battery_millivolts < battery_level_shutoff[battery_type];
#endif
}

#ifdef HAVE_BATTERY_SWITCH
/*
 * Reset the battery voltage filter to a new value and update the
 * status.
 */
void reset_battery_filter(int millivolts)
{
    avgbat = millivolts * BATT_AVE_SAMPLES;
    battery_millivolts = millivolts;
    battery_status_update();
}
#endif /* HAVE_BATTERY_SWITCH */

/** Generic charging algorithms for common charging types **/
#if CONFIG_CHARGING == 0 || CONFIG_CHARGING == CHARGING_SIMPLE
static inline void powermgmt_init_target(void)
{
    /* Nothing to do */
}

static inline void charging_algorithm_step(void)
{
    /* Nothing to do */
}

static inline void charging_algorithm_close(void)
{
    /* Nothing to do */
}
#elif CONFIG_CHARGING == CHARGING_MONITOR
/*
 * Monitor CHARGING/DISCHARGING state.
 */
static inline void powermgmt_init_target(void)
{
    /* Nothing to do */
}

static inline void charging_algorithm_step(void)
{
    switch (charger_input_state)
    {
    case CHARGER_PLUGGED:
    case CHARGER:
        if (charging_state()) {
            charge_state = CHARGING;
            break;
        }
    /* Fallthrough */
    case CHARGER_UNPLUGGED:
    case NO_CHARGER:
        charge_state = DISCHARGING;
        break;
    }
}

static inline void charging_algorithm_close(void)
{
    /* Nothing to do */
}
#endif /* CONFIG_CHARGING == * */

#if CONFIG_CHARGING
/* Shortcut function calls - compatibility, simplicity. */

/* Returns true if any power input is capable of charging. */
bool charger_inserted(void)
{
    return power_thread_inputs & POWER_INPUT_CHARGER;
}

/* Returns true if any power input is connected - charging-capable
 * or not. */
bool power_input_present(void)
{
    return power_thread_inputs & POWER_INPUT;
}

/*
 * Detect charger inserted. Return true if the state is transistional.
 */
static inline bool detect_charger(unsigned int pwr)
{
    /*
     * Detect charger plugged/unplugged transitions.  On a plugged or
     * unplugged event, we return immediately, run once through the main
     * loop (including the subroutines), and end up back here where we
     * transition to the appropriate steady state charger on/off state.
     */
    if (pwr & POWER_INPUT_CHARGER) {
        switch (charger_input_state)
        {
        case NO_CHARGER:
        case CHARGER_UNPLUGGED:
            charger_input_state = CHARGER_PLUGGED;
            break;

        case CHARGER_PLUGGED:
            queue_broadcast(SYS_CHARGER_CONNECTED, 0);
            last_sent_battery_level = 0;
            charger_input_state = CHARGER;
            break;

        case CHARGER:
            /* Steady state */
            return false;
        }
    }
    else {    /* charger not inserted */
        switch (charger_input_state)
        {
        case NO_CHARGER:
            /* Steady state */
            return false;

        case CHARGER_UNPLUGGED:
            queue_broadcast(SYS_CHARGER_DISCONNECTED, 0);
            last_sent_battery_level = 100;
            charger_input_state = NO_CHARGER;
            break;

        case CHARGER_PLUGGED:
        case CHARGER:
            charger_input_state = CHARGER_UNPLUGGED;
            break;
        }
    }

    /* Transitional state */
    return true;
}
#endif /* CONFIG_CHARGING */

/*
 * Monitor the presence of a charger and perform critical frequent steps
 * such as running the battery voltage filter.
 */
static inline void power_thread_step(void)
{
    /* If the power off timeout expires, the main thread has failed
       to shut down the system, and we need to force a power off */
    if (shutdown_timeout) {
        shutdown_timeout -= POWER_THREAD_STEP_TICKS;

        if (shutdown_timeout <= 0)
            power_off();
    }

#ifdef HAVE_RTC_ALARM
    power_thread_rtc_process();
#endif

    /*
     * Do a digital exponential filter.  We don't sample the battery if
     * the disk is spinning unless we are in USB mode (the disk will most
     * likely always be spinning in USB mode) or charging.
     */
    if (!storage_disk_is_active() || usb_inserted()
#if CONFIG_CHARGING >= CHARGING_MONITOR
            || charger_input_state == CHARGER
#endif
    ) {
        avgbat += battery_adc_voltage() - avgbat / BATT_AVE_SAMPLES;
        /*
         * battery_millivolts is the millivolt-scaled filtered battery value.
         */
        battery_millivolts = avgbat / BATT_AVE_SAMPLES;

        /* update battery status every time an update is available */
        battery_status_update();
    }
    else if (battery_percent < 8) {
        /*
         * If battery is low, observe voltage during disk activity.
         * Shut down if voltage drops below shutoff level and we are not
         * using NiMH or Alkaline batteries.
         */
        battery_millivolts = (battery_adc_voltage() +
                              battery_millivolts + 1) / 2;

        /* update battery status every time an update is available */
        battery_status_update();

        if (!shutdown_timeout && query_force_shutdown()) {
            sys_poweroff();
        }
        else {
            avgbat += battery_millivolts - avgbat / BATT_AVE_SAMPLES;
        }
    }
} /* power_thread_step */

static void power_thread(void)
{
    long next_power_hist;

    /* Delay reading the first battery level */
#ifdef MROBE_100
    while (battery_adc_voltage() > 4200) /* gives false readings initially */
#endif
    {
        sleep(HZ/100);
    }

#if CONFIG_CHARGING
    /* Initialize power input status before calling other routines. */
    power_thread_inputs = power_input_status();
#endif

    /* initialize the voltages for the exponential filter */
    avgbat = battery_adc_voltage() + 15;

#ifdef HAVE_DISK_STORAGE /* this adjustment is only needed for HD based */
    /* The battery voltage is usually a little lower directly after
       turning on, because the disk was used heavily. Raise it by 5% */
#if CONFIG_CHARGING
    if (!charger_inserted()) /* only if charger not connected */
#endif
    {
        avgbat += (percent_to_volt_discharge[battery_type][6] -
                   percent_to_volt_discharge[battery_type][5]) / 2;
    }
#endif /* HAVE_DISK_STORAGE */

    avgbat = avgbat * BATT_AVE_SAMPLES;
    battery_millivolts = avgbat / BATT_AVE_SAMPLES;
    power_history[0] = battery_millivolts;

#if CONFIG_CHARGING
    if (charger_inserted()) {
        battery_percent = voltage_to_percent(battery_millivolts,
                            percent_to_volt_charge);
    }
    else
#endif
    {
        battery_percent = voltage_to_percent(battery_millivolts,
                            percent_to_volt_discharge[battery_type]);
        battery_percent += battery_percent < 100;
    }

    powermgmt_init_target();

    next_power_hist = current_tick + HZ*60;

    while (1)
    {
#if CONFIG_CHARGING >= CHARGING_MONITOR
        unsigned int pwr = power_input_status();
#ifdef HAVE_BATTERY_SWITCH
        if ((pwr ^ power_thread_inputs) & POWER_INPUT_BATTERY) {
            sleep(HZ/10);
            reset_battery_filter(battery_adc_voltage());
        }
#endif
        power_thread_inputs = pwr;

        if (!detect_charger(pwr))
#endif /* CONFIG_CHARGING */
        {
            /* Steady state */
            sleep(POWER_THREAD_STEP_TICKS);

            /* Do common power tasks */
            power_thread_step();
        }

        /* Perform target tasks */
        charging_algorithm_step();

        if (TIME_BEFORE(current_tick, next_power_hist))
            continue;

        /* increment to ensure there is a record for every minute
         * rather than go forward from the current tick */
        next_power_hist += HZ*60;

        /* rotate the power history */
        memmove(&power_history[1], &power_history[0],
                sizeof(power_history) - sizeof(power_history[0]));

        /* insert new value at the start, in millivolts 8-) */
        power_history[0] = battery_millivolts;

        handle_auto_poweroff();
    }
} /* power_thread */

void powermgmt_init(void)
{
    /* init history to 0 */
    memset(power_history, 0, sizeof(power_history));
    create_thread(power_thread, power_stack, sizeof(power_stack), 0,
                  power_thread_name IF_PRIO(, PRIORITY_SYSTEM)
                  IF_COP(, CPU));
}

/* Various hardware housekeeping tasks relating to shutting down the player */
void shutdown_hw(void)
{
    charging_algorithm_close();
    audio_stop();

    if (battery_level_safe()) { /* do not save on critical battery */
#ifdef HAVE_LCD_BITMAP
        glyph_cache_save();
#endif
        if (storage_disk_is_active())
            storage_spindown(1);
    }

    while (storage_disk_is_active())
        sleep(HZ/10);

#if CONFIG_CODEC == SWCODEC
    audiohw_close();
#else
    mp3_shutdown();
#endif

    /* If HD is still active we try to wait for spindown, otherwise the
       shutdown_timeout in power_thread_step will force a power off */
    while (storage_disk_is_active())
        sleep(HZ/10);

#ifndef HAVE_LCD_COLOR
    lcd_set_contrast(0);
#endif
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
}

void sys_poweroff(void)
{
#ifndef BOOTLOADER
    logf("sys_poweroff()");
    /* If the main thread fails to shut down the system, we will force a
       power off after an 20 second timeout - 28 seconds if recording */
    if (shutdown_timeout == 0) {
#if defined(IAUDIO_X5) || defined(IAUDIO_M5)
        pcf50606_reset_timeout(); /* Reset timer on first attempt only */
#endif
#ifdef HAVE_RECORDING
        if (audio_status() & AUDIO_STATUS_RECORD)
            shutdown_timeout += HZ*8;
#endif
        shutdown_timeout += HZ*20;
    }

    queue_broadcast(SYS_POWEROFF, 0);
#endif /* BOOTLOADER */
}

void cancel_shutdown(void)
{
    logf("cancel_shutdown()");

#if defined(IAUDIO_X5) || defined(IAUDIO_M5)
    /* TODO: Move some things to target/ tree */
    if (shutdown_timeout)
        pcf50606_reset_timeout();
#endif

    shutdown_timeout = 0;
}
#endif /* SIMULATOR */

/* Send system battery level update events on reaching certain significant
   levels. This must be called after battery_percent has been updated. */
void send_battery_level_event(void)
{
    static const int levels[] = { 5, 15, 30, 50, 0 };
    const int *level = levels;

    while (*level)
    {
        if (battery_percent <= *level && last_sent_battery_level > *level) {
            last_sent_battery_level = *level;
            queue_broadcast(SYS_BATTERY_UPDATE, last_sent_battery_level);
            break;
        }

        level++;
    }
}
