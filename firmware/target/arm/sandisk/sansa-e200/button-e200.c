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

#include <stdlib.h>
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "system.h"

static unsigned int old_wheel_value = 0;
static unsigned int wheel_repeat = BUTTON_NONE;

/* Wheel backlight control */
#define WHEEL_BACKLIGHT_TIMEOUT 5*HZ;
static unsigned int wheel_backlight_timer;

void wheel_backlight_on(bool enable)
{
    if(enable)
        GPIOG_OUTPUT_VAL |=0x80;
    else
        GPIOG_OUTPUT_VAL &=~ 0x80;
}

void button_init_device(void)
{
    /* Enable all buttons */
    GPIOF_ENABLE |= 0xff;
    GPIOH_ENABLE |= 0xc0;
    
    /* Scrollwheel light - enable control through GPIOG pin 7 and set timeout */
    GPIOG_ENABLE = 0x80;
    GPIOG_OUTPUT_EN |= 0x80;
    wheel_backlight_timer = WHEEL_BACKLIGHT_TIMEOUT;
    
    /* Read initial wheel value (bit 6-7 of GPIOH) */
    old_wheel_value = GPIOH_INPUT_VAL & 0xc0;
}

bool button_hold(void)
{
    return (GPIOF_INPUT_VAL & 0x80)?true:false;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    unsigned char state;
    static bool hold_button = false;
    bool hold_button_old;
    unsigned int new_wheel_value = 0; /* read later, but this stops a warning */

    /* Hold */
    hold_button_old = hold_button;
    hold_button = button_hold();

#if 0
#ifndef BOOTLOADER
    /* light handling */
    if (hold_button != hold_button_old)
    {
        backlight_hold_changed(hold_button);
    }
#endif
#endif

    /* device buttons */
    if (!hold_button)
    {
        /* Read normal buttons */
        state = GPIOF_INPUT_VAL & 0xff;
        if ((state & 0x1) == 0) btn |= BUTTON_REC;
        if ((state & 0x2) == 0) btn |= BUTTON_DOWN;
        if ((state & 0x4) == 0) btn |= BUTTON_RIGHT;
        if ((state & 0x8) == 0) btn |= BUTTON_LEFT;
        if ((state & 0x10) == 0) btn |= BUTTON_SELECT; /* The centre button */
        if ((state & 0x20) == 0) btn |= BUTTON_UP; /* The "play" button */
        if ((state & 0x40) != 0) btn |= BUTTON_POWER;
        
        /* Read wheel 
         * Bits 6 and 7 of GPIOH change as follows:
         * Clockwise rotation   01 -> 00 -> 10 -> 11
         * Counter-clockwise    11 -> 10 -> 00 -> 01
         *
         * This is equivalent to wheel_value of:
         * Clockwise rotation   0x40 -> 0x00 -> 0x80 -> 0xc0
         * Counter-clockwise    0xc0 -> 0x80 -> 0x00 -> 0x40
         */
        new_wheel_value = GPIOH_INPUT_VAL & 0xc0;
        switch(new_wheel_value){
        case 0x00:
            if(old_wheel_value==0x80)
                btn |= BUTTON_SCROLL_UP;
            else if (old_wheel_value==0x40)
                btn |= BUTTON_SCROLL_DOWN;
            break;
        case 0x40:
            if(old_wheel_value==0x00)
                btn |= BUTTON_SCROLL_UP;
            else if (old_wheel_value==0xc0)
                btn |= BUTTON_SCROLL_DOWN;
            break;
        case 0x80:
            if(old_wheel_value==0xc0)
                btn |= BUTTON_SCROLL_UP;
            else if (old_wheel_value==0x00)
                btn |= BUTTON_SCROLL_DOWN;
            break;
        case 0xc0:
            if(old_wheel_value==0x40)
                btn |= BUTTON_SCROLL_UP;
            else if (old_wheel_value==0x80)
                btn |= BUTTON_SCROLL_DOWN;
            break;
        }
        
        if(wheel_repeat == BUTTON_NONE){
            if(btn & BUTTON_SCROLL_UP)
                wheel_repeat = BUTTON_SCROLL_UP;
            
            if(btn & BUTTON_SCROLL_DOWN)
                wheel_repeat = BUTTON_SCROLL_DOWN;
        } else if (wheel_repeat == BUTTON_SCROLL_UP)  {
            btn |= BUTTON_SCROLL_UP;
            wheel_repeat = BUTTON_NONE;
        } else if (wheel_repeat == BUTTON_SCROLL_DOWN) {
            btn |= BUTTON_SCROLL_DOWN;
            wheel_repeat = BUTTON_NONE;
        }
        
        old_wheel_value = new_wheel_value;
    }
    
    if(wheel_backlight_timer>0){
        wheel_backlight_timer--;
        if(wheel_backlight_timer==0){
            wheel_backlight_on(false);
        }
    }
    
    if( (btn & BUTTON_SCROLL_UP) || (btn & BUTTON_SCROLL_DOWN) ){
        /* only trigger once per click */
        if ((new_wheel_value == 0x00) || (new_wheel_value == 0xc0))
        {
            btn = BUTTON_NONE;
        }
        if(wheel_backlight_timer==0){
            wheel_backlight_on(true);
        }
        wheel_backlight_timer = WHEEL_BACKLIGHT_TIMEOUT;
    }
    
    return btn;
}
