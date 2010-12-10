/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Marcin Bukat
 * scrollstrip logic inspired by button-mini1g.c
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

#include "config.h"
#include "cpu.h"
#include "system.h"
#include "button.h"
#include "backlight.h"
#include "adc.h"
#include "powermgmt.h"

#define SLIDER_BASE_SENSITIVITY 8

/* GPI7 H-L, GPI6 H-L, GPI7 L-H, GPI6 L-H */
#define SLIDER_GPIO_MASK ((1<<15)|(1<<14)|(1<<7)|(1<<6))

static inline void disable_scrollstrip_interrupts(void)
{
   and_l(~SLIDER_GPIO_MASK,&GPIO_INT_EN);
}

static inline void enable_scrollstrip_interrupts(void)
{

    or_l(SLIDER_GPIO_MASK,&GPIO_INT_EN);
}

static inline void ack_scrollstrip_interrupt(void)
{
    or_l(SLIDER_GPIO_MASK,&GPIO_INT_CLEAR);
}

void scrollstrip_isr(void) __attribute__ ((interrupt_handler,section(".icode")));
void scrollstrip_isr(void)
{
    /* reading line 6 and 7 forms four possible states:
     * 0, 1 , 2, 3
     * tracking the order of line changes we can judge
     * if the slide is up or down in the following way:
     * sliding up order:   0 2 3 1 0 2 3 1
     * sliding down order: 0 1 3 2 0 1 3 2
     */
    static const signed char scroll_state[4][4] ICONST_ATTR = {
        { BUTTON_NONE, BUTTON_DOWN, BUTTON_UP,   BUTTON_NONE},
        { BUTTON_UP,   BUTTON_NONE, BUTTON_NONE, BUTTON_DOWN},
        { BUTTON_DOWN, BUTTON_NONE, BUTTON_NONE, BUTTON_UP},
        { BUTTON_NONE, BUTTON_UP,   BUTTON_DOWN, BUTTON_NONE}
    };

    static signed char prev_scroll_lines = -1;
    static signed char direction = 0;
    static unsigned char count = 0;
    static long next_backlight_on = 0;

    signed int new_scroll_lines;
    signed int scroll_dir;

    disable_scrollstrip_interrupts();

    /* read GPIO6 and GPIO7 state*/
    new_scroll_lines = (GPIO_READ >> 6) & 0x03;
    
    if ( prev_scroll_lines == -1 )
    {
        prev_scroll_lines = new_scroll_lines;
        ack_scrollstrip_interrupt();
        enable_scrollstrip_interrupts();
        return;
    }

    scroll_dir = scroll_state[prev_scroll_lines][new_scroll_lines];
    prev_scroll_lines = new_scroll_lines;
    
    if (direction != scroll_dir)
    {
        /* direction reversal */
        direction = scroll_dir;
        count = 0;
        ack_scrollstrip_interrupt();
        enable_scrollstrip_interrupts();
        return;
    }

    /* poke backlight */
    if (TIME_AFTER(current_tick, next_backlight_on))
    {
        backlight_on();
        reset_poweroff_timer();
        next_backlight_on = current_tick + HZ/4;
    }

    if (++count < SLIDER_BASE_SENSITIVITY)
    {
        ack_scrollstrip_interrupt();
        enable_scrollstrip_interrupts();
        return;
    }

    count = 0;

    /* post scrollstrip event to the button queue */
    if (queue_empty(&button_queue))
        queue_post(&button_queue, scroll_dir, 0);

    ack_scrollstrip_interrupt();
    enable_scrollstrip_interrupts();
}

/* register interrupt service routine for scrollstrip lines */
void GPI6(void) __attribute__ ((alias("scrollstrip_isr")));
void GPI7(void) __attribute__ ((alias("scrollstrip_isr")));

void button_init_device(void)
{
    /* GPIO56 (PLAY) general input
     * GPIO45 (ENTER) general input
     * GPIO41 (MENU) dual function pin shared with audio serial data
     *
     * GPIO6, GPIO7 scrollstrip lines
     * GPIO31 scrollstrip enable
     */

    or_l((1<<24)|(1<<13),&GPIO1_FUNCTION);
    and_l(~((1<<24)|(1<<13)),&GPIO1_ENABLE);

    or_l((1<<31)|(1<<7)|(1<<6),&GPIO_FUNCTION);
    and_l(~((1<<7)|(1<<6)),&GPIO_ENABLE);

    /* scrollstrip enable active low */
    and_l(~(1<<31),&GPIO_OUT);
    or_l((1<<31),&GPIO_ENABLE);

    /* GPI6, GPI7 interrupt level 4.0 */
    or_l((4<<28)|(4<<24), &INTPRI5);

    enable_scrollstrip_interrupts();
}

bool button_hold(void)
{
    /* GPIO51 active low */
    return (GPIO1_READ & (1<<19))?false:true;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    int data = 0;
    static bool hold_button = false;

    bool hold_button_old;


    /* read hold buttons status */
    hold_button_old = hold_button;
    hold_button = button_hold();
    
#ifndef BOOTLOADER
    /* Only main hold affects backlight */
    if (hold_button != hold_button_old)
    {
        backlight_hold_changed(hold_button);

        if ( hold_button )
            disable_scrollstrip_interrupts();
        else
            enable_scrollstrip_interrupts();
    }
#endif

    /* Skip if main hold is active */
    if (!hold_button)
    {
        data = adc_scan(ADC_BUTTONS);

        if (data < 800) /* middle */
        {
            if (data < 450)
            {
                if (data > 250)
                    btn |= BUTTON_FF;
            }
            else /* 800 - 450 */
            {
                if (data > 600)
                    btn |= BUTTON_REW;
            }
        }
        else /* data > 800 */
        {
            if (data < 1150)
                if (data > 950)
                    btn |= BUTTON_REC;
        }

        /* Handle GPIOs buttons
         *
         * GPIO56 active high PLAY/PAUSE/ON
         * GPIO45 active low ENTER
         * GPIO41 active low MENU
         */
 
       data = GPIO1_READ;
   
       if (data & (1<<24))
           btn |= BUTTON_PLAY;

       if (!(data & (1<<13)))
           btn |= BUTTON_ENTER;

       if (!(data & (1<<9)))
           btn |= BUTTON_MENU;

    } /* !button_hold() */

    return btn;
}
