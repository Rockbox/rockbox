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
#include "usb.h"
#include "powermgmt.h"
#include "backlight.h"
#include "lcd.h"
#include "rtc.h"
#if CONFIG_TUNER
#include "fmradio.h"
#endif
#include "sound.h"
#include "font.h"
#include "logf.h"
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
#include <time.h>
#endif

#if (defined(IAUDIO_X5) || defined(IAUDIO_M5) || defined(COWON_D2)) \
    && !defined (SIMULATOR)
#include "pcf50606.h"
#endif

static int last_sent_battery_level = 100;
static void set_sleep_timer(int seconds);

static bool sleeptimer_active = false;
static long sleeptimer_endtick;
/* Whether an active sleep timer should be restarted when a key is pressed */
static bool sleeptimer_key_restarts = false;
/* The number of seconds the sleep timer was last set to */
static unsigned int sleeptimer_duration = 0;

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

static int shutdown_timeout = 0;

void handle_auto_poweroff(void);
static int poweroff_timeout = 0;
static long last_event_tick = 0;

#if BATTERY_CAPACITY_INC > 0
static int battery_capacity = BATTERY_CAPACITY_DEFAULT;
#else
# define battery_capacity BATTERY_CAPACITY_DEFAULT
#endif

#if BATTERY_TYPES_COUNT > 1
static int battery_type = 0;
#else
#define battery_type 0
#endif

/* Power history: power_history[0] is the newest sample */
unsigned short power_history[POWER_HISTORY_LEN] = {0};

#if (CONFIG_CPU == JZ4732) || (CONFIG_CPU == JZ4760B) || \
    (CONFIG_CPU == X1000) || (CONFIG_PLATFORM & PLATFORM_HOSTED)
static char power_stack[DEFAULT_STACK_SIZE];
#else
static char power_stack[DEFAULT_STACK_SIZE/2];
#endif
static const char power_thread_name[] = "power";

/* Time estimation requires 64 bit math so don't use it in the bootloader.
 * Also we need to be able to measure current, and not have a better time
 * estimate source available. */
#define HAVE_TIME_ESTIMATION \
    (!defined(BOOTLOADER) && !(CONFIG_BATTERY_MEASURE & TIME_MEASURE) && \
     (defined(CURRENT_NORMAL) || (CONFIG_BATTERY_MEASURE & CURRENT_MEASURE)))

#if !(CONFIG_BATTERY_MEASURE & PERCENTAGE_MEASURE)
int _battery_level(void) { return -1; }
#endif
static int percent_now; /* Cached to avoid polling too often */

#if !(CONFIG_BATTERY_MEASURE & TIME_MEASURE)
int _battery_time(void) { return -1; }
#else
static int time_now; /* Cached to avoid polling too often */
#endif

#if HAVE_TIME_ESTIMATION
static int time_now;     /* reported time in minutes */
static int64_t time_cnt; /* reported time in seconds */
static int64_t time_err; /* error... it's complicated */
#endif

#if !(CONFIG_BATTERY_MEASURE & VOLTAGE_MEASURE)
int _battery_voltage(void) { return -1; }
#else
/* Data for the digital exponential filter */
static int voltage_avg, voltage_now;
#endif

#if !(CONFIG_BATTERY_MEASURE & CURRENT_MEASURE)
int _battery_current(void) { return -1; }
#else
static int current_avg, current_now;
#endif

/* The battery level can be obtained in two ways. If the target reports
 * voltage, the battery level can be estminated using percent_to_volt_*
 * curves. If the target can report the percentage directly, then that
 * will be used instead of voltage-based estimation. */
int battery_level(void)
{
#ifdef HAVE_BATTERY_SWITCH
    if ((power_input_status() & POWER_INPUT_BATTERY) == 0)
        return -1;
#endif

    return percent_now;
}

/* The time remaining to full charge/discharge can be provided by the
 * target if it has an accurate way of doing this. Otherwise, if the
 * target defines a valid battery capacity and can report the charging
 * and discharging current, the time remaining will be estimated based
 * on the battery level and the actual current usage. */
int battery_time(void)
{
#if (CONFIG_BATTERY_MEASURE & TIME_MEASURE) || HAVE_TIME_ESTIMATION
    return time_now;
#else
    return -1;
#endif
}

/* Battery voltage should always be reported if available, but it is
 * optional if the the target reports battery percentage directly. */
int battery_voltage(void)
{
#if CONFIG_BATTERY_MEASURE & VOLTAGE_MEASURE
    return voltage_now;
#else
    return -1;
#endif
}

/* Battery current can be estimated if the target defines CURRENT_NORMAL
 * as the number of milliamps usually consumed by the device in a normal
 * state. The target can also define other CURRENT_* values to estimate
 * the power consumed by the backlight, remote display, SPDIF, etc. */
int battery_current(void)
{
#if CONFIG_BATTERY_MEASURE & CURRENT_MEASURE
    return current_now;
#elif defined(CURRENT_NORMAL)
    int current = CURRENT_NORMAL;

#ifndef BOOTLOADER
    if (usb_inserted()
#ifdef HAVE_USB_POWER
    #if (CURRENT_USB < CURRENT_NORMAL)
       || usb_powered_only()
    #else
       && !usb_powered_only()
    #endif
#endif
    ) {
        current = CURRENT_USB;
    }

#if defined(HAVE_BACKLIGHT) && defined(CURRENT_BACKLIGHT)
    if (backlight_get_current_timeout() == 0) /* LED always on */
        current += CURRENT_BACKLIGHT;
#endif

#if defined(HAVE_RECORDING) && defined(CURRENT_RECORD)
    if (audio_status() & AUDIO_STATUS_RECORD)
        current += CURRENT_RECORD;
#endif

#if defined(HAVE_SPDIF_POWER) && defined(CURRENT_SPDIF_OUT)
    if (spdif_powered())
        current += CURRENT_SPDIF_OUT;
#endif

#if defined(HAVE_REMOTE_LCD) && defined(CURRENT_REMOTE)
    if (remote_detect())
        current += CURRENT_REMOTE;
#endif

#if defined(HAVE_ATA_POWER_OFF) && defined(CURRENT_ATA)
    if (ide_powered())
        current += CURRENT_ATA;
#endif

#if CONFIG_CHARGING >= CHARGING_MONITOR
    /* While charging we must report the charging current. */
    if (charging_state()) {
        current = CURRENT_MAX_CHG - current;
        current = MIN(current, 1);
    }
#endif

#endif /* BOOTLOADER */

    return current;
#else
    return -1;
#endif
}

/* Initialize the battery voltage/current filters. This is called
 * once by the power thread before entering the main polling loop. */
static void average_init(void)
{
#if CONFIG_BATTERY_MEASURE & VOLTAGE_MEASURE
    voltage_now = _battery_voltage() + 15;

    /* The battery voltage is usually a little lower directly after
       turning on, because the disk was used heavily. Raise it by 5% */
#ifdef HAVE_DISK_STORAGE
#if CONFIG_CHARGING
    if(!charger_inserted())
#endif
    {
        voltage_now += (percent_to_volt_discharge[battery_type][6] -
                        percent_to_volt_discharge[battery_type][5]) / 2;
    }
#endif /* HAVE_DISK_STORAGE */

    voltage_avg = voltage_now * BATT_AVE_SAMPLES;
#endif /* CONFIG_BATTERY_MEASURE & VOLTAGE_MEASURE */

#if CONFIG_BATTERY_MEASURE & CURRENT_MEASURE
    current_now = _battery_current();
    current_avg = current_now * BATT_CURRENT_AVE_SAMPLES;
#endif
}

/* Sample the battery voltage/current and update the filters.
 * Updated once every POWER_THREAD_STEP_TICKS. */
static void average_step(bool low_battery)
{
#if CONFIG_BATTERY_MEASURE & VOLTAGE_MEASURE
    int millivolts = _battery_voltage();
    if(low_battery) {
        voltage_now = (millivolts + voltage_now + 1) / 2;
        voltage_avg += voltage_now - voltage_avg / BATT_AVE_SAMPLES;
    } else {
        voltage_avg += millivolts - voltage_avg / BATT_AVE_SAMPLES;
        voltage_now = voltage_avg / BATT_AVE_SAMPLES;
    }
#else
    (void)low_battery;
#endif

#if CONFIG_BATTERY_MEASURE & CURRENT_MEASURE
    current_avg += _battery_current() - current_avg / BATT_CURRENT_AVE_SAMPLES;
    current_now = current_avg / BATT_CURRENT_AVE_SAMPLES;
#endif
}

/* Send system battery level update events on reaching certain significant
 * levels. This is called by battery_status_update() and does not have to
 * be called separately. */
static void send_battery_level_event(int percent)
{
    static const int levels[] = { 5, 15, 30, 50, 0 };
    const int *level = levels;

    while (*level)
    {
        if (percent <= *level && last_sent_battery_level > *level) {
            last_sent_battery_level = *level;
            queue_broadcast(SYS_BATTERY_UPDATE, last_sent_battery_level);
            break;
        }

        level++;
    }
}

#if !(CONFIG_BATTERY_MEASURE & PERCENTAGE_MEASURE) && \
    (CONFIG_BATTERY_MEASURE & VOLTAGE_MEASURE)
/* Look into the percent_to_volt_* table and estimate the battery level. */
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

/* Convert voltage to a battery level percentage using the appropriate
 * percent_to_volt_* lookup table. */
static int voltage_to_battery_level(int millivolts)
{
    int level;

    if (millivolts < 0)
        return -1;

#if CONFIG_CHARGING >= CHARGING_MONITOR
    if (charging_state()) {
        /* battery level is defined to be < 100% until charging is finished */
        level = voltage_to_percent(millivolts, percent_to_volt_charge);
        if (level > 99)
            level = 99;
    }
    else
#endif /* CONFIG_CHARGING >= CHARGING_MONITOR */
    {
        /* DISCHARGING or error state */
        level = voltage_to_percent(millivolts, percent_to_volt_discharge[battery_type]);
    }

    return level;
}
#endif

/* Update battery percentage and time remaining information.
 *
 * This will be called by the power thread after polling new battery data.
 * It must also be called if the battery type or capacity changes.
 */
static void battery_status_update(void)
{
#if CONFIG_BATTERY_MEASURE & PERCENTAGE_MEASURE
    int level = _battery_level();
#elif CONFIG_BATTERY_MEASURE & VOLTAGE_MEASURE
    int level = voltage_to_battery_level(voltage_now);
#else
    /* This should be a compile time error? */
    int level = -1;
#endif

#if CONFIG_BATTERY_MEASURE & TIME_MEASURE
    time_now = _battery_time();
#elif HAVE_TIME_ESTIMATION
    /* TODO: This is essentially a bad version of coloumb counting,
     * so in theory using coloumb counters when they are available
     * should provide a more accurate result. Also note that this
     * is hard-coded with a HZ/2 update rate to simplify arithmetic. */

    int current = battery_current();
    int resolution = battery_capacity * 36;

    int time_est = 0;
    if(level >= 0 && current > 0) {
#if CONFIG_CHARGING >= CHARGING_MONITOR
        if (charging_state())
            time_est = (100 - level) * battery_capacity * 36 / current;
        else
#endif
            time_est = level * battery_capacity * 36 / current;

        /* The first term nudges the counter toward the estimate. */
        time_err += current * (time_est - time_cnt);
    }

    /* The second term decrements the counter due to elapsed time. */
    time_err -= resolution;

    /* Arbitrary cutoff to ensure we don't get too far out
     * of sync. Seems to work well on synthetic tests. */
    if(time_err >  resolution * 12 ||
       time_err < -resolution * 13) {
        time_cnt = time_est;
        time_err = 0;
    }

    if(resolution > 0) {
        /* Convert the error into a time and adjust the counter. */
        int64_t adjustment = time_err / (2 * resolution);
        time_cnt += adjustment;
        time_err -= adjustment * (2 * resolution);
    }

    /* Update the reported time based on the counter. */
    time_now = (time_cnt + 30) / 60;
    if(time_now < 0)
        time_now = 0;
#endif

    percent_now = level;
    send_battery_level_event(level);
}

void battery_read_info(int *voltage, int *level)
{
    int millivolts = _battery_voltage();

    if (voltage)
        *voltage = millivolts;

    if (level)  {
#if (CONFIG_BATTERY_MEASURE & PERCENTAGE_MEASURE)
        *level = _battery_level();
#elif (CONFIG_BATTERY_MEASURE & VOLTAGE_MEASURE)
        *level = voltage_to_battery_level(millivolts);
#else
        *level = -1;
#endif
    }
}

#if BATTERY_TYPES_COUNT > 1
void set_battery_type(int type)
{
    if(type < 0 || type > BATTERY_TYPES_COUNT)
        type = 0;

    if (type != battery_type) {
        battery_type = type;
        battery_status_update(); /* recalculate the battery status */
    }
}
#endif

#if BATTERY_CAPACITY_INC > 0
void set_battery_capacity(int capacity)
{
    if (capacity > BATTERY_CAPACITY_MAX)
        capacity = BATTERY_CAPACITY_MAX;
    if (capacity < BATTERY_CAPACITY_MIN)
        capacity = BATTERY_CAPACITY_MIN;

    if (capacity != battery_capacity) {
        battery_capacity = capacity;
        battery_status_update(); /* recalculate the battery status */
    }
}
#endif

int get_battery_capacity(void)
{
    return battery_capacity;
}

/* Tells if the battery level is safe for disk writes */
bool battery_level_safe(void)
{
#if defined(NO_LOW_BATTERY_SHUTDOWN)
    return true;
#elif CONFIG_BATTERY_MEASURE & PERCENTAGE_MEASURE
    return percent_now > 0;
#elif defined(HAVE_BATTERY_SWITCH)
    /* Cannot rely upon the battery reading to be valid and the
     * device could be powered externally. */
    return input_millivolts() > battery_level_dangerous[battery_type];
#else
    return voltage_now > battery_level_dangerous[battery_type];
#endif
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
#elif CONFIG_BATTERY_MEASURE & PERCENTAGE_MEASURE
    return percent_now == 0;
#elif defined(HAVE_BATTERY_SWITCH)
    /* Cannot rely upon the battery reading to be valid and the
     * device could be powered externally. */
    return input_millivolts() < battery_level_shutoff[battery_type];
#else
    return voltage_now < battery_level_shutoff[battery_type];
#endif
}

#if defined(HAVE_BATTERY_SWITCH) || defined(HAVE_RESET_BATTERY_FILTER)
/*
 * Reset the battery voltage filter to a new value and update the
 * status.
 */
void reset_battery_filter(int millivolts)
{
    voltage_avg = millivolts * BATT_AVE_SAMPLES;
    voltage_now = millivolts;
#if CONFIG_BATTERY_MEASURE & CURRENT_MEASURE
    /* current would probably be inaccurate too */
    current_now = _battery_current();
    current_avg = current_now * BATT_CURRENT_AVE_SAMPLES;
#endif
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
#ifndef BOOTLOADER
    unsigned int data = power_thread_inputs;
#else
    unsigned int data = power_input_status();
#endif
    return data & POWER_INPUT_CHARGER;
}

/* Returns true if any power input is connected - charging-capable
 * or not. */
bool power_input_present(void)
{
#ifndef BOOTLOADER
    unsigned int data = power_thread_inputs;
#else
    unsigned int data = power_input_status();
#endif
    return data & POWER_INPUT;
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

#if CONFIG_BATTERY_MEASURE & VOLTAGE_MEASURE
static int power_hist_item(void)
{
    return voltage_now;
}
#else
static int power_hist_item(void)
{
    return percent_now;
}
#endif

static void collect_power_history(void)
{
    /* rotate the power history */
    memmove(&power_history[1], &power_history[0],
            sizeof(power_history) - sizeof(power_history[0]));
    power_history[0] = power_hist_item();
}

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
        average_step(false);
        battery_status_update();
    }
    else if (percent_now < 8) {
        average_step(true);
        battery_status_update();

        /*
         * If battery is low, observe voltage during disk activity.
         * Shut down if voltage drops below shutoff level and we are not
         * using NiMH or Alkaline batteries.
         */
        if (!shutdown_timeout && query_force_shutdown()) {
            sys_poweroff();
        }
    }
} /* power_thread_step */

static void power_thread(void)
{
    long next_power_hist;

    /* Delay reading the first battery level */
#ifdef MROBE_100
    while (_battery_voltage() > 4200) /* gives false readings initially */
    {
#elif defined(DX50) || defined(DX90)
    while (_battery_voltage() < 1) /* can give false readings initially */
    {
#elif defined(EROS_QN) || defined(FIIO_M3K) || defined(SHANLING_Q1)

    /* wait until the first battery read is ready */
    while (_battery_voltage() <= 0)
    {
#else
    {
#endif
        sleep(HZ/100);
    }

#if CONFIG_CHARGING
    /* Initialize power input status before calling other routines. */
    power_thread_inputs = power_input_status();
#endif

    /* call target specific init now */
    powermgmt_init_target();
    /* initialize voltage averaging (if available) */
    average_init();
    /* get initial battery level value (in %) */
    battery_status_update();
    /* get some initial data for the power curve */
    collect_power_history();

    next_power_hist = current_tick + HZ*60;

    while (1)
    {
#if CONFIG_CHARGING
        unsigned int pwr = power_input_status();
#ifdef HAVE_BATTERY_SWITCH
        if ((pwr ^ power_thread_inputs) & POWER_INPUT_BATTERY) {
            sleep(HZ/10);
            reset_battery_filter(_battery_voltage());
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

        /* check if some idle or sleep timer wears off */
        handle_auto_poweroff();

        if (TIME_AFTER(current_tick, next_power_hist)) {
            /* increment to ensure there is a record for every minute
             * rather than go forward from the current tick */
            next_power_hist += HZ*60;
            collect_power_history();
        }
    }
} /* power_thread */

void powermgmt_init(void)
{
    create_thread(power_thread, power_stack, sizeof(power_stack), 0,
                  power_thread_name IF_PRIO(, PRIORITY_SYSTEM)
                  IF_COP(, CPU));
}

/* Various hardware housekeeping tasks relating to shutting down the player */
void shutdown_hw(enum shutdown_type sd_type)
{
    charging_algorithm_close();
    audio_stop();

    if (battery_level_safe()) { /* do not save on critical battery */
        font_unload_all();

/* Commit pending writes if needed. Even though we don't do write caching,
   things like flash translation layers may need this to commit scattered
   pages to their final locations. So far only used for iPod Nano 2G. */
#ifdef HAVE_STORAGE_FLUSH
        storage_flush();
#endif

        if (storage_disk_is_active())
            storage_spindown(1);
    }

    audiohw_close();

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

    switch (sd_type) {
    case SHUTDOWN_POWER_OFF:
    default:
        power_off();
        break;

    case SHUTDOWN_REBOOT:
        system_reboot();
        break;
    }
}

void set_poweroff_timeout(int timeout)
{
    poweroff_timeout = timeout;
}

void reset_poweroff_timer(void)
{
    last_event_tick = current_tick;
    if (sleeptimer_active && sleeptimer_key_restarts)
        set_sleep_timer(sleeptimer_duration);
}

#ifndef BOOTLOADER
static void sys_shutdown_common(void)
{
    /* If the main thread fails to shut down the system, we will force a
       power off after an 20 second timeout - 28 seconds if recording */
    if (shutdown_timeout == 0) {
#if (defined(IAUDIO_X5) || defined(IAUDIO_M5) || defined(COWON_D2)) && !defined(SIMULATOR)
        pcf50606_reset_timeout(); /* Reset timer on first attempt only */
#endif
#ifdef HAVE_RECORDING
        if (audio_status() & AUDIO_STATUS_RECORD)
            shutdown_timeout += HZ*8;
#endif
#ifdef IPOD_NANO2G
        /* The FTL alone may take half a minute to shut down cleanly. */
        shutdown_timeout += HZ*60;
#else
        shutdown_timeout += HZ*20;
#endif
    }
}
#endif /* BOOTLOADER */

void sys_poweroff(void)
{
#ifndef BOOTLOADER
    logf("sys_poweroff()");
    sys_shutdown_common();
    queue_broadcast(SYS_POWEROFF, 0);
#endif
}

/* not to be confused with system_reboot... :( */
void sys_reboot(void)
{
#ifndef BOOTLOADER
    logf("sys_reboot()");
    sys_shutdown_common();
    queue_broadcast(SYS_REBOOT, 0);
#endif
}

void cancel_shutdown(void)
{
    logf("cancel_shutdown()");

#if (defined(IAUDIO_X5) || defined(IAUDIO_M5) || defined(COWON_D2)) && !defined(SIMULATOR)
    /* TODO: Move some things to target/ tree */
    if (shutdown_timeout)
        pcf50606_reset_timeout();
#endif

    shutdown_timeout = 0;
}

void set_sleeptimer_duration(int minutes)
{
    set_sleep_timer(minutes * 60);
}

static void set_sleep_timer(int seconds)
{
    if (seconds) {
        sleeptimer_active  = true;
        sleeptimer_endtick = current_tick + seconds * HZ;
    }
    else {
        sleeptimer_active  = false;
        sleeptimer_endtick = 0;
    }
    sleeptimer_duration = seconds;
}

int get_sleep_timer(void)
{
    if (sleeptimer_active && (sleeptimer_endtick >= current_tick))
        return (sleeptimer_endtick - current_tick) / HZ;
    else
        return 0;
}

void set_keypress_restarts_sleep_timer(bool enable)
{
    sleeptimer_key_restarts = enable;
}

#ifndef BOOTLOADER
static void handle_sleep_timer(void)
{
    if (!sleeptimer_active)
      return;

    /* Handle sleeptimer */
    if (TIME_AFTER(current_tick, sleeptimer_endtick)) {
        if (usb_inserted()
#if CONFIG_CHARGING && !defined(HAVE_POWEROFF_WHILE_CHARGING)
            || charger_input_state != NO_CHARGER
#endif
        ) {
            DEBUGF("Sleep timer timeout. Stopping...\n");
            audio_pause();
            set_sleep_timer(0);
            backlight_off(); /* Nighty, nighty... */
        }
        else {
            DEBUGF("Sleep timer timeout. Shutting off...\n");
            sys_poweroff();
        }
    }
}
#endif /* BOOTLOADER */

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
void handle_auto_poweroff(void)
{
#ifndef BOOTLOADER
    long timeout = poweroff_timeout*60*HZ;
    int audio_stat = audio_status();
    long tick = current_tick;

    /*
     * Inhibit shutdown as long as the charger is plugged in.  If it is
     * unplugged, wait for a timeout period and then shut down.
     */
    if (audio_stat == AUDIO_STATUS_PLAY
#if CONFIG_CHARGING
        || charger_input_state == CHARGER
#endif
    ) {
        last_event_tick = current_tick;
    }

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

        if (TIME_AFTER(tick, last_event_tick + timeout)
#if !(CONFIG_PLATFORM & PLATFORM_HOSTED)
            && TIME_AFTER(tick, storage_last_disk_activity() + timeout)
#endif
        ) {
            sys_poweroff();
        }
    } else
        handle_sleep_timer();
#endif
}
