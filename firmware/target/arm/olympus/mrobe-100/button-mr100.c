/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Mark Arigo
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

#include "system.h"
#include "button.h"
#include "backlight.h"
#include "backlight-target.h"
#include "synaptics-mep.h"

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote-target.h"
static bool remote_hold = false;
static bool headphones_status = true;
#endif

/*#define LOGF_ENABLE*/
#include "logf.h"

static int int_btn = BUTTON_NONE;

#ifndef BOOTLOADER
void button_init_device(void)
{
    /* enable touchpad leds */
    GPIOA_ENABLE     |= BUTTONLIGHT_ALL;
    GPIOA_OUTPUT_EN  |= BUTTONLIGHT_ALL;
    
    /* enable touchpad */
    GPO32_ENABLE     |=  0x40000000;
    GPO32_VAL        &= ~0x40000000;

    /* enable ACK, CLK, DATA lines */
    GPIOD_ENABLE     |=  (0x1 | 0x2 | 0x4);

    GPIOD_OUTPUT_EN  |=  0x1; /* ACK  */
    GPIOD_OUTPUT_VAL |=  0x1; /* high */

    GPIOD_OUTPUT_EN  &= ~0x2; /* CLK  */
    
    GPIOD_OUTPUT_EN  |=  0x4; /* DATA */
    GPIOD_OUTPUT_VAL |=  0x4; /* high */

    if (!touchpad_init())
    {
        logf("touchpad not ready");
    }

    /* headphone detection bit */
    GPIOD_OUTPUT_EN &= ~0x80;
    GPIOD_ENABLE |= 0x80;

    /* remote detection (via headphone state) */
    headphones_int();
}

/*
 * Button interrupt handler
 */
void button_int(void)
{
    char data[4];
    int val;

    int_btn = BUTTON_NONE;

    val = touchpad_read_device(data, 4);

    if (val == MEP_BUTTON_HEADER)
    {
        /* Buttons packet - touched one of the 5 "buttons" */
        if (data[1] & 0x1) int_btn |= BUTTON_PLAY;
        if (data[1] & 0x2) int_btn |= BUTTON_MENU;
        if (data[1] & 0x4) int_btn |= BUTTON_LEFT;
        if (data[1] & 0x8) int_btn |= BUTTON_DISPLAY;
        if (data[2] & 0x1) int_btn |= BUTTON_RIGHT;
    }
    else if (val == MEP_ABSOLUTE_HEADER)
    {
        /* Absolute packet - the finger is on the vertical strip.
           Position ranges from 1-4095, with 1 at the bottom. */
        val = ((data[1] >> 4) << 8) | data[2]; /* position */

        if ((val > 0) && (val <= 1365))
            int_btn |= BUTTON_DOWN;
        else if ((val > 1365) && (val <= 2730))
            int_btn |= BUTTON_SELECT;
        else if ((val > 2730) && (val <= 4095))
            int_btn |= BUTTON_UP;
    }
}
#else
void button_init_device(void){}
#endif /* bootloader */

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;

#ifdef HAVE_REMOTE_LCD
    unsigned char data[5];

    if (lcd_remote_read_device(data))
    {
        remote_hold = (data[2] & 0x80) ? true : false;
        if (!remote_hold)
        {
            if (data[1] & 0x1)  btn |= BUTTON_RC_PLAY;
            if (data[1] & 0x2)  btn |= BUTTON_RC_DOWN;
            if (data[1] & 0x4)  btn |= BUTTON_RC_FF;
            if (data[1] & 0x8)  btn |= BUTTON_RC_REW;
            if (data[1] & 0x10) btn |= BUTTON_RC_VOL_UP;
            if (data[1] & 0x20) btn |= BUTTON_RC_VOL_DOWN;
            if (data[1] & 0x40) btn |= BUTTON_RC_MODE;
            if (data[1] & 0x80) btn |= BUTTON_RC_HEART;
        }
    }
#endif

    if(!button_hold())
    {
        btn |= int_btn;

        if (~GPIOA_INPUT_VAL & 0x40)
            btn |= BUTTON_POWER;
    }

    return btn;
}

bool button_hold(void)
{
    return (GPIOD_INPUT_VAL & 0x10) ? false : true;
}

#ifdef HAVE_REMOTE_LCD
bool remote_button_hold(void)
{
    return remote_hold;
}

bool headphones_inserted(void)
{
    return headphones_status;
}

void headphones_int(void)
{
    int state = 0x80 & ~GPIOD_INPUT_VAL;
    headphones_status = (state) ? true : false;

    GPIO_CLEAR_BITWISE(GPIOD_INT_EN, 0x80);
    GPIO_WRITE_BITWISE(GPIOD_INT_LEV, state, 0x80);
    GPIO_WRITE_BITWISE(GPIOD_INT_CLR, 0x80, 0x80);
    GPIO_SET_BITWISE(GPIOD_INT_EN, 0x80);

    lcd_remote_on();
}
#else
bool headphones_inserted(void)
{
    return (GPIOD_INPUT_VAL & 0x80) ? false : true;
}
#endif
