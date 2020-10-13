/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Mark Arigo
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
#include "kernel.h"
#include "button.h"
#include "backlight.h"
#if defined(SAMSUNG_YH920) || defined(SAMSUNG_YH925)
#include "powermgmt.h"
#include "adc.h"

static int int_btn = BUTTON_NONE;
static unsigned int rec_switch;

void button_init_device(void)
{
    /* remote interrupt - low when key is pressed */
    GPIOD_ENABLE |= 0x01;
    GPIOD_OUTPUT_EN &= ~0x01;

    /* disable/reset int */
    GPIOD_INT_EN  &= ~0x01;
    GPIOD_INT_CLR |=  0x01;

    /* enable int */
    GPIOD_INT_LEV &= ~0x01;
    GPIOD_INT_EN  |=  0x01;

    /* remote PLAY */
    GPIOD_ENABLE |= 0x02;
    GPIOD_OUTPUT_EN &= ~0x02;

    /* current record switch state */
    rec_switch = ~GPIOA_INPUT_VAL & 0x40;
}

/* Remote buttons */
void remote_int(void)
{
    int state = 0x01 & ~GPIOD_INPUT_VAL;

    GPIO_CLEAR_BITWISE(GPIOD_INT_EN, 0x01);
    GPIO_WRITE_BITWISE(GPIOD_INT_LEV, state, 0x01);

    if (state != 0)
    {
        /* necessary delay 1msec */
        udelay(1000);
        unsigned int val = adc_scan(ADC_REMOTE);
        if (val > 750) int_btn = BUTTON_RC_MINUS;
        else
        if (val > 375) int_btn = BUTTON_RC_PLUS;
        else
        if (val > 100) int_btn = BUTTON_RC_REW;
        else
        int_btn = BUTTON_RC_FFWD;
    }
    else
    int_btn = BUTTON_NONE;

    GPIO_SET_BITWISE(GPIOD_INT_CLR, 0x01);
    GPIO_SET_BITWISE(GPIOD_INT_EN, 0x01);
}
#else
void button_init_device(void)
{
    /* nothing */
}
#endif /* (SAMSUNG_YH920) || (SAMSUNG_YH925) */


bool button_hold(void)
{
    return (~GPIOA_INPUT_VAL & 0x1);
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
#if defined(SAMSUNG_YH920) || defined(SAMSUNG_YH925)
    int btn = int_btn;
#else
    int btn = BUTTON_NONE;
#endif /* (SAMSUNG_YH920) || (SAMSUNG_YH925) */
    static bool hold_button = false;
#ifndef BOOTLOADER
    bool hold_button_old;
#endif

    /* Hold */
#ifndef BOOTLOADER
    hold_button_old = hold_button;
#endif
    hold_button = button_hold();

#ifndef BOOTLOADER
    if (hold_button != hold_button_old)
        backlight_hold_changed(hold_button);
#endif

    /* device buttons */
    if (!hold_button)
    {
        if (~GPIOA_INPUT_VAL & 0x04) btn |= BUTTON_LEFT;
        if (~GPIOA_INPUT_VAL & 0x20) btn |= BUTTON_RIGHT;
        if (~GPIOA_INPUT_VAL & 0x10) btn |= BUTTON_UP;
        if (~GPIOA_INPUT_VAL & 0x08) btn |= BUTTON_DOWN;
        if (~GPIOA_INPUT_VAL & 0x02) btn |= BUTTON_FFWD;
        if (~GPIOA_INPUT_VAL & 0x80) btn |= BUTTON_REW;
#if defined(SAMSUNG_YH820)
        if (~GPIOA_INPUT_VAL & 0x40) btn |= BUTTON_REC;
#else
        if ((~GPIOA_INPUT_VAL & 0x40) != rec_switch)
        {
            if (rec_switch) {
                queue_post(&button_queue,BUTTON_REC_SW_OFF,0);
                queue_post(&button_queue,BUTTON_REC_SW_OFF|BUTTON_REL,0);
            }
            else {
                queue_post(&button_queue,BUTTON_REC_SW_ON,0);
                queue_post(&button_queue,BUTTON_REC_SW_ON|BUTTON_REL,0);
            }
            rec_switch = ~GPIOA_INPUT_VAL & 0x40;
            backlight_on();
            reset_poweroff_timer();
        }
#endif
#if defined(SAMSUNG_YH820)
        if ( GPIOB_INPUT_VAL & 0x80) btn |= BUTTON_PLAY;
#elif defined(SAMSUNG_YH920) || defined(SAMSUNG_YH925)
        if ( GPIOD_INPUT_VAL & 0x04) btn |= BUTTON_PLAY;
        if ( GPIOD_INPUT_VAL & 0x02) btn |= BUTTON_RC_PLAY; /* Remote PLAY */
#endif
    }

    return btn;
}
