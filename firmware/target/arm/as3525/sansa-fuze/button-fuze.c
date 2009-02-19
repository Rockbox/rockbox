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

/* Buttons */
static bool hold_button     = false;
#ifndef BOOTLOADER
static bool hold_button_old = false;
#endif
static short _dbop_din    = BUTTON_NONE;

/* in the lcd driver */
extern bool lcd_button_support(void);

void button_init_device(void)
{
    GPIOA_DIR |= (1<<1); 	 
    GPIOA_PIN(1) = (1<<1);
}

#if !defined(BOOTLOADER) && defined(HAVE_SCROLLWHEEL)
static void scrollwheel(void)
{
    static unsigned old_wheel_value = 0;
    static unsigned wheel_value     = 0;
    static unsigned wheel_repeat    = BUTTON_NONE;
    unsigned        btn             = BUTTON_NONE;
    /* getting BUTTON_REPEAT works like this: We increment repeat by 2 if the
     * wheel was turned, and decrement it by 1 each tick,
     * that means: if you change the wheel fast enough, repeat will be >1 and
     * we send BUTTON_REPEAT
     */
    static int repeat = 0;
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
    wheel_value = _dbop_din & (1<<13|1<<14);
    wheel_value >>= 13;

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
                buttonlight_on();
                backlight_on();
                queue_post(&button_queue, btn, 1<<24);
                /* message posted - reset count */
                counter = 0;
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

bool button_hold(void)
{
    return hold_button;
}

static short button_dbop(void)
{
    /* skip home reading if lcd_button_support was blocked,
     * since the dbop bit 15 is invalid then, and use the old value instead */
    /* -20 (arbitary value) indicates valid home button read */
    int old_home = -20;
    int delay = 0;
    if(!lcd_button_support())
        old_home = (_dbop_din & 1<<15);

    /* Wait for fifo to empty */
    while ((DBOP_STAT & (1<<10)) == 0);

    DBOP_CTRL |= (1<<19);
    DBOP_CTRL &= ~(1<<16); /* disable output */

    DBOP_TIMPOL_01 = 0xe167e167;
    DBOP_TIMPOL_23 = 0xe167006e;

    while(delay++ < 64);

    DBOP_CTRL |= (1<<15); /* start read */
    ((DBOP_STAT & (1<<16)) == 0); /* wait for valid data */

    _dbop_din = DBOP_DIN; /* now read */

    DBOP_TIMPOL_01 = 0x6e167;
    DBOP_TIMPOL_23 = 0xa167e06f;

    DBOP_CTRL |= (1<<16);
    DBOP_CTRL &= ~(1<<19);

    if (old_home != -20)
        _dbop_din |= old_home;
    return _dbop_din;
}

/* for the debug menu */
short button_dbop_data(void)
{
    return _dbop_din;
}

static int button_gpio(void)
{
    int btn = BUTTON_NONE;
    if(hold_button)
        return btn;
    /* set afsel, so that we can read our buttons */
    GPIOC_AFSEL &= ~(1<<2|1<<3|1<<4|1<<5|1<<6);
    /* set dir so we can read our buttons (but reset the C pins first) */
    GPIOB_DIR &= ~(1<<4);
    GPIOC_DIR |= (1<<2|1<<3|1<<4|1<<5|1<<6);
    GPIOC_PIN(2) = (1<<2);
    GPIOC_PIN(3) = (1<<3);
    GPIOC_PIN(4) = (1<<4);
    GPIOC_PIN(5) = (1<<5);
    GPIOC_PIN(6) = (1<<6);

    GPIOC_DIR &= ~(1<<2|1<<3|1<<4|1<<5|1<<6);

    /* small delay needed to read buttons correctly */
    int delay = 0;
    while(delay++ < 32);

    /* direct GPIO connections */
    if (!GPIOC_PIN(3))
        btn |= BUTTON_LEFT;
    if (!GPIOC_PIN(2))
        btn |= BUTTON_UP;
    if (!GPIOC_PIN(6))
        btn |= BUTTON_DOWN;
    if (!GPIOC_PIN(5))
        btn |= BUTTON_RIGHT;
    if (!GPIOC_PIN(4))
        btn |= BUTTON_SELECT;
    /* return to settings needed for lcd */
    GPIOC_DIR |= (1<<2|1<<3|1<<4|1<<5|1<<6);
    GPIOC_AFSEL |= (1<<2|1<<3|1<<4|1<<5|1<<6);

    return btn;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    short dbop = button_dbop();
    static unsigned power_counter = HZ;
    /* hold button */
    if(dbop & (1<<12))
    {
        power_counter = 0;
        hold_button = true;
    }
    else
    {
        /* might wrap, but shouldn't be much of an issue*/
        power_counter++;
        hold_button = false;
#if defined(HAVE_SCROLLWHEEL) && !defined(BOOTLOADER)
    /* read wheel on bit 13 & 14, but sent to the button queue seperately */
        scrollwheel();
#endif
    /* read power on bit 8, but not if hold button was just released, since
     * you basically always hit power due to the slider mechanism after releasing
     * hold (wait ~1 sec) */
        if (dbop & (1<<8) && power_counter>HZ)
            btn |= BUTTON_POWER;
    /* read home on bit 15 */
        if (!(dbop & (1<<15)))
            btn |= BUTTON_HOME;
        btn |= button_gpio();
    }

#ifndef BOOTLOADER
    /* light handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
#endif /* BOOTLOADER */

    return btn;
}
