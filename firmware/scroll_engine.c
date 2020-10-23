/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Michael Sevakis
 *
 * LCD scrolling thread and scheduler
 *
 * Much collected and combined from the various Rockbox LCD drivers.
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

#include <stdio.h>
#include "config.h"
#include "gcc_extensions.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "usb.h"
#include "lcd.h"
#include "font.h"
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#include "scroll_engine.h"


/* private helper function for the scroll engine. Do not use in apps/.
 * defined in lcd-bitmap-common.c */
extern struct viewport *lcd_get_viewport(bool *is_defaut);
#ifdef HAVE_REMOTE_LCD
extern struct viewport *lcd_remote_get_viewport(bool *is_defaut);
#endif

static const char scroll_tick_table[18] = {
 /* Hz values [f(x)=100.8/(x+.048)]:
    1, 1.25, 1.55, 2, 2.5, 3.12, 4, 5, 6.25, 8.33, 10, 12.5, 16.7, 20, 25, 33, 49.2, 96.2 */
    100, 80, 64, 50, 40, 32, 25, 20, 16, 12, 10, 8, 6, 5, 4, 3, 2, 1
};

static void scroll_thread(void);
static const char scroll_name[] = "scroll";

#include "drivers/lcd-scroll.c"

#ifdef HAVE_REMOTE_LCD
static struct event_queue scroll_queue SHAREDBSS_ATTR;

/* copied from lcd-remote-1bit.c */
/* Compile 1 bit vertical packing LCD driver for remote LCD */
#undef LCDFN
#define LCDFN(fn) lcd_remote_ ## fn
#undef LCDM
#define LCDM(ma) LCD_REMOTE_ ## ma

#include "drivers/lcd-scroll.c"

static void sync_display_ticks(void)
{
    lcd_scroll_info.last_scroll =
    lcd_remote_scroll_info.last_scroll = current_tick;
}

static bool scroll_process_message(int delay)
{
    struct queue_event ev;

    /* just poll once for negative delays */
    if (delay < 0)
        delay = TIMEOUT_NOBLOCK;

    do
    {
        long tick = current_tick;
        queue_wait_w_tmo(&scroll_queue, &ev, delay);

        switch (ev.id)
        {
        case SYS_TIMEOUT:
            return false;
        case SYS_USB_CONNECTED:
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            usb_wait_for_disconnect(&scroll_queue);
            sync_display_ticks();
            return true;
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
        case SYS_REMOTE_PLUGGED:
            if (!remote_initialized)
                sync_display_ticks();
#endif
        }

        delay -= current_tick - tick;
    }
    while (delay > 0);

    return false;
}
#endif /* HAVE_REMOTE_LCD */

static void scroll_thread(void) NORETURN_ATTR;
#ifdef HAVE_REMOTE_LCD

static void scroll_thread(void)
{
    enum
    {
        SCROLL_LCD        = 0x1,
        SCROLL_LCD_REMOTE = 0x2,
    };

    sync_display_ticks();

    while ( 1 )
    {
        long delay;
        int scroll;
        long tick_lcd, tick_remote;

        tick_lcd = lcd_scroll_info.last_scroll + lcd_scroll_info.ticks;
        delay = current_tick;

        if (
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
            !remote_initialized ||
#endif
            (tick_remote = lcd_remote_scroll_info.last_scroll +
                           lcd_remote_scroll_info.ticks,
             TIME_BEFORE(tick_lcd, tick_remote)))
        {
            scroll = SCROLL_LCD;
            delay = tick_lcd - delay;
        }
        /* TIME_BEFORE(tick_remote, tick_lcd) */
        else if (tick_lcd != tick_remote)
        {
            scroll = SCROLL_LCD_REMOTE;
            delay = tick_remote - delay;
        }
        else
        {
            scroll = SCROLL_LCD | SCROLL_LCD_REMOTE;
            delay = tick_lcd - delay;
        }

        if (scroll_process_message(delay))
            continue;

        if (scroll & SCROLL_LCD)
        {
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
            if (lcd_active())
#endif
                lcd_scroll_worker();
            lcd_scroll_info.last_scroll = current_tick;
        }

        if (scroll == (SCROLL_LCD | SCROLL_LCD_REMOTE))
            yield();

        if (scroll & SCROLL_LCD_REMOTE)
        {
            lcd_remote_scroll_worker();
            lcd_remote_scroll_info.last_scroll = current_tick;
        }
    }
}
#else
static void scroll_thread(void)
{
    while (1)
    {
        sleep(lcd_scroll_info.ticks);
#if !defined(BOOTLOADER)
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
        if (lcd_active())
#endif
            lcd_scroll_worker();
#endif /* !BOOTLOADER */
    }
}
#endif /* HAVE_REMOTE_LCD */


#ifndef BOOTLOADER
void scroll_init(void)
{
    static char scroll_stack[DEFAULT_STACK_SIZE*3];
#ifdef HAVE_REMOTE_LCD
    queue_init(&scroll_queue, true);
#endif
    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), 0, scroll_name
                  IF_PRIO(, PRIORITY_USER_INTERFACE)
                  IF_COP(, CPU));
}
#else
void scroll_init(void)
{
    /* DUMMY */
}
#endif /* ndef BOOTLOADER*/
