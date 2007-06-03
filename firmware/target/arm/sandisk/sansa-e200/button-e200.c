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

#define WHEEL_REPEAT_INTERVAL   30
#define WHEEL_FAST_ON_INTERVAL  2
#define WHEEL_FAST_OFF_INTERVAL 6

/* Clickwheel */
#ifndef BOOTLOADER
static unsigned int  old_wheel_value   = 0;
static unsigned int  wheel_repeat      = BUTTON_NONE;
static unsigned int  wheel_click_count = 0;
static          int  wheel_fast_mode   = 0;
static unsigned long last_wheel_tick   = 0;
static unsigned long last_wheel_post   = 0;
static unsigned long next_backlight_on = 0;
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
    last_wheel_tick = current_tick;
    last_wheel_post = current_tick;

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
    CPU_HI_INT_EN = GPIO_MASK;
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
            btn = BUTTON_SCROLL_DOWN;
        else if (old_wheel_value == wheel_tbl[1][wheel_value >> 6])
            btn = BUTTON_SCROLL_UP;

        if (btn != BUTTON_NONE)
        {
            int repeat = 1;

            if (btn != wheel_repeat)
            {
                wheel_repeat      = btn;
                repeat            =
                wheel_fast_mode   =
                wheel_click_count = 0;
            }

            if (wheel_fast_mode)
            {
                if (TIME_AFTER(current_tick,
                    last_wheel_tick + WHEEL_FAST_OFF_INTERVAL))
                {
                    if (++wheel_click_count < 2)
                        btn = BUTTON_NONE;
                    wheel_fast_mode = 0;
                }
            }
            else
            {
                if (repeat && TIME_BEFORE(current_tick,
                    last_wheel_tick + WHEEL_FAST_ON_INTERVAL))
                    wheel_fast_mode = 1;
                else if (++wheel_click_count < 2)
                    btn = BUTTON_NONE;
            }

            if (TIME_AFTER(current_tick, next_backlight_on))
            {
                next_backlight_on = current_tick + HZ/4;
                backlight_on();
                button_backlight_on();
            }

            if (btn != BUTTON_NONE)
            {
                wheel_click_count = 0;

                if (repeat && TIME_BEFORE(current_tick,
                    last_wheel_post + WHEEL_REPEAT_INTERVAL))
                    btn |= BUTTON_REPEAT;

                last_wheel_post = current_tick;

                if (queue_empty(&button_queue))
                    queue_post(&button_queue, btn, 0);
            }

            last_wheel_tick = current_tick;
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
