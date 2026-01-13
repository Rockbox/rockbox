/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 by Aidan MacDonald
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
#include "kernel/kernel-internal.h"
#include "system.h"
#include "power.h"
#include "lcd.h"
#include "backlight.h"
#include "button.h"
#include "storage.h"
#include "disk.h"
#include "file_internal.h"
#include "usb.h"
#include "rbversion.h"
#include "system-echoplayer.h"
#include "gpio-stm32h7.h"

/* Events for the monitor callback to signal the main thread */
#define EV_POWER_PRESSED    MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0)
#define EV_POWER_RELEASED   MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 1)
#define EV_USB_UNPLUGGED    MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 2)

/* Long press duration for power button */
#define POWERBUTTON_LONG_PRESS_TIME     (HZ)

/* Time to remain powered with no USB cable inserted */
#define USB_UNPLUGGED_ACTIVE_TIME       (30 * HZ)
#define USB_UNPLUGGED_INACTIVE_TIME     (3 * HZ)

/* Power button monitor state */
static bool pwr_curr_state;
static bool pwr_prev_state;
struct timeout pwr_stable_tmo;

/* USB monitor state */
static bool usb_curr_state;
static bool usb_prev_state;
struct timeout usb_unplugged_tmo;

static volatile bool restart_pwr_stable_tmo;
static volatile bool restart_usb_unplugged_tmo;

/*
 * Because power is always enabled while USB is plugged in the
 * bootloader decides whether to appear "active" or "inactive"
 * to the user.
 */
static bool is_active;

/*
 * This flag is set if the bootloader is entered after software
 * poweroff. The user may still be holding the power button and
 * we don't want to boot Rockbox because of this, so we have to
 * wait for the power button to be released first.
 */
static bool wait_for_power_released;

/* Optional error message displayed on LCD */
static const char *status_msg = NULL;

/* Helper functions */
static bool is_power_button_pressed(void)
{
    return button_status() & BUTTON_POWER;
}

static bool is_usbmode_button_pressed(void)
{
    return button_status() & BUTTON_DOWN;
}

static int send_event_on_tmo(struct timeout *tmo)
{
    button_queue_post(tmo->data, 0);
    return 0;
}

/*
 * Monitors the state of the power button and USB cable
 * insertion status. It will post an event to the main
 * thread when (a) the power button is continously held
 * or released for long enough, or (b) the USB cable is
 * unplugged for long enough.
 */
static void monitor_tick(void)
{
    /* Power button state */
    pwr_prev_state = pwr_curr_state;
    pwr_curr_state = is_power_button_pressed();

    if (pwr_curr_state != pwr_prev_state || restart_pwr_stable_tmo)
    {
        long event = pwr_curr_state ? EV_POWER_PRESSED : EV_POWER_RELEASED;
        int ticks = POWERBUTTON_LONG_PRESS_TIME;

        restart_pwr_stable_tmo = false;
        timeout_register(&pwr_stable_tmo, send_event_on_tmo, ticks, event);
    }

    /* USB cable state */
    usb_prev_state = usb_curr_state;
    usb_curr_state = usb_inserted();

    /* Ignore cable state change in inactive state */
    if (usb_curr_state != usb_prev_state || restart_usb_unplugged_tmo)
    {
        long event = EV_USB_UNPLUGGED;
        int ticks = is_active ? USB_UNPLUGGED_ACTIVE_TIME : USB_UNPLUGGED_INACTIVE_TIME;

        restart_usb_unplugged_tmo = false;

        if (usb_curr_state)
            timeout_cancel(&usb_unplugged_tmo);
        else
            timeout_register(&usb_unplugged_tmo, send_event_on_tmo, ticks, event);
    }
}

static void monitor_init(void)
{
    pwr_curr_state = is_power_button_pressed();
    usb_curr_state = usb_inserted();

    /* Make sure events fire even if inputs don't change after boot */
    restart_pwr_stable_tmo = true;
    restart_usb_unplugged_tmo = true;

    tick_add_task(monitor_tick);
}

static void go_active(void)
{
    is_active = true;
    restart_usb_unplugged_tmo = true;

    gpio_set_level(GPIO_CPU_POWER_ON, 1);
    storage_enable(true);
}

static void go_inactive(void)
{
    is_active = false;
    restart_usb_unplugged_tmo = true;

    storage_enable(false);
    gpio_set_level(GPIO_CPU_POWER_ON, 0);
}

static void refresh_display(void)
{
    if (!is_active)
    {
        lcd_shutdown();
        return;
    }

    int y = 0;

    lcd_clear_display();

    lcd_putsf(0, y++, "Rockbox on %s", MODEL_NAME);
    y++;

    if (status_msg)
    {
        lcd_putsf(0, y++, "Error: %s", status_msg);
        y++;
    }

    if (!usb_inserted())
    {
        lcd_putsf(0, y++, "Hold POWER to power off");
        lcd_putsf(0, y++, "Connect USB cable for USB mode");
        y++;
    }
    else
    {
        lcd_putsf(0, y++, "Bootloader USB mode");

        if (charging_state())
        {
            lcd_putsf(0, y++, "Battery charging (%d mA)",
                      usb_charging_maxcurrent());
        }
        else
        {
            lcd_putsf(0, y++, "Battery charged");
        }

        y++;
    }

    lcd_putsf(0, y++, "Version: %s", RBVERSION);

    lcd_update();
    lcd_enable(true);
}

static void launch(void)
{
    /* No-op if USB mode was requested */
    if (is_usbmode_button_pressed())
        return;

    /* TODO: load rockbox */
    status_msg = "Can't boot RB yet!";
}

void main(void)
{
    system_init();
    kernel_init();
    power_init();
    button_init();

    /* Start monitoring power button / usb state */
    monitor_init();

    /* Prepare LCD in case we need to display something */
    lcd_init();
    backlight_init();

    /*
     * Prepare storage subsystem, but keep SD card unpowered
     * until we actually need to access it.
     */
    storage_init();
    storage_enable(false);

    /*
     * Initialize fs/disk internal state, the disk will not
     * be mountable due to being disabled but this will not
     * cause any fatal errors.
     */
    filesystem_init();
    disk_mount_all();

    if (echoplayer_boot_reason == ECHOPLAYER_BOOT_REASON_SW_REBOOT ||
        usb_detect() == USB_INSERTED)
    {
        /*
         * For software reboot we want to immediately launch Rockbox.
         *
         * We also do so if USB is plugged in at boot, because there's
         * no way to dynamically switch between charge only and mass
         * storage mode depending on the active/inactive state; to avoid
         * confusion, it is simpler to just boot Rockbox.
         */
        go_active();
        launch();
    }
    else if (echoplayer_boot_reason == ECHOPLAYER_BOOT_REASON_SW_POWEROFF)
    {
        /* Ignore power button pressed event until button is first released */
        wait_for_power_released = true;
    }

    /*
     * Initialize USB so we can enumerate with the host and
     * negotiate charge current (needed even in inactive mode)
     */
    usb_init();
    usb_start_monitoring();
    usb_charging_enable(USB_CHARGING_FORCE);

    for (;;)
    {
        refresh_display();

        long refresh_tmo = is_active ? HZ : TIMEOUT_BLOCK;

        switch (button_get_w_tmo(refresh_tmo))
        {
        case EV_POWER_PRESSED:
            if (wait_for_power_released)
                break;

            if (!is_active)
            {
                /* Launch Rockbox due to user pressing power button */
                go_active();
                launch();
            }
            else
            {
                /*
                 * NOTE: The check for USB insertion here is only
                 * because we can't change to charging only mode.
                 */
                if (!usb_inserted())
                    go_inactive();
            }

            break;

        case EV_POWER_RELEASED:
            /*
             * This cuts power if the power button is not
             * held and we're not in the active state due
             * to USB mode, etc.
             */
            wait_for_power_released = false;
            gpio_set_level(GPIO_CPU_POWER_ON, is_active);
            break;

        case EV_USB_UNPLUGGED:
            go_inactive();
            break;

        case SYS_USB_CONNECTED:
            go_active();
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            break;
        }
    }
}
