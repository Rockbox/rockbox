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

#include "system.h"
#include "button.h"
#include "button-target.h"
#include "backlight.h"


#ifdef SANSA_FUZE
#define DBOP_BIT15_BUTTON       BUTTON_HOME
#define WHEEL_REPEAT_INTERVAL   (HZ/5)
#define WHEEL_COUNTER_DIV       4
#define ACCEL_INCREMENT         2
#define ACCEL_SHIFT             2
#define BUTTON_DELAY            60
#endif

#ifdef SANSA_E200V2
#define DBOP_BIT15_BUTTON       BUTTON_REC
#define WHEEL_REPEAT_INTERVAL   (HZ/5)
#define WHEEL_COUNTER_DIV       2
#define ACCEL_INCREMENT         3
#define ACCEL_SHIFT             1
#define BUTTON_DELAY            20

/* read_missed is true if buttons could not
 * be read (see lcd_button_support) */
static bool read_missed              = false;

#endif

/* Buttons */
static bool hold_button     = false;
#ifndef BOOTLOADER
static bool hold_button_old = false;
#endif
static unsigned short _dbop_din    = BUTTON_NONE;

/* in the lcd driver */
extern bool lcd_button_support(void);

void button_init_device(void)
{
    GPIOA_DIR |= (1<<1);     
    GPIOA_PIN(1) = (1<<1);
}

#if !defined(BOOTLOADER) && defined(HAVE_SCROLLWHEEL)
static void scrollwheel(unsigned short dbop_din)
{
    /* current wheel values, parsed from dbop and the resulting button */
    unsigned         wheel_value     = 0;
    unsigned         btn             = BUTTON_NONE;
    /* old wheel values */
    static unsigned old_wheel_value = 0;
    static unsigned old_btn         = BUTTON_NONE;

    /*
     * Getting BUTTON_REPEAT works like this: Remember when the btn value was
     * posted to the button_queue last, and if it was recent enough, generate
     * BUTTON_REPEAT
     */
    static long      last_wheel_post = 0;

    /*
     *  Providing wheel acceleration works as follows: We increment accel
     * by 2 if the wheel was turned, and decrement it by 1 each tick
     * (no matter if it was turned), that means: the longer and faster you turn,
     * the higher accel will be. accel>>2 will actually posted to the button_queue
     */
    static int        accel = 0;
    /* We only post every 4th action, as this matches better with the physical
     * clicks of the wheel */
    static int        counter = 0;
    /* Read wheel 
     * Bits 13 and 14 of DBOP_DIN change as follows (Gray Code):
     * Clockwise rotation   00 -> 01 -> 11 -> 10 -> 00
     * Counter-clockwise    00 -> 10 -> 11 -> 01 -> 00
     *
     * For easy look-up, actual wheel values act as indicies also,
     * which is why the table seems to be not ordered correctly
     */
    static const unsigned char wheel_tbl[2][4] =
    {
        { 2, 0, 3, 1 }, /* Clockwise rotation */
        { 1, 3, 0, 2 }, /* Counter-clockwise  */ 
    };

    if(hold_button)
    {
        accel  =  counter  = 0;
        return;
    }

    wheel_value = (dbop_din >> 13) & (1<<1|1<<0);

    if (old_wheel_value == wheel_tbl[0][wheel_value])
        btn = BUTTON_SCROLL_FWD;
    else if (old_wheel_value == wheel_tbl[1][wheel_value])
        btn = BUTTON_SCROLL_BACK;
    else if (old_wheel_value != wheel_value && accel > ACCEL_INCREMENT)
    {   /* if no button is read and wheel_value changed, assume old_btn */
        btn = old_btn;
    }
    /* else btn = BUTTON_NONE */

    if (btn != BUTTON_NONE)
    {
        if (btn != old_btn)
        {
            /* direction reversals nullify acceleration and counters */
            old_btn =  btn;
            accel  =  counter  = 0;
        }
        /* wheel_delta will cause lists to jump over items,
         * we want this for fast scrolling, but we must keep it accurate
         * for slow scrolling */
        int wheel_delta = 0;
        /* generate BUTTON_REPEAT if quick enough, scroll slightly faster too*/
        if (TIME_BEFORE(current_tick, last_wheel_post + WHEEL_REPEAT_INTERVAL))
        {
            btn |= BUTTON_REPEAT;
            wheel_delta = accel>>ACCEL_SHIFT;
        }

        accel += ACCEL_INCREMENT;

        /* the wheel is more reliable if we don't send every change,
         * every WHEEL_COUNTER_DIVth is basically one "physical click" 
         * which should make up 1 item in lists */
        if (++counter >= WHEEL_COUNTER_DIV && queue_empty(&button_queue))
        {
            buttonlight_on();
            backlight_on();
            queue_post(&button_queue, btn, ((wheel_delta+1)<<24));
            /* message posted - reset count and remember post */
            counter = 0;
            last_wheel_post = current_tick;
        }
    }
    if (accel > 0
#ifdef SANSA_E200V2
         && !read_missed /* decrement only if reading buttons was successful */
#endif         
        )
        accel--;

    old_wheel_value = wheel_value;
}
#endif /* !defined(BOOTLOADER) && defined(SCROLLWHEEL) */

bool button_hold(void)
{
    return hold_button;
}

static void button_delay(void)
{
    int i = BUTTON_DELAY;
    while(i--) asm volatile ("nop\n");
}

unsigned short button_read_dbop(void)
{
#ifdef SANSA_FUZE
    /* skip home and power reading if lcd_button_support was blocked,
     * since the dbop bit 15 is invalid then, and use the old value instead
     * -20 (arbitary value) indicates valid home&power button read 
     * (fuze only) */
    int old_home_power = -20;
#endif
    if(!lcd_button_support())
    {
#if defined(SANSA_FUZE)
        old_home_power = (_dbop_din & (1<<15|1<<8));
#elif defined(SANSA_E200V2)
        read_missed = true;
#endif
    }

#ifdef SANSA_E200V2
    if (!read_missed)   /* read buttons only if lcd_button_support was not blocked */
#endif
    {
        /* Set up dbop for input */
        DBOP_CTRL |= (1<<19);                /* Tri-state DBOP on read cycle */
        DBOP_CTRL &= ~(1<<16);               /* disable output (1:write enabled) */
        DBOP_TIMPOL_01 = 0xe167e167;         /* Set Timing & Polarity regs 0 & 1 */
        DBOP_TIMPOL_23 = 0xe167006e;         /* Set Timing & Polarity regs 2 & 3 */

        button_delay();
        DBOP_CTRL |= (1<<15);                /* start read */
        while (!(DBOP_STAT & (1<<16)));      /* wait for valid data */

        _dbop_din = DBOP_DIN;                /* Read dbop data*/

        /* Reset dbop for output */
        DBOP_TIMPOL_01 = 0x6e167;            /* Set Timing & Polarity regs 0 & 1 */
        DBOP_TIMPOL_23 = 0xa167e06f;         /* Set Timing & Polarity regs 2 & 3 */
        DBOP_CTRL |= (1<<16);                /* Enable output (0:write disable)  */
        DBOP_CTRL &= ~(1<<19);               /* Tri-state when no active write */
    }

#ifdef SANSA_FUZE
    /* write back old values if blocked */
    if (old_home_power != -20)
    {
        _dbop_din |= old_home_power & 1<<15;
        _dbop_din &= 0xfeff|(old_home_power & 1<<8);
    }
#endif

#if defined(HAVE_SCROLLWHEEL) && !defined(BOOTLOADER)
    /* read wheel on bit 13 & 14, but sent to the button queue seperately */
        scrollwheel(_dbop_din);
#endif

#ifdef SANSA_E200V2
    read_missed = false;
#endif

    return _dbop_din;
}

/* for the debug menu */
unsigned short button_dbop_data(void)
{
    return _dbop_din;
}

static int button_gpio(void)
{
    int btn = BUTTON_NONE;
    if(hold_button)
        return btn;

    /* disable DBOP output while changing GPIO pins that share lines with it */
    DBOP_CTRL &= ~(1<<16);
    button_delay();
    
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
    button_delay();

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
    
    DBOP_CTRL |= (1<<16);               /* enable output again */
    return btn;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    unsigned short dbop = button_read_dbop();
#ifdef SANSA_FUZE
    static unsigned power_counter = 0;
#endif
    /* hold button */
    if(dbop & (1<<12))
    {
#ifdef SANSA_FUZE
        power_counter = HZ;
#endif
        hold_button = true;
    }
    else
    {
        hold_button = false;
#ifdef SANSA_FUZE
    /* read power on bit 8, but not if hold button was just released, since
     * you basically always hit power due to the slider mechanism after releasing
     * (fuze only)
     * hold (wait 1 sec) */
        if (power_counter)
            power_counter--;
#endif
        if (dbop & (1<<8)
#ifdef SANSA_FUZE
            && !power_counter
#endif
            )
            btn |= BUTTON_POWER;
    /* read home on bit 15 */
        if (!(dbop & (1<<15)))
            btn |= DBOP_BIT15_BUTTON;

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
