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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include "adc.h"
#include "serial.h"
#include "power.h"
#include "system.h"
#include "powermgmt.h"

#define WHEEL_FAST_OFF_TIMEOUT   250000 /* timeout for acceleration = 250ms */
#define WHEEL_REPEAT_TIMEOUT     250000 /* timeout for button repeat = 250ms */
#define WHEEL_UNTOUCH_TIMEOUT    150000 /* timeout for untouching wheel = 100ms */
#define WHEELCLICKS_PER_ROTATION     96 /* wheelclicks per full rotation */

/* This amount of clicks is needed for at least scrolling 1 item. Choose small values 
 * to have high sensitivity but few precision, choose large values to have less 
 * sensitivity and good precision. */
#if defined(IPOD_NANO)
#define WHEEL_SENSITIVITY 6 /* iPod nano has smaller wheel, lower sensitivity needed */
#else
#define WHEEL_SENSITIVITY 4 /* default sensitivity */
#endif

int  old_wheel_value  = -1;
int  new_wheel_value  = 0;
int  repeat           = 0;
int  wheel_delta      = 0;
bool wheel_is_touched = false;
unsigned int  accumulated_wheel_delta = 0;
unsigned int  wheel_repeat            = 0;
unsigned int  wheel_velocity          = 0;
unsigned long last_wheel_usec         = 0;

/* Variable to use for setting button status in interrupt handler */
int int_btn = BUTTON_NONE;
#ifdef HAVE_WHEEL_POSITION
    static int wheel_position = -1;
    static bool send_events = true;
#endif

static void opto_i2c_init(void)
{
    int i, curr_value;

    /* wait for value to settle */
    i = 1000;
    curr_value = (inl(0x7000c104) << 16) >> 24;
    while (i > 0)
    {
        int new_value = (inl(0x7000c104) << 16) >> 24;

        if (new_value != curr_value) {
            i = 10000;
            curr_value = new_value;
        }
        else {
            i--;
        }
    }

    GPIOB_OUTPUT_VAL |= 0x10;
    DEV_EN |= 0x10000;
    DEV_RS |= 0x10000;
    udelay(5);
    DEV_RS &= ~0x10000; /* finish reset */

    outl(0xffffffff, 0x7000c120);
    outl(0xffffffff, 0x7000c124);
    outl(0xc00a1f00, 0x7000c100);
    outl(0x1000000, 0x7000c104);
}

static inline int ipod_4g_button_read(void)
{
    int whl = -1;
    
    /* The ipodlinux source had a udelay(250) here, but testing has shown that
       it is not needed - tested on Nano, Color/Photo and Video. */
    /* udelay(250);*/

    int btn = BUTTON_NONE;
    unsigned reg = 0x7000c104;
    if ((inl(0x7000c104) & 0x4000000) != 0) 
    {
        unsigned status = inl(0x7000c140);

        reg = reg + 0x3C;      /* 0x7000c140 */
        outl(0x0, 0x7000c140); /* clear interrupt status? */

        if ((status & 0x800000ff) == 0x8000001a) 
        {
            if (status & 0x100)
                btn |= BUTTON_SELECT;
            if (status & 0x200)
                btn |= BUTTON_RIGHT;
            if (status & 0x400)
                btn |= BUTTON_LEFT;
            if (status & 0x800)
                btn |= BUTTON_PLAY;
            if (status & 0x1000)
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
                        wheel_velocity = (31 * wheel_velocity + v) / 32;
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
        else if (status == 0xffffffff) 
        {
            opto_i2c_init();
        }
    }

    if ((inl(reg) & 0x8000000) != 0) 
    {
        outl(0xffffffff, 0x7000c120);
        outl(0xffffffff, 0x7000c124);
    }
    /* Save the new absolute wheel position */
#ifdef HAVE_WHEEL_POSITION
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

void ipod_4g_button_int(void)
{
    CPU_HI_INT_CLR = I2C_MASK;
    /* The following delay was 250 in the ipodlinux source, but 50 seems to 
       work fine - tested on Nano, Color/Photo and Video. */
    udelay(50); 
    outl(0x0, 0x7000c140); 
    int_btn = ipod_4g_button_read();
    outl(inl(0x7000c104) | 0xC000000, 0x7000c104);
    outl(0x400a1f00, 0x7000c100);

    GPIOB_OUTPUT_VAL |= 0x10;
    CPU_INT_EN = 0x40000000;
    CPU_HI_INT_EN = I2C_MASK;
}

void button_init_device(void)
{
    opto_i2c_init();
    
    /* hold button - enable as input */
    GPIOA_ENABLE |= 0x20;
    GPIOA_OUTPUT_EN &= ~0x20; 
    
    /* hold button - set interrupt levels */
    GPIOA_INT_LEV = ~(GPIOA_INPUT_VAL & 0x20);
    GPIOA_INT_CLR = GPIOA_INT_STAT & 0x20;

    /* enable interrupts */
    GPIOA_INT_EN = 0x20;
    
    /* unmask interrupt */
    CPU_INT_EN = 0x40000000;
    CPU_HI_INT_EN = I2C_MASK;
}

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
        backlight_hold_changed(hold_button);

    /* The int_btn variable is set in the button interrupt handler */
    return int_btn;
}

bool button_hold(void)
{
    return (GPIOA_INPUT_VAL & 0x20)?false:true;
}

bool headphones_inserted(void)
{
    return (GPIOA_INPUT_VAL & 0x80)?true:false;
}
