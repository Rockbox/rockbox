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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
#include "serial.h"
#include "power.h"
#include "powermgmt.h"
#ifdef IPOD_NANO2G
#include "pmu-target.h"
#endif
#ifdef IPOD_6G
#include "pmu-target.h"
#include "clocking-s5l8702.h"
#endif

#define WHEEL_FAST_OFF_TIMEOUT   250000 /* timeout for acceleration = 250ms */
#define WHEEL_REPEAT_TIMEOUT     250000 /* timeout for button repeat = 250ms */
#define WHEEL_UNTOUCH_TIMEOUT    150000 /* timeout for untouching wheel = 150ms */

#ifdef CPU_PP
#define CLICKWHEEL_DATA   (*(volatile unsigned long*)(0x7000c140))
#elif CONFIG_CPU==S5L8701 || CONFIG_CPU==S5L8702
#define CLICKWHEEL_DATA   WHEELRX
#else
#error CPU architecture not supported!
#endif

#define WHEELCLICKS_PER_ROTATION     96 /* wheelclicks per full rotation */

/* This amount of clicks is needed for at least scrolling 1 item. Choose small values 
 * to have high sensitivity but few precision, choose large values to have less 
 * sensitivity and good precision. */
#if defined(IPOD_NANO) || defined(IPOD_NANO2G)
#define WHEEL_SENSITIVITY 6 /* iPod nano has smaller wheel, lower sensitivity needed */
#else
#define WHEEL_SENSITIVITY 4 /* default sensitivity */
#endif

int  old_wheel_value  = -1;
int  new_wheel_value  = 0;
static int  repeat           = 0;
int  wheel_delta      = 0;
bool wheel_is_touched = false;
unsigned int  accumulated_wheel_delta = 0;
static unsigned int  wheel_repeat            = 0;
unsigned int  wheel_velocity          = 0;
static unsigned long last_wheel_usec         = 0;

/* Variable to use for setting button status in interrupt handler */
static int int_btn = BUTTON_NONE;
#ifdef HAVE_WHEEL_POSITION
    static int wheel_position = -1;
    static bool send_events = true;
#endif

#if CONFIG_CPU==S5L8701 || CONFIG_CPU==S5L8702
static struct semaphore button_init_wakeup;
#endif

#ifdef CPU_PP
static void opto_i2c_init(void)
{
    DEV_EN |= DEV_OPTO;
    DEV_RS |= DEV_OPTO;
    udelay(5);
    DEV_RS &= ~DEV_OPTO; /* finish reset */
    DEV_INIT1 |= INIT_BUTTONS; /* enable buttons (needed for "hold"-detection) */

    outl(0xc00a1f00, 0x7000c100);
    outl(0x01000000, 0x7000c104);
}
#endif

static inline int ipod_4g_button_read(void)
{
    int whl = -1;
    int btn = BUTTON_NONE;

#ifdef CPU_PP    
    if ((inl(0x7000c104) & 0x04000000) != 0) 
    {
#endif
        unsigned status = CLICKWHEEL_DATA;

        if ((status & 0x800000ff) == 0x8000001a) 
        {
            if (status & 0x00000100)
                btn |= BUTTON_SELECT;
            if (status & 0x00000200)
                btn |= BUTTON_RIGHT;
            if (status & 0x00000400)
                btn |= BUTTON_LEFT;
            if (status & 0x00000800)
                btn |= BUTTON_PLAY;
            if (status & 0x00001000)
                btn |= BUTTON_MENU;
            if (status & 0x40000000) 
            {
                unsigned long usec = USEC_TIMER;
                
                /* Highest wheel = 0x5F, clockwise increases */
                new_wheel_value = (status >> 16) & 0x7f;
                whl = new_wheel_value;
                
                /* switch on backlight (again), reset power-off timer */
                backlight_on();
                reset_poweroff_timer();
                
                /* Check whether the scrollwheel was untouched by accident or by will. */
                /* This is needed because wheel may be untoched very shortly during rotation */
                if ( (!wheel_is_touched) && TIME_AFTER(usec, last_wheel_usec + WHEEL_UNTOUCH_TIMEOUT) )
                {
                    /* wheel has been really untouched -> reset internal variables */
                    old_wheel_value         = -1;
                    wheel_velocity          = 0;
                    accumulated_wheel_delta = 0;
                    wheel_repeat            = BUTTON_NONE;
                }
                else
                {
                    /* wheel was shortly untouched by accident -> leave internal variables */
                    wheel_is_touched = true;
                }

                if (old_wheel_value >= 0)
                {
                    /* This is for later = BUTTON_SCROLL_TOUCH;*/
                    wheel_delta = new_wheel_value - old_wheel_value;
                    unsigned int wheel_keycode = BUTTON_NONE;

                    /* Taking into account wrapping during transition from highest 
                     * to lowest wheel position and back */
                    if      (wheel_delta < -WHEELCLICKS_PER_ROTATION/2)
                        wheel_delta += WHEELCLICKS_PER_ROTATION; /* Forward wrapping case */
                    else if (wheel_delta >  WHEELCLICKS_PER_ROTATION/2)
                        wheel_delta -= WHEELCLICKS_PER_ROTATION; /* Backward wrapping case */
    
                    /* Getting direction and wheel_keycode from wheel_delta.
                     * Need at least some clicks to be sure to avoid haptic fuzziness */
                    if      (wheel_delta >=  WHEEL_SENSITIVITY)
                        wheel_keycode = BUTTON_SCROLL_FWD;
                    else if (wheel_delta <= -WHEEL_SENSITIVITY)
                        wheel_keycode = BUTTON_SCROLL_BACK;
                    else 
                        wheel_keycode = BUTTON_NONE;

                    if (wheel_keycode != BUTTON_NONE)
                    {
                        long v = (usec - last_wheel_usec) & 0x7fffffff;
                        
                        /* undo signedness */
                        wheel_delta = (wheel_delta>0) ? wheel_delta : -wheel_delta;
                        
                        /* add the current wheel_delta */
                        accumulated_wheel_delta += wheel_delta;

                        v = v ? (1000000 * wheel_delta) / v : 0;  /* clicks/sec = 1000000 * clicks/usec */
                        v = (v * 360) / WHEELCLICKS_PER_ROTATION; /* conversion to degree/sec */
                        v = (v<0) ? -v : v;                       /* undo signedness */
            
                        /* some velocity filtering to smooth things out */
                        wheel_velocity = (15 * wheel_velocity + v) / 16;
                        /* limit to 24 bit */
                        wheel_velocity = (wheel_velocity>0xffffff) ? 0xffffff : wheel_velocity;

                        /* assume REPEAT = off */
                        repeat = 0;
                        
                        /* direction reversals must nullify acceleration and accumulator */
                        if (wheel_keycode != wheel_repeat)
                        {
                            wheel_repeat            = wheel_keycode;
                            wheel_velocity          = 0;
                            accumulated_wheel_delta = 0;
                        }
                        /* on same direction REPEAT is assumed when new click is within timeout */
                        else if (TIME_BEFORE(usec, last_wheel_usec + WHEEL_REPEAT_TIMEOUT))
                        {
                            repeat = BUTTON_REPEAT;
                        }
                        /* timeout nullifies acceleration and accumulator */
                        if (TIME_AFTER(usec, last_wheel_usec + WHEEL_FAST_OFF_TIMEOUT))
                        {
                            wheel_velocity          = 0;
                            accumulated_wheel_delta = 0;
                        }

#ifdef HAVE_WHEEL_POSITION
                        if (send_events) 
#endif
                        /* The queue should have no other events when scrolling */
                        if (queue_empty(&button_queue))
                        {
                            /* each WHEEL_SENSITIVITY clicks = scrolling 1 item */
                            accumulated_wheel_delta /= WHEEL_SENSITIVITY;
#ifdef HAVE_SCROLLWHEEL
                            /* use data-format for HAVE_SCROLLWHEEL */
                            /* always use acceleration mode (1<<31) */
                            /* always set message post count to (1<<24) for iPod */
                            /* this way the scrolling is always calculated from wheel_velocity */
                            queue_post(&button_queue, wheel_keycode | repeat, 
                                       (1<<31) | (1 << 24) | wheel_velocity);
                                       
#else
                            queue_post(&button_queue, wheel_keycode | repeat, 
                                       (accumulated_wheel_delta << 16) | new_wheel_value);
#endif
                            accumulated_wheel_delta = 0;
                        }
                        last_wheel_usec = usec;
                        old_wheel_value = new_wheel_value;
                    }
                }
                else
                {
                    /* scrollwheel was touched for the first time after finger lifting */
                    old_wheel_value = new_wheel_value;
                    wheel_is_touched = true;
                }
            }
            else
            {
                /* In this case the finger was lifted from the scrollwheel. */
                wheel_is_touched = false; 
            }

        }
        else if ((status & 0x800000ff) != 0x8000003a &&
                 status != 0xFFFFFFFF)
        {
            udelay(2000);
            outl(inl(0x7000c100) & ~0x60000000, 0x7000c100);
            outl(inl(0x7000c104) | 0x04000000, 0x7000c104);
            outl(inl(0x7000c100) | 0x60000000, 0x7000c100);
        }
#if CONFIG_CPU==S5L8701 || CONFIG_CPU==S5L8702
        else if ((status & 0x8000FFFF) == 0x8000023A)
        {
            if (status & 0x00010000)
                btn |= BUTTON_SELECT;
            if (status & 0x00020000)
                btn |= BUTTON_RIGHT;
            if (status & 0x00040000)
                btn |= BUTTON_LEFT;
            if (status & 0x00080000)
                btn |= BUTTON_PLAY;
            if (status & 0x00100000)
                btn |= BUTTON_MENU;
            semaphore_release(&button_init_wakeup);
        }
#endif

#ifdef CPU_PP    
    }
#endif

#ifdef HAVE_WHEEL_POSITION
    /* Save the new absolute wheel position */
    wheel_position = whl;
#endif
    return btn;
}

#ifdef HAVE_WHEEL_POSITION
int wheel_status(void)
{
    return wheel_position;
}
 
void wheel_send_events(bool send)
{
    send_events = send;
}
#endif

#ifdef CPU_PP
void ipod_4g_button_int(void)
{
    CPU_HI_INT_DIS = I2C_MASK;

    /* The following delay was 250 in the ipodlinux source, but 50 seems to 
       work fine - tested on Nano, Color/Photo and Video. */
    udelay(50);

    int_btn = ipod_4g_button_read();
    
    outl(inl(0x7000c104) | 0x0c000000, 0x7000c104);
    outl(0x400a1f00, 0x7000c100);

    CPU_HI_INT_EN = I2C_MASK;
}

void button_init_device(void)
{
    opto_i2c_init();

    /* fixes first button press being ignored */
#if defined(IPOD_4G) || defined(IPOD_COLOR)
    outl(inl(0x7000c100) & ~0x60000000, 0x7000c100);
    outl(inl(0x7000c104) | 0x04000000, 0x7000c104);
    outl(inl(0x7000c100) | 0x60000000, 0x7000c100);
#endif
    /* hold button - enable as input */
    GPIOA_ENABLE |= 0x20;
    GPIOA_OUTPUT_EN &= ~0x20; 
    
    /* unmask interrupt */
    CPU_INT_EN = HI_MASK;
    CPU_HI_INT_EN = I2C_MASK;
}

bool button_hold(void)
{
    return (GPIOA_INPUT_VAL & 0x20)?false:true;
}

bool headphones_inserted(void)
{
    return (GPIOA_INPUT_VAL & 0x80)?true:false;
}
#else
void INT_WHEEL(void)
{
    int clickwheel_events = WHEELINT;

    /* Clear interrupts */
    if (clickwheel_events & 4) WHEELINT = 4;
    if (clickwheel_events & 2) WHEELINT = 2;
    if (clickwheel_events & 1) WHEELINT = 1;

    int_btn = ipod_4g_button_read();
}

static void s5l_clickwheel_init(void)
{
#if CONFIG_CPU==S5L8701
    PWRCONEXT &= ~1;
    PCON15 = (PCON15 & ~0xFFFF0000) | 0x22220000;
    PUNK15 = 0xF0;
    WHEEL08 = 0x3A980;
    WHEEL00 = 0x280000;
    WHEEL10 = 3;
    PCON10 = (PCON10 & ~0xFF0) | 0x10;
    PDAT10 |= 2;
    WHEELTX = 0x8000023A;
    WHEEL04 |= 1;
    PDAT10 &= ~2;
#elif CONFIG_CPU==S5L8702
    clockgate_enable(CLOCKGATE_CWHEEL, true);
    PCONE = (PCONE & ~0x00ffff00) | 0x00222200;
    WHEEL00 = 0; /* stop s5l8702 controller */
    WHEELINT = 7;
    WHEEL10 = 1;
    WHEEL00 = 0x380000;
    WHEEL08 = 0x20000;
    WHEELTX = 0x8000023A;
    WHEEL04 |= 1;
#endif
}

void button_init_device(void)
{
    semaphore_init(&button_init_wakeup, 1, 0);
#if CONFIG_CPU==S5L8701
    INTMSK |= (1<<26);
#endif
    s5l_clickwheel_init();
    semaphore_wait(&button_init_wakeup, HZ / 10);
}

bool button_hold(void)
{
#if CONFIG_CPU==S5L8701
    bool value = (PDAT14 & (1 << 6)) == 0;
    if (value)
        PCON15 = PCON15 & ~0xffff0000;
    else PCON15 = (PCON15 & ~0xffff0000) | 0x22220000;
    return value;
#elif CONFIG_CPU==S5L8702
    return pmu_holdswitch_locked();
#endif
}

bool headphones_inserted(void)
{
#if CONFIG_CPU==S5L8701
    return ((PDAT14 & (1 << 5)) != 0);
#elif CONFIG_CPU==S5L8702
    return ((PDATA & (1 << 6)) != 0);
#endif
}
#endif

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    static bool hold_button = false;
    bool hold_button_old;

    /* normal buttons */
    hold_button_old = hold_button;
    hold_button = button_hold();

    if (hold_button != hold_button_old)
    {
#ifndef BOOTLOADER
        backlight_hold_changed(hold_button);
#endif
        
        if (hold_button)
        {
#ifdef CPU_PP
            /* lock -> disable wheel sensor */
            DEV_EN &= ~DEV_OPTO;
#elif CONFIG_CPU==S5L8701
            /*pmu_ldo_power_off(1);*/ /* disable clickwheel power supply */
            WHEEL00 = 0;
            WHEEL10 = 0;
            PWRCONEXT |= 1;
#elif CONFIG_CPU==S5L8702
            WHEEL00 = 0;
            PCONE = (PCONE & ~0x00ffff00) | 0x000e0e00;
            clockgate_enable(CLOCKGATE_CWHEEL, false);
#endif
        }
        else
        {
#ifdef CPU_PP
            /* unlock -> enable wheel sensor */
            DEV_EN |= DEV_OPTO;
            opto_i2c_init();
#elif CONFIG_CPU==S5L8701
            /*pmu_ldo_power_on(1);*/ /* enable clickwheel power supply */
            s5l_clickwheel_init();
#elif CONFIG_CPU==S5L8702
            s5l_clickwheel_init();
#endif
        }
    }

    /* The int_btn variable is set in the button interrupt handler */
#ifdef IPOD_ACCESSORY_PROTOCOL
    return int_btn | remote_control_rx();
#else
    return int_btn;
#endif
}
