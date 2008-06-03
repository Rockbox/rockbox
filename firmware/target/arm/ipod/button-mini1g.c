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
 * iPod driver based on code from the ipodlinux project - http://ipodlinux.org
 * Adapted for Rockbox in December 2005
 * Original file: linux/arch/armnommu/mach-ipod/keyboard.c
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
 *
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "cpu.h"
#include "system.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "adc.h"
#include "serial.h"
#include "power.h"
#include "system.h"
#include "powermgmt.h"

/* Variable to use for setting button status in interrupt handler */
int int_btn = BUTTON_NONE;
#ifdef HAVE_WHEEL_POSITION
    static int wheel_position = -1;
    static bool send_events = true;
#endif

static void handle_scroll_wheel(int new_scroll, int was_hold)
{
    int wheel_keycode = BUTTON_NONE;
    static int prev_scroll = -1;
    static int direction = 0;
    static int count = 0;
    static int scroll_state[4][4] = {
        {0, 1, -1, 0},
        {-1, 0, 0, 1},
        {1, 0, 0, -1},
        {0, -1, 1, 0}
    };

    if ( prev_scroll == -1 ) {
        prev_scroll = new_scroll;
    }
    else if (direction != scroll_state[prev_scroll][new_scroll]) {
        direction = scroll_state[prev_scroll][new_scroll];
        count = 0;
    }
    else if (!was_hold) {
        backlight_on();
        reset_poweroff_timer();
        if (++count == 6) { /* reduce sensitivity */
            count = 0;
            /* Mini 1st Gen wheel has inverse direction mapping
             * compared to 1st..3rd Gen wheel. */
            switch (direction) {
                case 1:
                    wheel_keycode = BUTTON_SCROLL_FWD;
                    break;
                case -1:
                    wheel_keycode = BUTTON_SCROLL_BACK;
                    break;
                default:
                    /* only happens if we get out of sync */
                    break;
            }
        }
    }
    if (wheel_keycode != BUTTON_NONE && queue_empty(&button_queue))
        queue_post(&button_queue, wheel_keycode, 0);
    prev_scroll = new_scroll;
}

/* mini 1 only, mini 2G uses iPod 4G code */
static int ipod_mini_button_read(void)
{
    unsigned char source, wheel_source, state, wheel_state;
    static bool was_hold = false;
    int btn = BUTTON_NONE;

    /* The ipodlinux source had a udelay(250) here, but testing has shown that
       it is not needed - tested on mini 1g. */
    /* udelay(250);*/

    /* get source(s) of interupt */
    source = GPIOA_INT_STAT & 0x3f;
    wheel_source = GPIOB_INT_STAT & 0x30;

    if (source == 0 && wheel_source == 0) {
        return BUTTON_NONE; /* not for us */
    }

    /* get current keypad & wheel status */
    state = GPIOA_INPUT_VAL & 0x3f;
    wheel_state = GPIOB_INPUT_VAL & 0x30;

    /* toggle interrupt level */
    GPIOA_INT_LEV = ~state;
    GPIOB_INT_LEV = ~wheel_state;

    /* hold switch causes all outputs to go low    */
    /* we shouldn't interpret these as key presses */
    if ((state & 0x20)) {
        if (!(state & 0x1))
            btn |= BUTTON_SELECT;
        if (!(state & 0x2))
            btn |= BUTTON_MENU;
        if (!(state & 0x4))
            btn |= BUTTON_PLAY;
        if (!(state & 0x8))
            btn |= BUTTON_RIGHT;
        if (!(state & 0x10))
            btn |= BUTTON_LEFT;

        if (wheel_source & 0x30) {
            handle_scroll_wheel((wheel_state & 0x30) >> 4, was_hold);
        }
    }

    was_hold = button_hold();

    /* ack any active interrupts */
    if (source)
        GPIOA_INT_CLR = source;
    if (wheel_source)
        GPIOB_INT_CLR = wheel_source;

    return btn;
}

void ipod_mini_button_int(void)
{
    CPU_HI_INT_DIS = GPIO0_MASK;
    int_btn = ipod_mini_button_read();
    //CPU_INT_EN = 0x40000000;
    CPU_HI_INT_EN = GPIO0_MASK;
}

void button_init_device(void)
{
    /* iPod Mini G1 */
    /* buttons - enable as input */
    GPIOA_ENABLE |= 0x3f;
    GPIOA_OUTPUT_EN &= ~0x3f;
    /* scroll wheel- enable as input */
    GPIOB_ENABLE |= 0x30; /* port b 4,5 */
    GPIOB_OUTPUT_EN &= ~0x30; /* port b 4,5 */
    /* buttons - set interrupt levels */
    GPIOA_INT_LEV = ~(GPIOA_INPUT_VAL & 0x3f);
    GPIOA_INT_CLR = GPIOA_INT_STAT & 0x3f;
    /* scroll wheel - set interrupt levels */
    GPIOB_INT_LEV = ~(GPIOB_INPUT_VAL & 0x30);
    GPIOB_INT_CLR = GPIOB_INT_STAT & 0x30;
    /* enable interrupts */
    GPIOA_INT_EN = 0x3f;
    GPIOB_INT_EN = 0x30;
    /* unmask interrupt */
    CPU_INT_EN = 0x40000000;
    CPU_HI_INT_EN = GPIO0_MASK;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    static bool hold_button = false;
    bool hold_button_old;

    /* normal buttons */
    hold_button_old = hold_button;
    hold_button = button_hold();

    if (hold_button != hold_button_old)
        backlight_hold_changed(hold_button);

    /* The int_btn variable is set in the button interrupt handler */
    return int_btn;
}

bool button_hold(void)
{
    return (GPIOA_INPUT_VAL & 0x20)?false:true;
}

bool headphones_inserted(void)
{
    return (GPIOA_INPUT_VAL & 0x80)?true:false;
}
