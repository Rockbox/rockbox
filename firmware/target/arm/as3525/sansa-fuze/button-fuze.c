/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Thomas Martitz
 * Copyright (C) 2008 by Dominik Wenger
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


/* Basic button driver for the Fuze
 *
 * TODO: - Get the wheel working with interrupts
 *       - find that Home button
 */

#include "system.h"
#include "button.h"
#include "button-target.h"
#include "backlight.h"

#define WHEEL_REPEAT_INTERVAL   30
#define WHEELCLICKS_PER_ROTATION    48 /* wheelclicks per full rotation */

#ifndef BOOTLOADER
/* Buttons */
static bool hold_button     = false;
static bool hold_button_old = false;
#else
#define hold_button false
#endif /* !BOOTLOADER */
static int      int_btn         = BUTTON_NONE;
static short    dbop_din        = BUTTON_NONE;

void button_init_device(void)
{
    GPIOA_DIR |= (1<<1);
    GPIOA_PIN(1) = (1<<1);
}

/* clickwheel */
#if !defined(BOOTLOADER) && defined(HAVE_SCROLLWHEEL)
static void get_wheel(void)
{
    static unsigned int  old_wheel_value    = 0;
    static unsigned int  wheel_value        = 0;
    static unsigned int  wheel_repeat       = BUTTON_NONE;
    /* getting BUTTON_REPEAT works like this: We increment repeat by if the
     * wheel was turned, and decrement it by 1 each tick,
     * that means: if you change the wheel fast enough, repeat will be >1 and
     * we send BUTTON_REPEAT
     */
    static int repeat;
    /* we omit 3 of 4 posts to the button_queue, that works better, so count */
    static int counter = 0;
    /* Read wheel 
     * Bits 13 and 14 of DBOP_DIN change as follows:
     * Clockwise rotation   00 -> 01 -> 11 -> 10 -> 00
     * Counter-clockwise    00 -> 10 -> 11 -> 01 -> 00
     */
    static const unsigned char wheel_tbl[2][4] =
    {
        { 2, 0, 3, 1 }, /* Clockwise rotation */
        { 1, 3, 0, 2 }, /* Counter-clockwise  */ 
    };
    wheel_value = dbop_din & (1<<13|1<<14);
    wheel_value >>= 13;
    /* did the wheel value change? */    
    if (!hold_button)
    {
        unsigned int btn = BUTTON_NONE;
        if (old_wheel_value == wheel_tbl[0][wheel_value])
            btn = BUTTON_SCROLL_FWD;
        else if (old_wheel_value == wheel_tbl[1][wheel_value])
            btn = BUTTON_SCROLL_BACK;

        if (btn != BUTTON_NONE)
        {
            if (btn != wheel_repeat)
            {
                /* direction reversals nullify repeats */
                wheel_repeat      = btn;
                repeat            = 0;
            }
            if (btn != BUTTON_NONE)
            {
                /* generate repeats if quick enough */
                if (repeat > 0)
                {
                    btn |= BUTTON_REPEAT;
                }
                repeat += 2;
                /* the wheel is more reliable if we don't send ever change,
                 * every 4th is basically one "physical click" is 1 item in
                 * the rockbox menus */
                if (queue_empty(&button_queue) && ++counter >= 4)
                {
                    backlight_on();
                    /* 1<<24 is rather arbitary, seems to work well */
                    queue_post(&button_queue, btn, 1<<24);
                    /* message posted - reset count */
                    counter = 0;
                }
            }
        }
    }
    if (repeat > 0)
        repeat--;
    else
        repeat = 0;
    old_wheel_value = wheel_value;
}
#endif /* !defined(BOOTLOADER) && defined(SCROLLWHEEL) */

#if !defined(BOOTLOADER)
/* get hold button state */
static void get_hold(void)
{
    hold_button = dbop_din & (1<<12);
}
#endif

bool button_hold(void)
{
    return hold_button;
}

static void get_power(void)
{
    if (dbop_din & (1<<8))
        int_btn |= BUTTON_POWER;
}

static void get_button_from_dbob(void)
{
    int_btn &= ~(BUTTON_HOLD|
                BUTTON_POWER);

    /* Wait for fifo to empty */
    while ((DBOP_STAT & (1<<10)) == 0);

    DBOP_CTRL |= (1<<19);
    DBOP_CTRL &= ~(1<<16); /* disable output */

    DBOP_TIMPOL_01 = 0xe167e167;
    DBOP_TIMPOL_23 = 0xe167006e;
    int loop = 0;
    do
    {
        asm volatile ("nop\n");
        loop++;
    } while(loop < 64);

    DBOP_CTRL |= (1<<15); /* start read */
    int temp;
    do
    {
        temp = DBOP_STAT;
    } while ((temp & (1<<16)) == 0); /* wait for valid data */

    dbop_din = DBOP_DIN; /* now read */

    DBOP_TIMPOL_01 = 0x6e167;
    DBOP_TIMPOL_23 = 0xa167e06f;

    DBOP_CTRL |= (1<<16);
    DBOP_CTRL &= ~(1<<19);

#if !defined(BOOTLOADER)
    get_hold();
#if defined(HAVE_SCROLLWHEEL)
    get_wheel();
#endif
#endif
    get_power();
}

static void get_button_from_gpio(void)
{
    /* reset buttons we're going to read */
    int_btn &= ~(BUTTON_LEFT|
                BUTTON_RIGHT|
                BUTTON_UP|
                BUTTON_DOWN|
                BUTTON_SELECT);
    if(hold_button)
        return;
    /* set afsel, so that we can read our buttons */
    GPIOC_AFSEL &= ~(1<<2|1<<3|1<<4|1<<5|1<<6);
    /* set dir so we can read our buttons (but reset the C pins first) */
    GPIOB_DIR &= ~(1<<4);
    GPIOC_DIR |= (1<<2|1<<3|1<<4|1<<5|1<<6);
    GPIOC_PIN(2) |= (1<<2);
    GPIOC_PIN(3) |= (1<<3);
    GPIOC_PIN(4) |= (1<<4);
    GPIOC_PIN(5) |= (1<<5);
    GPIOC_PIN(6) |= (1<<6);

    GPIOC_DIR &= ~(1<<2|1<<3|1<<4|1<<5|1<<6);

    /* small delay needed to read buttons correctly */
    int delay = 50;
    while(delay >0) delay--;

    /* direct GPIO connections */
    if (!GPIOC_PIN(3))
        int_btn |= BUTTON_LEFT;
    if (!GPIOC_PIN(2))
        int_btn |= BUTTON_UP;
    if (!GPIOC_PIN(6))
        int_btn |= BUTTON_DOWN;
    if (!GPIOC_PIN(5))
        int_btn |= BUTTON_RIGHT;
    if (!GPIOC_PIN(4))
        int_btn |= BUTTON_SELECT;
    /* return to settings needed for lcd */
    GPIOC_DIR |= (1<<2|1<<3|1<<4|1<<5|1<<6);
    GPIOC_AFSEL |= (1<<2|1<<3|1<<4|1<<5|1<<6);
}

static inline void get_buttons_from_hw(void)
{
    get_button_from_dbob();
    get_button_from_gpio();
}
/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    get_buttons_from_hw();
#ifndef BOOTLOADER
    /* light handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
#endif /* BOOTLOADER */

    return int_btn; /* set in button_int */
}
