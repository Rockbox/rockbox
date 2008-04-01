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
#include "hwcompat.h"

static int int_btn = BUTTON_NONE;
#ifdef IPOD_1G2G
/* The 1st Gen wheel draws ~12mA when enabled permanently. Therefore
 * we only enable it for a very short time to check for changes every
 * tick, and only keep it enabled if there is activity. */
#define WHEEL_TIMEOUT (HZ/4)
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
            /* 1st..3rd Gen wheel has inverse direction mapping
             * compared to Mini 1st Gen wheel. */
            switch (direction) {
                case 1:
                    wheel_keycode = BUTTON_SCROLL_BACK;
                    break;
                case -1:
                    wheel_keycode = BUTTON_SCROLL_FWD;
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

static int ipod_3g_button_read(void)
{
    unsigned char source, state;
    static bool was_hold = false;
    int btn = BUTTON_NONE;
    
#ifdef IPOD_3G
    /* The following delay was 250 in the ipodlinux source,
     * but 50 seems to work fine. 250 causes the wheel to stop
     * working when spinning it real fast. */
    udelay(50);
#endif

    /* get source of interupts */
    source = GPIOA_INT_STAT;
    
    /* get current keypad status */
    state = GPIOA_INPUT_VAL;
    
    /* toggle interrupt level */
    GPIOA_INT_LEV = ~state;

#ifdef IPOD_3G
    if (was_hold && source == 0x40 && state == 0xbf) {
        /* ack any active interrupts */
        GPIOA_INT_CLR = source;
        return BUTTON_NONE;
    }
    was_hold = false;

    if ((state & 0x20) == 0) {
        /* 3g hold switch is active low */
        was_hold = true;
        /* hold switch on 3g causes all outputs to go low */
        /* we shouldn't interpret these as key presses */
        GPIOA_INT_CLR = source;
        return BUTTON_NONE;
    }
#elif defined IPOD_1G2G
    if (state & 0x20) {
        /* 1g/2g hold switch is active high */
        GPIOA_INT_CLR = source;
        return BUTTON_NONE;
    }
#endif
    if ((state & 0x1) == 0) {
        btn |= BUTTON_RIGHT;
    }
    if ((state & 0x2) == 0) {
        btn |= BUTTON_SELECT;
    }
    if ((state & 0x4) == 0) {
        btn |= BUTTON_PLAY;
    }
    if ((state & 0x8) == 0) {
        btn |= BUTTON_LEFT;
    }
    if ((state & 0x10) == 0) {
        btn |= BUTTON_MENU;
    }

    if (source & 0xc0) {
        handle_scroll_wheel((state & 0xc0) >> 6, was_hold);
    }

    /* ack any active interrupts */
    GPIOA_INT_CLR = source;

    return btn;
}

void ipod_3g_button_int(void)
{
    CPU_INT_CLR = GPIO_MASK;
    int_btn = ipod_3g_button_read();
    CPU_INT_EN = GPIO_MASK;
}

void button_init_device(void)
{
    GPIOA_ENABLE = 0xff;
    GPIOA_OUTPUT_EN = 0;

    GPIOA_INT_LEV = ~GPIOA_INPUT_VAL;
    GPIOA_INT_CLR = GPIOA_INT_STAT;

#ifdef IPOD_1G2G
    if ((IPOD_HW_REVISION >> 16) == 1)
    {   /* enable scroll wheel */
        GPIOB_ENABLE |= 0x01;
        GPIOB_OUTPUT_EN |= 0x01;
        GPIOB_OUTPUT_VAL |= 0x01;
    }
#endif
    GPIOA_INT_EN  = 0xff;

    CPU_INT_EN = GPIO_MASK;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    static bool hold_button = false;
    bool hold_button_old;
#ifdef IPOD_1G2G
    static int wheel_timeout = 0;
    static unsigned char last_wheel_value = 0;
    unsigned char wheel_value;

    if ((IPOD_HW_REVISION >> 16) == 1)
    {
        if (!hold_button && (wheel_timeout == 0))
        {
            GPIOB_OUTPUT_VAL |= 0x01; /* enable wheel */
            udelay(50);               /* let the voltage settle */
            GPIOA_INT_EN = 0xff;      /* enable wheel interrupts */
        }
        wheel_value = GPIOA_INPUT_VAL >> 6;
        if (wheel_value != last_wheel_value)
        {
            last_wheel_value = wheel_value;
            wheel_timeout = WHEEL_TIMEOUT; /* keep wheel enabled */
        }
        if (wheel_timeout)
            wheel_timeout--;
        else
        {
            GPIOA_INT_EN = 0x3f;       /* disable wheel interrupts */
            GPIOB_OUTPUT_VAL &= ~0x01; /* disable wheel */
        }
    }
#endif

    /* normal buttons */
    hold_button_old = hold_button;
    hold_button = button_hold();

    if (hold_button != hold_button_old)
        backlight_hold_changed(hold_button);

    return int_btn;
}

bool button_hold(void)
{
#ifdef IPOD_1G2G
    return (GPIOA_INPUT_VAL & 0x20);
#else
    return !(GPIOA_INPUT_VAL & 0x20);
#endif
}

bool headphones_inserted(void)
{
    return (GPIOC_INPUT_VAL & 0x1)?true:false;
}
