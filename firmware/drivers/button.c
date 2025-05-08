/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
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

/*
 * Rockbox button functions
 */

#include <stdlib.h>
#include "config.h"
#include "system.h"
#include "button.h"
#include "kernel.h"
#include "thread.h"
#include "backlight.h"
#include "serial.h"
#include "power.h"
#include "powermgmt.h"
#ifdef HAVE_SDL
#include "button-sdl.h"
#else
#include "button-target.h"
#endif
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#if defined(HAVE_TRANSFLECTIVE_LCD) && defined(HAVE_LCD_SLEEP)
#include "lcd.h"       /* lcd_active() prototype */
#endif

static long lastbtn;   /* Last valid button status */
static long last_read; /* Last button status, for debouncing/filtering */
static bool flipped;  /* buttons can be flipped to match the LCD flip */

#ifdef HAVE_BACKLIGHT /* Filter first keypress function pointer */
static bool (*keypress_filter_fn)(int, int);
#ifdef HAVE_REMOTE_LCD
static bool (*remote_keypress_filter_fn)(int, int);
#endif
#endif /* HAVE_BACKLIGHT */

#ifdef HAVE_SW_POWEROFF
static bool enable_sw_poweroff = true;
#endif

/* how long until repeat kicks in, in centiseconds */
#define REPEAT_START      (30*HZ/100)

/* The next two make repeat "accelerate", which is nice for lists
 * which begin to scroll a bit faster when holding until the
 * real list acceleration kicks in (this smooths acceleration).
 *
 * Note that touchscreen pointing events are not subject to this
 * acceleration and always use REPEAT_INTERVAL_TOUCH. (Do repeat
 * events even do anything sane for touchscreens??)
 */

/* the speed repeat starts at, in centiseconds */
#define REPEAT_INTERVAL_START   (16*HZ/100)
/* speed repeat finishes at, in centiseconds */
#define REPEAT_INTERVAL_FINISH  (5*HZ/100)
/* repeat interval for touch events */
#define REPEAT_INTERVAL_TOUCH   (5*HZ/100)

static int lastdata = 0;
static int button_read(int *data);

#ifdef HAVE_TOUCHSCREEN
static long last_touchscreen_touch;
#endif

static void button_remote_post(void)
{
#if defined(HAS_SERIAL_REMOTE) && !defined(SIMULATOR)
    /* Post events for the remote control */
    int btn = remote_control_rx();
    if(btn)
        button_queue_try_post(btn, 0);
#endif
}

#if defined(HAVE_HEADPHONE_DETECTION)
static int hp_detect_callback(struct timeout *tmo)
{
    /* Try to post only transistions */
    const long id = tmo->data ? SYS_PHONE_PLUGGED : SYS_PHONE_UNPLUGGED;
    button_queue_post_remove_head(id, 0);
    return 0;
    /*misc.c:hp_unplug_change*/
}
#endif

#if defined(HAVE_LINEOUT_DETECTION)
static int lo_detect_callback(struct timeout *tmo)
{
    /* Try to post only transistions */
    const long id = tmo->data ? SYS_LINEOUT_PLUGGED : SYS_LINEOUT_UNPLUGGED;
    button_queue_post_remove_head(id, 0);
    return 0;
    /*misc.c:lo_unplug_change*/
}
#endif

static void check_audio_peripheral_state(void)
{
#if defined(HAVE_HEADPHONE_DETECTION)
    static struct timeout hp_detect_timeout; /* Debouncer for headphone plug/unplug */
    static bool phones_present = false;

    if (headphones_inserted() != phones_present)
    {
        /* Use the autoresetting oneshot to debounce the detection signal */
        phones_present = !phones_present;
        timeout_register(&hp_detect_timeout, hp_detect_callback,
                         HZ/2, phones_present);
    }
#endif
#if defined(HAVE_LINEOUT_DETECTION)
    static struct timeout lo_detect_timeout; /* Debouncer for lineout plug/unplug */
    static bool lineout_present = false;

    if (lineout_inserted() != lineout_present)
    {
        /* Use the autoresetting oneshot to debounce the detection signal */
        lineout_present = !lineout_present;
        timeout_register(&lo_detect_timeout, lo_detect_callback,
                         HZ/2, lineout_present);
    }
#endif
}

#ifdef HAVE_BACKLIGHT
/* disabled function is shared between Main & Remote LCDs */
static bool filter_first_keypress_disabled(int button, int data)
{
    button_queue_try_post(button, data);
    return false;
}

static bool filter_first_keypress_enabled(int button, int data)
{
#if defined(HAVE_TRANSFLECTIVE_LCD) && defined(HAVE_LCD_SLEEP)
    if (is_backlight_on(false) && lcd_active())
#else
    if (is_backlight_on(false))
#endif
    {
        return filter_first_keypress_disabled(button, data);
    }
    return true;
}

#ifdef HAVE_REMOTE_LCD
static bool filter_first_remote_keypress_enabled(int button, int data)
{
    if (is_remote_backlight_on(false)
#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
        || (remote_type()==REMOTETYPE_H300_NONLCD)
#endif
    )
    {
        return filter_first_keypress_disabled(button, data);
    }
    return true;
}
#endif /* def HAVE_REMOTE_LCD */
#endif /* def HAVE_BACKLIGHT */

static void button_tick(void)
{
    static int count = 0;
    static int repeat_speed = REPEAT_INTERVAL_START;
    static int repeat_count = 0;
    static bool repeat = false;
    static bool post = false;
#ifdef HAVE_BACKLIGHT
    static bool skip_release = false;
#ifdef HAVE_REMOTE_LCD
    static bool skip_remote_release = false;
#endif
#endif
    int diff;
    int btn;
    int data = 0;

    button_remote_post();

    btn = button_read(&data);

    check_audio_peripheral_state();

    /* Find out if a key has been released */
    diff = btn ^ lastbtn;
    if(diff && (btn & diff) == 0)
    {
#ifdef HAVE_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
        if(diff & BUTTON_REMOTE)
            if(!skip_remote_release)
                button_queue_try_post(BUTTON_REL | diff, data);
            else
                skip_remote_release = false;
        else
#endif
            if(!skip_release)
                button_queue_try_post(BUTTON_REL | diff, data);
            else
                skip_release = false;
#else
        button_queue_try_post(BUTTON_REL | diff, data);
#endif
    }
    else
    {
        if ( btn )
        {
            /* normal keypress */
            if ( btn != lastbtn )
            {
                post = true;
                repeat = false;
                repeat_speed = REPEAT_INTERVAL_START;
            }
            else /* repeat? */
            {

#if defined(DX50) || defined(DX90)
                /*
                    Power button on these devices reports two distinct key codes, which are
                    triggerd by a short or medium duration press. Additionlly a long duration press
                    will trigger a hard reset, which is hardwired.

                    The time delta between medium and long duration press is not large enough to
                    register here as power off repeat. A hard reset is triggered before Rockbox
                    can power off.

                    To cirumvent the hard reset, Rockbox will shutdown on the first POWEROFF_BUTTON
                    repeat. POWEROFF_BUTTON is associated with the a medium duration press of the
                    power button.
                */
                if(btn & POWEROFF_BUTTON)
                {
                    sys_poweroff();
                }
#endif

                if ( repeat )
                {
                    if (!post)
                        count--;
                    if (count == 0) {
                        post = true;
                        /* yes we have repeat */
                        if (repeat_speed > REPEAT_INTERVAL_FINISH)
                            repeat_speed--;
#ifdef HAVE_TOUCHSCREEN
                        if(btn & BUTTON_TOUCHSCREEN)
                            repeat_speed = REPEAT_INTERVAL_TOUCH;
#endif

                        count = repeat_speed;

                        repeat_count++;

                        /* Send a SYS_POWEROFF event if we have a device
                           which doesn't shut down easily with the OFF
                           key */
#ifdef HAVE_SW_POWEROFF
                        if (enable_sw_poweroff &&
                            (btn & POWEROFF_BUTTON
#ifdef RC_POWEROFF_BUTTON
                                    || btn == RC_POWEROFF_BUTTON
#endif
                                    ) &&
#if CONFIG_CHARGING && !defined(HAVE_POWEROFF_WHILE_CHARGING)
                                !charger_inserted() &&
#endif
                                repeat_count > POWEROFF_COUNT)
                        {
                            /* Tell the main thread that it's time to
                               power off */
                            sys_poweroff();

                            /* Safety net for players without hardware
                               poweroff */
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
                            if(repeat_count > POWEROFF_COUNT * 10)
                                power_off();
#endif
                        }
#endif
                    }
                }
                else
                {
                    if (count++ > REPEAT_START)
                    {
                        post = true;
                        repeat = true;
                        repeat_count = 0;
                        /* initial repeat */
                        count = REPEAT_INTERVAL_START;
                    }
#ifdef HAVE_TOUCHSCREEN
                    else if (lastdata != data && btn == lastbtn)
                    {   /* only coordinates changed, post anyway */
                        if (touchscreen_get_mode() == TOUCHSCREEN_POINT)
                            post = true;
                    }
#endif
                }
            }
            if ( post )
            {
                if (repeat)
                {
                    /* Only post repeat events if the queue is empty,
                     * to avoid afterscroll effects. */
                    if (button_queue_try_post(BUTTON_REPEAT | btn, data))
                    {
#ifdef HAVE_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
                        skip_remote_release = false;
#endif
                        skip_release = false;
#endif
                        post = false;
                        /* Need to post back/buttonlight_on() on repeat buttons */
#ifdef HAVE_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
                        if (btn & BUTTON_REMOTE) {
                            remote_backlight_on();
                        }
                        else
#endif
                        {
                            backlight_on();
                            buttonlight_on();
                        }
#endif
                    }
                }
                else
                {
#ifdef HAVE_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
                    if (btn & BUTTON_REMOTE) {
                        skip_remote_release = remote_keypress_filter_fn(btn, data);
                        remote_backlight_on();
                    }
                    else
#endif
                    {
                        skip_release = keypress_filter_fn(btn, data);
                        backlight_on();
                        buttonlight_on();
                    }
#else /* no backlight, nothing to skip */
                    button_queue_try_post(btn, data);
#endif
                    post = false;
                }
                reset_poweroff_timer();
            }
        }
        else
        {
            repeat = false;
            count = 0;
        }
    }
    lastbtn = btn & ~(BUTTON_REL | BUTTON_REPEAT);

    lastdata = data;
}

void button_init(void)
{
    int temp;
    /* Init used objects first */
    button_queue_init();

    /* hardware inits */
    button_init_device();

    button_read(&temp);
    lastbtn = button_read(&temp);

    reset_poweroff_timer();

    flipped = false;
#ifdef HAVE_BACKLIGHT
    set_backlight_filter_keypress(false);
#ifdef HAVE_REMOTE_LCD
    set_remote_backlight_filter_keypress(false);
#endif
#endif
#ifdef HAVE_TOUCHSCREEN
    last_touchscreen_touch = -1;
#endif
    /* Start polling last */
    tick_add_task(button_tick);
}

#ifdef BUTTON_DRIVER_CLOSE
void button_close(void)
{
    tick_remove_task(button_tick);
}
#endif /* BUTTON_DRIVER_CLOSE */

#ifdef HAVE_LCD_FLIP
/*
 * helper function to swap LEFT/RIGHT, UP/DOWN (if present)
 */
static int button_flip(int button)
{
    int newbutton = button;

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    newbutton &= ~(
#if defined(BUTTON_LEFT) && defined(BUTTON_RIGHT)
        BUTTON_LEFT | BUTTON_RIGHT
#else
#warning "LEFT/RIGHT not defined!"
	 0
#endif
#if defined(BUTTON_UP) && defined(BUTTON_DOWN)
        | BUTTON_UP | BUTTON_DOWN
#endif
#if defined(BUTTON_SCROLL_BACK) && defined(BUTTON_SCROLL_FWD)
        | BUTTON_SCROLL_BACK | BUTTON_SCROLL_FWD
#endif
#if (CONFIG_KEYPAD == SANSA_C200_PAD) || (CONFIG_KEYPAD == SANSA_CLIP_PAD) ||\
    (CONFIG_KEYPAD == GIGABEAT_PAD) || (CONFIG_KEYPAD == GIGABEAT_S_PAD)
        | BUTTON_VOL_UP | BUTTON_VOL_DOWN
#endif
#if CONFIG_KEYPAD == PHILIPS_SA9200_PAD
        | BUTTON_VOL_UP | BUTTON_VOL_DOWN
        | BUTTON_NEXT | BUTTON_PREV
#endif
        );

#if defined(BUTTON_LEFT) && defined(BUTTON_RIGHT)
    if (button & BUTTON_LEFT)
        newbutton |= BUTTON_RIGHT;
    if (button & BUTTON_RIGHT)
        newbutton |= BUTTON_LEFT;
#else
#warning "LEFT/RIGHT not defined!"
#endif

#if defined(BUTTON_UP) && defined(BUTTON_DOWN)
    if (button & BUTTON_UP)
        newbutton |= BUTTON_DOWN;
    if (button & BUTTON_DOWN)
        newbutton |= BUTTON_UP;
#endif
#if defined(BUTTON_SCROLL_BACK) && defined(BUTTON_SCROLL_FWD)
    if (button & BUTTON_SCROLL_BACK)
        newbutton |= BUTTON_SCROLL_FWD;
    if (button & BUTTON_SCROLL_FWD)
        newbutton |= BUTTON_SCROLL_BACK;
#endif
#if (CONFIG_KEYPAD == SANSA_C200_PAD) || (CONFIG_KEYPAD == SANSA_CLIP_PAD) ||\
    (CONFIG_KEYPAD == GIGABEAT_PAD) || (CONFIG_KEYPAD == GIGABEAT_S_PAD)
    if (button & BUTTON_VOL_UP)
        newbutton |= BUTTON_VOL_DOWN;
    if (button & BUTTON_VOL_DOWN)
        newbutton |= BUTTON_VOL_UP;
#endif
#if CONFIG_KEYPAD == PHILIPS_SA9200_PAD
    if (button & BUTTON_VOL_UP)
        newbutton |= BUTTON_VOL_DOWN;
    if (button & BUTTON_VOL_DOWN)
        newbutton |= BUTTON_VOL_UP;
    if (button & BUTTON_NEXT)
        newbutton |= BUTTON_PREV;
    if (button & BUTTON_PREV)
        newbutton |= BUTTON_NEXT;
#endif
#endif /* !SIMULATOR */
    return newbutton;
}

/*
 * set the flip attribute
 * better only call this when the queue is empty
 */
void button_set_flip(bool flip)
{
    if (flip != flipped) /* not the current setting */
    {
        /* avoid race condition with the button_tick() */
        int oldlevel = disable_irq_save();
        lastbtn = button_flip(lastbtn);
        flipped = flip;
        restore_irq(oldlevel);
    }
}
#endif /* HAVE_LCD_FLIP */

#ifdef HAVE_BACKLIGHT
void set_backlight_filter_keypress(bool value)
{
    if (!value)
        keypress_filter_fn = filter_first_keypress_disabled;
    else
        keypress_filter_fn = filter_first_keypress_enabled;
}
#ifdef HAVE_REMOTE_LCD
void set_remote_backlight_filter_keypress(bool value)
{
    if (!value)
        remote_keypress_filter_fn = filter_first_keypress_disabled;
    else
        remote_keypress_filter_fn = filter_first_remote_keypress_enabled;
}
#endif
#endif

/*
 * Get button pressed from hardware
 */

static int button_read(int *data)
{
#ifdef HAVE_BUTTON_DATA
    int btn = button_read_device(data);
#else
    (void) data;
    int btn = button_read_device();
#endif
    int retval;

#ifdef HAVE_LCD_FLIP
    if (btn && flipped)
        btn = button_flip(btn); /* swap upside down */
#endif /* HAVE_LCD_FLIP */

#ifdef HAVE_TOUCHSCREEN
    if (btn & BUTTON_TOUCHSCREEN)
        last_touchscreen_touch = current_tick;
#endif
    /* Filter the button status. It is only accepted if we get the same
       status twice in a row. */
#ifndef HAVE_TOUCHSCREEN
    if (btn != last_read)
        retval = lastbtn;
    else
#endif
        retval = btn;
    last_read = btn;

    return retval;
}

int button_status(void)
{
    return lastbtn;
}

#ifdef HAVE_BUTTON_DATA
int button_status_wdata(int *pdata)
{
    *pdata = lastdata;
    return lastbtn;
}
#endif

#ifdef HAVE_TOUCHSCREEN
long touchscreen_last_touch(void)
{
    return last_touchscreen_touch;
}
#endif

#ifdef HAVE_WHEEL_ACCELERATION
/* WHEEL_ACCEL_FACTOR = 2^16 / WHEEL_ACCEL_START */
#define WHEEL_ACCEL_FACTOR (1<<16)/WHEEL_ACCEL_START
/**
 * data:
 *    [31] Use acceleration
 * [30:24] Message post count (skipped + 1) (1-127)
 *  [23:0] Velocity - degree/sec
 *
 * WHEEL_ACCEL_FACTOR:
 * Value in degree/sec -- configurable via settings -- above which
 * the accelerated scrolling starts. Factor is internally scaled by
 * 1<<16 in respect to the following 32bit integer operations.
 */
int button_apply_acceleration(const unsigned int data)
{
    int delta = (data >> 24) & 0x7f;

    if ((data & (1 << 31)) != 0)
    {
        /* read driver's velocity from data */
        unsigned int v = data & 0xffffff;

        /* v = 28.4 fixed point */
        v = (WHEEL_ACCEL_FACTOR * v)>>(16-4);

        /* Calculate real numbers item to scroll based upon acceleration
         * setting, use correct roundoff */
#if   (WHEEL_ACCELERATION == 1)
        v = (v*v     + (1<< 7))>> 8;
#elif (WHEEL_ACCELERATION == 2)
        v = (v*v*v   + (1<<11))>>12;
#elif (WHEEL_ACCELERATION == 3)
        v = (v*v*v*v + (1<<15))>>16;
#endif

        if (v > 1)
            delta *= v;
    }

    return delta;
}
#endif /* HAVE_WHEEL_ACCELERATION */

#if (defined(HAVE_TOUCHPAD) || defined(HAVE_TOUCHSCREEN)) && !defined(HAS_BUTTON_HOLD)
void button_enable_touch(bool en)
{
#ifdef HAVE_TOUCHPAD
    touchpad_enable(en);
#endif
#ifdef HAVE_TOUCHSCREEN
    touchscreen_enable(en);
#endif
}
#endif

#ifdef HAVE_SW_POWEROFF
void button_set_sw_poweroff_state(bool en) {
    enable_sw_poweroff = en;
}

bool button_get_sw_poweroff_state() {
    return enable_sw_poweroff;
}
#endif
