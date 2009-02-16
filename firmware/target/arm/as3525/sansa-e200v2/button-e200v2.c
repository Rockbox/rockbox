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

#define WHEEL_REPEAT_INTERVAL   300000
#define WHEEL_FAST_ON_INTERVAL   20000
#define WHEEL_FAST_OFF_INTERVAL  60000
#define WHEELCLICKS_PER_ROTATION    48 /* wheelclicks per full rotation */

/* Clickwheel */
static unsigned int  old_wheel_value   = 0;
static unsigned int  wheel_repeat      = BUTTON_NONE;
static unsigned int  wheel_click_count = 0;
static unsigned int  wheel_delta       = 0;
static          int  wheel_fast_mode   = 0;
static unsigned long last_wheel_usec   = 0;
static unsigned long wheel_velocity    = 0;
static          long last_wheel_post   = 0;
static          long next_backlight_on = 0;
/* Buttons */
static bool hold_button     = false;
#ifndef BOOTLOADER  
static bool hold_button_old = false;
#endif

extern void lcd_button_support(void);

void button_init_device(void)
{

}

bool button_hold(void)
{
    return hold_button;
}

void clickwheel(unsigned int wheel_value)
{
    static const unsigned char wheel_tbl[2][4] =
    {
     { 2, 0, 3, 1 }, /* Clockwise rotation */
     { 1, 3, 0, 2 }, /* Counter-clockwise  */
    };
   /* Read wheel
     * Bits 13 and 14 of DBOP_DIN change as follows:
     * Clockwise rotation   00 -> 01 -> 11 -> 10 -> 00
     * Counter-clockwise    00 -> 10 -> 11 -> 01 -> 00
     */

	/* did the wheel value change? */
    unsigned int btn = BUTTON_NONE;
    if (old_wheel_value == wheel_tbl[0][wheel_value])
        btn = BUTTON_SCROLL_FWD;
    else if (old_wheel_value == wheel_tbl[1][wheel_value])
        btn = BUTTON_SCROLL_BACK;

    if (btn != BUTTON_NONE)
    {
        int repeat = 1; /* assume repeat */
        unsigned long usec = TIMER1_VALUE; /* WAG!!!  and it works!!*/
        unsigned v = (usec - last_wheel_usec) & 0x7fffffff;

        v = (v>0) ? 1000000 / v : 0;     /* clicks/sec = 1000000 *
+clicks/usec */
        v = (v>0xffffff) ? 0xffffff : v; /* limit to 24 bit */

        /* some velocity filtering to smooth things out */
        wheel_velocity = (7*wheel_velocity + v) / 8;

        if (btn != wheel_repeat)
        {
            /* direction reversals nullify all fast mode states */
            wheel_repeat      = btn;
            repeat            =
            wheel_fast_mode   =
            wheel_velocity    =
            wheel_click_count = 0;
        }

        if (wheel_fast_mode != 0)
        {
            /* fast OFF happens immediately when velocity drops below
        threshold */
            if (TIME_AFTER(usec,
                    last_wheel_usec + WHEEL_FAST_OFF_INTERVAL))
            {
                /* moving out of fast mode */
                wheel_fast_mode = 0;
                /* reset velocity */
                wheel_velocity = 0;
                /* wheel_delta is always 1 in slow mode */
                wheel_delta = 1;
            }
        }
        else
        {
            /* fast ON gets filtered to avoid inadvertent jumps to fast mode
*/
            if (repeat && wheel_velocity > 1000000/WHEEL_FAST_ON_INTERVAL)
            {
                /* moving into fast mode */
                wheel_fast_mode = 1 << 31;
                wheel_click_count = 0;
                wheel_velocity = 1000000/WHEEL_FAST_OFF_INTERVAL;
            }
            else if (++wheel_click_count < 2)
            {
                btn = BUTTON_NONE;
            }

            /* wheel_delta is always 1 in slow mode */
            wheel_delta = 1;
        }

        if (TIME_AFTER(current_tick, next_backlight_on) ||
            v <= 4)
        {
            /* poke backlight to turn it on or maintain it no more often
        than every 1/4 second*/
            next_backlight_on = current_tick + HZ/4;
            backlight_on();
            buttonlight_on();
            reset_poweroff_timer();
        }

         if (btn != BUTTON_NONE)
         {
             wheel_click_count = 0;

             /* generate repeats if quick enough */
             if (repeat && TIME_BEFORE(usec,
                     last_wheel_post + WHEEL_REPEAT_INTERVAL))
                 btn |= BUTTON_REPEAT;

             last_wheel_post = usec;

             if (queue_empty(&button_queue))
             {
                 queue_post(&button_queue, btn, wheel_fast_mode |
                            (wheel_delta << 24) |
			wheel_velocity*360/WHEELCLICKS_PER_ROTATION);
                 /* message posted - reset delta */
                 wheel_delta = 1;
             }
             else
             {
                 /* skipped post - increment delta */
                 if (++wheel_delta > 0x7f)
                     wheel_delta = 0x7f;
             }
         }

         last_wheel_usec = usec;
    }
    old_wheel_value = wheel_value;
}

int read_dbop(void)
{
    /*write a red pixel */
   lcd_button_support();

  /* Set up dbop for input */
    while (!(DBOP_STAT & (1<<10))); 	  /* Wait for fifo to empty */
    DBOP_CTRL |= (1<<19);
    DBOP_CTRL &= ~(1<<16);               /* disable output */
  
    DBOP_TIMPOL_01 = 0xe167e167; //11100001011001111110000101100111
    DBOP_TIMPOL_23 = 0xe167006e; //11100001011001110000000001101110
 
    DBOP_CTRL |= (1<<15); 		/* start read */
    while (!(DBOP_STAT & (1<<16))); 	/* wait for valid data */
    int delay = 50;
    while(delay--);    			/* small delay to set up read */

    int ret = DBOP_DIN; 		/* now read dbop & store info*/

    DBOP_TIMPOL_01 = 0x6e167;
    DBOP_TIMPOL_23 = 0xa167e06f;
    DBOP_CTRL |= (1<<16);
    DBOP_CTRL &= ~(1<<19);

    return ret;    
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    /* read buttons from dbop */
    int dbop = read_dbop();
    
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

    /* handle wheel */
    int wheel_value = dbop & (1<<13|1<<14);
    wheel_value >>= 13;
    clickwheel(wheel_value);    
        
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

    int delay = 50;    /* small delay needed to read buttons correctly */
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
