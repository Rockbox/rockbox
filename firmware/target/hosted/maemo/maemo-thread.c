/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Thomas Jarosch
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


#include <libhal.h>
#include <libosso.h>
#include <SDL_thread.h>

#include "config.h"
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "power.h"
#include "debug.h"
#include "button.h"

/* Battery status information */
#define BME_UDI "/org/freedesktop/Hal/devices/bme"
#define BATTERY_PERCENTAGE "battery.charge_level.percentage"
#define BATTER_REMAINING_TIME "battery.remaining_time"

/* Bluetooth headset support */
#define BT_HEADSET_UDI "/org/freedesktop/Hal/devices/computer_logicaldev_input_1"

GMainLoop *maemo_main_loop = NULL;
osso_context_t *maemo_osso_ctx = NULL;

volatile int maemo_display_on = 1;
volatile int maemo_battery_level = 0;
volatile int maemo_remaining_time_sec = 0;

extern void send_battery_level_event(void);
extern int last_sent_battery_level;
extern int battery_percent;

void display_status_callback(osso_display_state_t state, gpointer data)
{
    (void)data;

    if (state == OSSO_DISPLAY_OFF)
       maemo_display_on = 0;
    else
       maemo_display_on = 1;
}


void get_battery_values(LibHalContext *ctx)
{
    /* Get initial battery percentage and remaining time */
    maemo_battery_level = libhal_device_get_property_int(
                                                        ctx, BME_UDI,
                                                        BATTERY_PERCENTAGE, NULL);

    maemo_remaining_time_sec = libhal_device_get_property_int(
                                                        ctx, BME_UDI,
                                                        BATTER_REMAINING_TIME, NULL);

    DEBUGF("[maemo] Battery percentage: %d, remaining_time_sec: %d\n", maemo_battery_level, maemo_remaining_time_sec);
}

static void on_battery_changed (LibHalContext *ctx,
                                  const char *udi, 
                                  const char *key,
                                  dbus_bool_t is_removed,
                                  dbus_bool_t is_added)
{
    (void)is_removed;
    (void)is_added;

    if (!g_str_equal (udi, BME_UDI))
        return;

    if (!g_str_equal (key, BATTERY_PERCENTAGE) && !g_str_equal (key, BATTER_REMAINING_TIME))
        return;

    get_battery_values(ctx);
}

static void on_bt_button_pressed(LibHalContext *ctx,
                       const char *udi,
                       const char *condition_name,
                       const char *condition_detail)
{
    (void)ctx;

    if (!g_str_equal (udi, BT_HEADSET_UDI) || !g_str_equal(condition_name, "ButtonPressed"))
        return;

    sim_enter_irq_handler();

    if (g_str_equal(condition_detail, "play-cd") || g_str_equal(condition_detail, "pause-cd"))
        queue_post(&button_queue, BUTTON_MULTIMEDIA_PLAYPAUSE, 0);
    else if (g_str_equal(condition_detail, "stop-cd"))
        queue_post(&button_queue, BUTTON_MULTIMEDIA_STOP, 0);
    else if (g_str_equal(condition_detail, "next-song"))
        queue_post(&button_queue, BUTTON_MULTIMEDIA_NEXT, 0);
    else if (g_str_equal(condition_detail, "previous-song"))
        queue_post(&button_queue, BUTTON_MULTIMEDIA_PREV, 0);
    else if (g_str_equal(condition_detail, "fast-forward"))
        queue_post(&button_queue, BUTTON_MULTIMEDIA_FFWD, 0);
    else if (g_str_equal(condition_detail, "rewind"))
        queue_post(&button_queue, BUTTON_MULTIMEDIA_REW, 0);

    sim_exit_irq_handler();
}

int maemo_thread_func (void *wait_for_osso_startup)
{
    maemo_main_loop = g_main_loop_new (NULL, FALSE);

    /* Register display callback */
    maemo_osso_ctx = osso_initialize ("rockbox", "666", FALSE, NULL);
    osso_hw_set_display_event_cb(maemo_osso_ctx, display_status_callback, NULL);

    /* Register battery status callback */
    LibHalContext *hal_ctx;
    hal_ctx = libhal_ctx_new();

    DBusConnection *system_bus = (DBusConnection*)osso_get_sys_dbus_connection(maemo_osso_ctx);
    libhal_ctx_set_dbus_connection(hal_ctx, system_bus);

    libhal_ctx_init(hal_ctx, NULL);
    libhal_ctx_set_device_property_modified (hal_ctx, on_battery_changed);
    libhal_device_add_property_watch (hal_ctx, BME_UDI, NULL);

    /* Work around libhal API issue: We need to add a property watch
       to get the condition change callback working */
    libhal_device_add_property_watch (hal_ctx, BT_HEADSET_UDI, NULL);
    libhal_ctx_set_device_condition(hal_ctx, on_bt_button_pressed);

    get_battery_values(hal_ctx);

    /* let system_init proceed */
    SDL_SemPost((SDL_sem *)wait_for_osso_startup);

    g_main_loop_run (maemo_main_loop);

    /* Cleanup */
    osso_deinitialize (maemo_osso_ctx);
    libhal_device_remove_property_watch (hal_ctx, BT_HEADSET_UDI, NULL);
    libhal_device_remove_property_watch (hal_ctx, BME_UDI, NULL);
    libhal_ctx_shutdown (hal_ctx, NULL);
    libhal_ctx_free(hal_ctx);

    return 0;
}

/** Rockbox battery related functions */
void battery_status_update(void)
{
    battery_percent = maemo_battery_level;
    send_battery_level_event();
}

/* Returns true if any power input is connected - charging-capable
 * or not. */
bool power_input_present(void)
{
    return false;
}

unsigned battery_voltage(void)
{
    return 0;
}

/* Returns battery level in percent */
int battery_level(void)
{
    battery_status_update();
    return maemo_battery_level;
}

/* Return remaining battery time in minutes */
int battery_time(void)
{
    battery_status_update();
    return maemo_remaining_time_sec / 60;
}

bool battery_level_safe(void)
{
    return battery_level() >= 5;
}
