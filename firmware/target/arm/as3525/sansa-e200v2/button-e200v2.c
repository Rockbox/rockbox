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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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


static bool hold_button     = false;
#ifndef BOOTLOADER
static bool hold_button_old = false;
#endif
static unsigned short _dbop_din      = 0;
/* read_missed is true if buttons could not
 * be read (see lcd_button_support) */
static bool read_missed              = false;

#define WHEEL_REPEAT_INTERVAL   (HZ/4)
/* in the lcd driver */
extern bool lcd_button_support(void);

void button_init_device(void)
{

}

bool button_hold(void)
{
    return hold_button;
}

#if !defined(BOOTLOADER) && defined(HAVE_SCROLLWHEEL)
static void scrollwheel(unsigned short dbop_din)
{
    /* current wheel values, parsed from dbop and the resulting button */
    unsigned        wheel_value     = 0;
    unsigned        btn             = BUTTON_NONE;
    /* old wheel values */
    static unsigned old_wheel_value = 0;
    static unsigned old_btn         = BUTTON_NONE;

    /* getting BUTTON_REPEAT works like this: Remember when the btn value was
     * posted to the button_queue last, and if it was recent enough, generate
     * BUTTON_REPEAT
     */
    static long last_wheel_post     = 0;
    /* Repeat is used for the scrollwheel acceleration. If high enough then
     * jump over some items */
    static unsigned repeat          = 0;
    /* we omit 1 of 2 posts to the button_queue, that works better, so count */
    static int counter              = 0;
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

    if(hold_button)
    {
        repeat  =  counter  = 0;
        return;
    }

    wheel_value = dbop_din & (1<<13|1<<14);
    wheel_value >>= 13;

    if (old_wheel_value == wheel_tbl[0][wheel_value])
        btn = BUTTON_SCROLL_FWD;
    else if (old_wheel_value == wheel_tbl[1][wheel_value])
        btn = BUTTON_SCROLL_BACK;

    if (btn != BUTTON_NONE)
    {
        if (btn != old_btn)
        {
            /* direction reversals nullify repeats */
            old_btn =  btn;
            repeat  =  counter  = 0;
        }
        /* wheel_delta will cause lists to jump over items,
         * we want this for fast scrolling, but we must keep it accurate
         * for slow scrolling */
        int wheel_delta = 0;
        /* generate repeats if quick enough, scroll slightly too */
        if (TIME_BEFORE(current_tick, last_wheel_post + WHEEL_REPEAT_INTERVAL))
        {
            btn |= BUTTON_REPEAT;
            wheel_delta = repeat>>1;
        }

        repeat += 3;
        /* the wheel is more reliable if we don't send ever change,
         * every 2th is basically one "physical click" is
         * 1 item in the rockbox menus */
        if (++counter >= 2 && queue_empty(&button_queue))
        {
            buttonlight_on();
            backlight_on();
            queue_post(&button_queue, btn, ((wheel_delta+1)<<24));
            /* message posted - reset count & last post to the queue */
            counter = 0;
            last_wheel_post = current_tick;
        }
    }
    if (repeat > 0 && !read_missed)
        repeat--;

    old_wheel_value = wheel_value;
}
#endif /* !defined(BOOTLOADER) && defined(HAVE_SCROLLWHEEL) */

unsigned short button_read_dbop(void)
{
    /*write a red pixel */
    if (!lcd_button_support())
    {
      read_missed = true;
    }
    if (!read_missed)
    {
        /* Set up dbop for input */
        while (!(DBOP_STAT & (1<<10)));      /* Wait for fifo to empty */
        DBOP_CTRL |= (1<<19);                /* Tri-state DBOP on read cycle */
        DBOP_CTRL &= ~(1<<16);               /* disable output (1:write enabled) */
        DBOP_TIMPOL_01 = 0xe167e167;         /* Set Timing & Polarity regs 0 & 1 */
        DBOP_TIMPOL_23 = 0xe167006e;         /* Set Timing & Polarity regs 2 & 3 */

        DBOP_CTRL |= (1<<15);                /* start read */
        while (!(DBOP_STAT & (1<<16)));      /* wait for valid data */

        int delay=10;
        while(delay--);                      /* short delay before reading */

        _dbop_din = DBOP_DIN;                /* Read dbop data*/

        /* Reset dbop for output */
        DBOP_TIMPOL_01 = 0x6e167;            /* Set Timing & Polarity regs 0 & 1 */
        DBOP_TIMPOL_23 = 0xa167e06f;         /* Set Timing & Polarity regs 2 & 3 */
        DBOP_CTRL |= (1<<16);                /* Enable output (0:write disable)  */
        DBOP_CTRL &= ~(1<<19);               /* Tri-state when no active write */
    }
#if !defined(BOOTLOADER) && defined(HAVE_SCROLLWHEEL)
    scrollwheel(_dbop_din);
#endif
    read_missed = false;
    return _dbop_din;
}

unsigned short button_dbop_data(void)
{
    return _dbop_din;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    /* read buttons from dbop */
    unsigned short dbop = button_read_dbop();

    /* hold button */
    if(dbop & (1<<12))
    {
        hold_button = true;
        return btn;
    }
    else
    {
        hold_button = false;
    }

    if (dbop & (1<<8))
        btn |= BUTTON_POWER;

    if (!(dbop & (1<<15)))
    btn |= BUTTON_REC;

    /* Set afsel, so that we can read our buttons */
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

    int delay = 8;    /* small delay needed to read buttons correctly */
    while(delay--);

     /* direct GPIO connections */
    if (!GPIOC_PIN(2))
        btn |= BUTTON_UP;
    if (!GPIOC_PIN(3))
        btn |= BUTTON_LEFT;
    if (!GPIOC_PIN(4))
        btn |= BUTTON_SELECT;
    if (!GPIOC_PIN(5))
        btn |= BUTTON_RIGHT;
    if (!GPIOC_PIN(6))
        btn |= BUTTON_DOWN;

    /* return to settings needed for lcd */
    GPIOC_DIR |= (1<<2|1<<3|1<<4|1<<5|1<<6);
    GPIOC_AFSEL |= (1<<2|1<<3|1<<4|1<<5|1<<6);

#ifndef BOOTLOADER
    /* light handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
#endif /* BOOTLOADER */
    /* The int_btn variable is set in the button interrupt handler */
    return btn;
}
