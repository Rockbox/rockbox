/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* Taken from button-h10.c by Barry Wardell and reverse engineering by MrH. */

#include "system.h"
#include "button.h"
#include "backlight.h"
#include "powermgmt.h"

#define WHEEL_REPEAT_INTERVAL   300000
#define WHEEL_FAST_ON_INTERVAL   20000
#define WHEEL_FAST_OFF_INTERVAL  60000
#define WHEELCLICKS_PER_ROTATION    48 /* wheelclicks per full rotation */

/* Clickwheel */
#ifndef BOOTLOADER
static unsigned int  old_wheel_value   = 0;
static unsigned int  wheel_repeat      = BUTTON_NONE;
static unsigned int  wheel_click_count = 0;
static unsigned int  wheel_delta       = 0;
static          int  wheel_fast_mode   = 0;
static unsigned long last_wheel_usec   = 0;
static unsigned long wheel_velocity    = 0;
static          long last_wheel_post   = 0;
static          long next_backlight_on = 0;
/* Buttons */
static bool hold_button     = false;
static bool hold_button_old = false;
#define _button_hold() hold_button
#else
#define _button_hold() ((GPIOF_INPUT_VAL & 0x80) != 0)
#endif /* BOOTLOADER */
static int  int_btn         = BUTTON_NONE;

void button_int(void);

void button_init_device(void)
{
    /* Enable all buttons */
    GPIOF_OUTPUT_EN &= ~0xff;
    GPIOF_ENABLE |= 0xff;

    /* Scrollwheel light - enable control through GPIOG pin 7 and set timeout */
    GPIOG_OUTPUT_EN |= 0x80;
    GPIOG_ENABLE = 0x80;

#ifndef BOOTLOADER
    /* Mask these before performing init ... because init has possibly
       occurred before */
    GPIOF_INT_EN &= ~0xff;
    GPIOH_INT_EN &= ~0xc0;

    /* Get current tick before enabling button interrupts */
    last_wheel_usec = USEC_TIMER;
    last_wheel_post = last_wheel_usec;

    GPIOH_ENABLE |= 0xc0;
    GPIOH_OUTPUT_EN &= ~0xc0;

    /* Read initial buttons */
    button_int();
    GPIOF_INT_CLR = 0xff;

    /* Read initial wheel value (bit 6-7 of GPIOH) */
    old_wheel_value = GPIOH_INPUT_VAL & 0xc0;
    GPIOH_INT_LEV = (GPIOH_INT_LEV & ~0xc0) | (old_wheel_value ^ 0xc0);
    GPIOH_INT_CLR = 0xc0;

    /* Enable button interrupts */
    GPIOF_INT_EN |= 0xff;
    GPIOH_INT_EN |= 0xc0;

    CPU_INT_EN = HI_MASK;
    CPU_HI_INT_EN = GPIO1_MASK;
#endif /* BOOTLOADER */
}

bool button_hold(void)
{
    return _button_hold();
}

/* clickwheel */
#ifndef BOOTLOADER
void clickwheel_int(void)
{
    /* Read wheel 
     * Bits 6 and 7 of GPIOH change as follows:
     * Clockwise rotation   01 -> 00 -> 10 -> 11
     * Counter-clockwise    11 -> 10 -> 00 -> 01
     *
     * This is equivalent to wheel_value of:
     * Clockwise rotation   0x40 -> 0x00 -> 0x80 -> 0xc0
     * Counter-clockwise    0xc0 -> 0x80 -> 0x00 -> 0x40
     */
    static const unsigned char wheel_tbl[2][4] =
    {
       /* 0x00  0x40  0x80  0xc0 */ /* Wheel value        */
        { 0x40, 0xc0, 0x00, 0x80 }, /* Clockwise rotation */
        { 0x80, 0x00, 0xc0, 0x40 }, /* Counter-clockwise  */ 
    };

    unsigned int wheel_value;

    wheel_value = GPIOH_INPUT_VAL & 0xc0;
    GPIOH_INT_LEV = (GPIOH_INT_LEV & ~0xc0) | (wheel_value ^ 0xc0);
    GPIOH_INT_CLR = GPIOH_INT_STAT & 0xc0;

    if (!hold_button)
    {
        unsigned int btn = BUTTON_NONE;

        if (old_wheel_value == wheel_tbl[0][wheel_value >> 6])
            btn = BUTTON_SCROLL_FWD;
        else if (old_wheel_value == wheel_tbl[1][wheel_value >> 6])
            btn = BUTTON_SCROLL_BACK;

        if (btn != BUTTON_NONE)
        {
            int repeat = 1; /* assume repeat */
            unsigned long usec = USEC_TIMER;
            unsigned v = (usec - last_wheel_usec) & 0x7fffffff;

            v = (v>0) ? 1000000 / v : 0;     /* clicks/sec = 1000000 * clicks/usec */
            v = (v>0xffffff) ? 0xffffff : v; /* limit to 24 bit */

            /* some velocity filtering to smooth things out */
            wheel_velocity = (7*wheel_velocity + v) / 8;

            if (btn != wheel_repeat)
            {
                /* direction reversals nullify all fast mode states */
                wheel_repeat      = btn;
                repeat            =
                wheel_fast_mode   =
                wheel_velocity    =
                wheel_click_count = 0;
            }

            if (wheel_fast_mode != 0)
            {
                /* fast OFF happens immediately when velocity drops below
                   threshold */
                if (TIME_AFTER(usec,
                        last_wheel_usec + WHEEL_FAST_OFF_INTERVAL))
                {
                    /* moving out of fast mode */
                    wheel_fast_mode = 0;
                    /* reset velocity */
                    wheel_velocity = 0;
                    /* wheel_delta is always 1 in slow mode */
                    wheel_delta = 1;
                }
            }
            else
            {
                /* fast ON gets filtered to avoid inadvertent jumps to fast mode */
                if (repeat && wheel_velocity > 1000000/WHEEL_FAST_ON_INTERVAL)
                {
                    /* moving into fast mode */
                    wheel_fast_mode = 1 << 31;
                    wheel_click_count = 0;
                    wheel_velocity = 1000000/WHEEL_FAST_OFF_INTERVAL;
                }
                else if (++wheel_click_count < 2)
                {
                    btn = BUTTON_NONE;
                }

                /* wheel_delta is always 1 in slow mode */
                wheel_delta = 1;
            }

            if (TIME_AFTER(current_tick, next_backlight_on) ||
                v <= 4)
            {
                /* poke backlight to turn it on or maintain it no more often
                   than every 1/4 second*/
                next_backlight_on = current_tick + HZ/4;
                backlight_on();
                buttonlight_on();
                reset_poweroff_timer();
            }

            if (btn != BUTTON_NONE)
            {
                wheel_click_count = 0;

                /* generate repeats if quick enough */
                if (repeat && TIME_BEFORE(usec,
                        last_wheel_post + WHEEL_REPEAT_INTERVAL))
                    btn |= BUTTON_REPEAT;

                last_wheel_post = usec;

                if (queue_empty(&button_queue))
                {
                    queue_post(&button_queue, btn, wheel_fast_mode |
                               (wheel_delta << 24) | wheel_velocity*360/WHEELCLICKS_PER_ROTATION);
                    /* message posted - reset delta */
                    wheel_delta = 1;
                }
                else
                {
                    /* skipped post - increment delta */
                    if (++wheel_delta > 0x7f)
                        wheel_delta = 0x7f;
                }
            }

            last_wheel_usec = usec;
        }
    }

    old_wheel_value = wheel_value;
}
#endif /* BOOTLOADER */

/* device buttons */
void button_int(void)
{
    unsigned char state;

    int_btn = BUTTON_NONE;

    state = GPIOF_INPUT_VAL & 0xff;

#ifndef BOOTLOADER
    GPIOF_INT_LEV = (GPIOF_INT_LEV & ~0xff) | (state ^ 0xff);
    GPIOF_INT_CLR = GPIOF_INT_STAT;

    hold_button = (state & 0x80) != 0;
#endif

    if (!_button_hold())
    {
        /* Read normal buttons */
        if ((state & 0x01) == 0) int_btn |= BUTTON_REC;
        if ((state & 0x02) == 0) int_btn |= BUTTON_DOWN;
        if ((state & 0x04) == 0) int_btn |= BUTTON_RIGHT;
        if ((state & 0x08) == 0) int_btn |= BUTTON_LEFT;
        if ((state & 0x10) == 0) int_btn |= BUTTON_SELECT; /* The centre button */
        if ((state & 0x20) == 0) int_btn |= BUTTON_UP; /* The "play" button */
        if ((state & 0x40) != 0) int_btn |= BUTTON_POWER;
    }
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
#ifdef BOOTLOADER
    /* Read buttons directly in the bootloader */
    button_int();
#else
    /* light handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
#endif /* BOOTLOADER */

    /* The int_btn variable is set in the button interrupt handler */
    return int_btn;
}
