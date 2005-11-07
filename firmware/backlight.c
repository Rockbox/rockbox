/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include <stdlib.h>
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "i2c.h"
#include "debug.h"
#include "rtc.h"
#include "usb.h"
#include "power.h"
#include "system.h"
#include "timer.h"

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

#if defined(CONFIG_BACKLIGHT) && !defined(BOOTLOADER)

const char backlight_timeout_value[19] =
{
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 45, 60, 90
};

#define BACKLIGHT_ON 1
#define BACKLIGHT_OFF 2
#define REMOTE_BACKLIGHT_ON 3
#define REMOTE_BACKLIGHT_OFF 4
#define BACKLIGHT_UNBOOST_CPU 5

static void backlight_thread(void);
static long backlight_stack[DEFAULT_STACK_SIZE/sizeof(long)];
static const char backlight_thread_name[] = "backlight";
static struct event_queue backlight_queue;

static bool charger_was_inserted = 0;
static bool backlight_on_when_charging = 0;

static int backlight_timer;
static unsigned int backlight_timeout = 5;
#ifdef HAVE_REMOTE_LCD
static int remote_backlight_timer;
static unsigned int remote_backlight_timeout = 5;
#endif

#if CONFIG_BACKLIGHT == BL_IRIVER
/* backlight fading */
#define BL_PWM_INTERVAL 5000  /* Cycle interval in µs */
#define BL_PWM_COUNT    100
static const char backlight_fade_value[8] = { 0, 1, 2, 4, 6, 8, 10, 20 };
static int fade_in_count = 1;
static int fade_out_count = 4;

static bool bl_timer_active = false;
static int bl_dim_current = BL_PWM_COUNT;
static int bl_dim_target  = BL_PWM_COUNT;
static int bl_pwm_counter = 0;
static volatile int bl_cycle_counter = 0;
static enum {DIM_STATE_START, DIM_STATE_MAIN} bl_dim_state = DIM_STATE_START;

static void backlight_isr(void)
{
    int timer_period;
    bool idle = false;
    
    timer_period = CPU_FREQ / 1000 * BL_PWM_INTERVAL / 1000;
    switch (bl_dim_state) 
    {
      /* New cycle */
      case DIM_STATE_START:
        bl_pwm_counter = 0;
        bl_cycle_counter++;
        
        if (bl_dim_current > 0 && bl_dim_current < BL_PWM_COUNT)
        {
            and_l(~0x00020000, &GPIO1_OUT);
            bl_pwm_counter = bl_dim_current;
            timer_period = timer_period * bl_pwm_counter / BL_PWM_COUNT;
            bl_dim_state = DIM_STATE_MAIN;
        } 
        else
        {
            if (bl_dim_current)
                and_l(~0x00020000, &GPIO1_OUT);
            else
                or_l(0x00020000, &GPIO1_OUT);

            if (bl_dim_current == bl_dim_target)
                idle = true;
        }
        
        break ;
        
      /* Dim main screen */
      case DIM_STATE_MAIN:
        or_l(0x00020000, &GPIO1_OUT);
        bl_dim_state = DIM_STATE_START;
        timer_period = timer_period * (BL_PWM_COUNT - bl_pwm_counter) / BL_PWM_COUNT;
        break ;
    }

    if ((bl_dim_target > bl_dim_current) && (bl_cycle_counter >= fade_in_count))
    {
        bl_dim_current++;
        bl_cycle_counter = 0;
    }

    if ((bl_dim_target < bl_dim_current) && (bl_cycle_counter >= fade_out_count))
    {
        bl_dim_current--;
        bl_cycle_counter = 0;
    }

    if (idle) 
    {
        queue_post(&backlight_queue, BACKLIGHT_UNBOOST_CPU, NULL);
        timer_unregister();
        bl_timer_active = false;
    }
    else
        timer_set_period(timer_period);
}

static void backlight_switch(void)
{
    if (bl_dim_target > (BL_PWM_COUNT/2))
    {
        and_l(~0x00020000, &GPIO1_OUT);
        bl_dim_current = BL_PWM_COUNT;
    }
    else
    {
        or_l(0x00020000, &GPIO1_OUT);
        bl_dim_current = 0;
    }
}

static void backlight_release_timer(void)
{
    cpu_boost(false);
    timer_unregister();
    bl_timer_active = false;
    backlight_switch();
}

static void backlight_dim(int value)
{
    /* protect from extraneous calls with the same target value */
    if (value == bl_dim_target)
        return;

    bl_dim_target = value;

    if (bl_timer_active)
        return ;

    if (timer_register(0, backlight_release_timer, 1, 0, backlight_isr))
    {
        /* Prevent cpu frequency changes while dimming. */
        cpu_boost(true);
        bl_timer_active = true;
    }
    else
        backlight_switch();
}

void backlight_set_fade_in(int index)
{
    fade_in_count = backlight_fade_value[index];
}

void backlight_set_fade_out(int index)
{
    fade_out_count = backlight_fade_value[index];
}
#endif

static void __backlight_off(void)
{
#if CONFIG_BACKLIGHT == BL_IRIVER
    if (fade_out_count > 0)
        backlight_dim(0);
    else
    {
        bl_dim_target = bl_dim_current = 0;
        or_l(0x00020000, &GPIO1_OUT);
    }
#elif CONFIG_BACKLIGHT == BL_RTC
    /* Disable square wave */
    rtc_write(0x0a, rtc_read(0x0a) & ~0x40);
#elif CONFIG_BACKLIGHT == BL_PA14_LO /* Player */
    and_b(~0x40, &PAIORH); /* let it float (up) */
#elif CONFIG_BACKLIGHT == BL_PA14_HI /* Ondio */
    and_b(~0x40, &PADRH); /* drive it low */
#elif CONFIG_BACKLIGHT == BL_GMINI
    P1 &= ~0x10;
#elif CONFIG_BACKLIGHT == BL_IPOD4G
   /* fades backlight off on 4g */
   outl(inl(0x70000084) & ~0x2000000, 0x70000084);
   outl(0x80000000, 0x7000a010);
#endif
}

static void __backlight_on(void)
{
#if CONFIG_BACKLIGHT == BL_IRIVER
    if (fade_in_count > 0)
        backlight_dim(BL_PWM_COUNT);
    else
    {
        bl_dim_target = bl_dim_current = BL_PWM_COUNT;
        and_l(~0x00020000, &GPIO1_OUT);
    }
#elif CONFIG_BACKLIGHT == BL_RTC
    /* Enable square wave */
    rtc_write(0x0a, rtc_read(0x0a) | 0x40);
#elif CONFIG_BACKLIGHT == BL_PA14_LO /* Player */
    and_b(~0x40, &PADRH); /* drive and set low */
    or_b(0x40, &PAIORH);
#elif CONFIG_BACKLIGHT == BL_PA14_HI /* Ondio */
    or_b(0x40, &PADRH); /* drive it high */
#elif CONFIG_BACKLIGHT == BL_GMINI
    P1 |= 0x10;
#elif CONFIG_BACKLIGHT == BL_IPOD4G
    /* brightness full */
    outl(0x80000000 | (0xff << 16), 0x7000a010);

    /* set port b bit 3 on */
    outl(((0x100 | 1) << 3), 0x6000d824);
#endif
}

void backlight_thread(void)
{
    struct event ev;
    
    while(1)
    {
        queue_wait(&backlight_queue, &ev);
        switch(ev.id)
        {
#ifdef HAVE_REMOTE_LCD
            case REMOTE_BACKLIGHT_ON:
                remote_backlight_timer =
                    HZ*backlight_timeout_value[remote_backlight_timeout];

                /* Backlight == OFF in the setting? */
                if(remote_backlight_timer < 0)
                {
                    remote_backlight_timer = 0; /* Disable the timeout */
                    lcd_remote_backlight_off();
                }
                else 
                {
                    lcd_remote_backlight_on();
                }
                break;

            case REMOTE_BACKLIGHT_OFF:
                lcd_remote_backlight_off();
                break;
                
#endif
            case BACKLIGHT_ON:
                if( backlight_on_when_charging && charger_inserted() )
                {
                    /* Forcing to zero keeps the lights on */
                    backlight_timer = 0;
                }
                else
                {
                    backlight_timer = HZ*backlight_timeout_value[backlight_timeout];
                }

                if(backlight_timer < 0) /* Backlight == OFF in the setting? */
                {
                    backlight_timer = 0; /* Disable the timeout */
                    __backlight_off();
                }
                else 
                {
                    __backlight_on();
                }
                break;
                
            case BACKLIGHT_OFF:
                __backlight_off();
                break;

#if CONFIG_BACKLIGHT == BL_IRIVER
            case BACKLIGHT_UNBOOST_CPU:
                cpu_boost(false);
                break;
#endif

            case SYS_USB_CONNECTED:
                /* Tell the USB thread that we are safe */
                DEBUGF("backlight_thread got SYS_USB_CONNECTED\n");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                break;

            case SYS_USB_DISCONNECTED:
                usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
                break;
        }
    }
}

void backlight_on(void)
{
    queue_post(&backlight_queue, BACKLIGHT_ON, NULL);
}

void backlight_off(void)
{
    queue_post(&backlight_queue, BACKLIGHT_OFF, NULL);
}

#ifdef HAVE_REMOTE_LCD
void remote_backlight_on(void)
{
    queue_post(&backlight_queue, REMOTE_BACKLIGHT_ON, NULL);
}

void remote_backlight_off(void)
{
    queue_post(&backlight_queue, REMOTE_BACKLIGHT_OFF, NULL);
}

void remote_backlight_set_timeout(int index)
{
    if((unsigned)index >= sizeof(backlight_timeout_value))
        /* if given a weird value, use 0 */
        index=0;
    remote_backlight_timeout = index; /* index in the backlight_timeout_value table */
    remote_backlight_on();
}
#endif

int backlight_get_timeout(void)
{
    return backlight_timeout;
}

void backlight_set_timeout(int index)
{
    if((unsigned)index >= sizeof(backlight_timeout_value))
        /* if given a weird value, use 0 */
        index=0;
    backlight_timeout = index; /* index in the backlight_timeout_value table */
    backlight_on();
}

#ifdef HAVE_CHARGE_CTRL
bool backlight_get_on_when_charging(void)
{
    return backlight_on_when_charging;
}
#endif

void backlight_set_on_when_charging(bool yesno)
{
    backlight_on_when_charging = yesno;
    backlight_on();
}

void backlight_tick(void)
{
    bool charger_is_inserted = charger_inserted();
    if( backlight_on_when_charging &&
        (charger_was_inserted != charger_is_inserted) )
    {
        backlight_on();
    }
    charger_was_inserted = charger_is_inserted;
    
    if(backlight_timer)
    {
        backlight_timer--;
        if(backlight_timer == 0)
        {
            backlight_off();
        }
    }
#ifdef HAVE_REMOTE_LCD
    if(remote_backlight_timer)
    {
        remote_backlight_timer--;
        if(remote_backlight_timer == 0)
        {
            remote_backlight_off();
        }
    }
#endif
}

void backlight_init(void)
{
    queue_init(&backlight_queue);
    create_thread(backlight_thread, backlight_stack,
                  sizeof(backlight_stack), backlight_thread_name);
                  
#if CONFIG_BACKLIGHT == BL_IRIVER
    or_l(0x00020000, &GPIO1_ENABLE);
    or_l(0x00020000, &GPIO1_FUNCTION);
    and_l(~0x00020000, &GPIO1_OUT);  /* Start with the backlight ON */
#elif CONFIG_BACKLIGHT == BL_PA14_LO || CONFIG_BACKLIGHT == BL_PA14_HI
    PACR1 &= ~0x3000;    /* Set PA14 (backlight control) to GPIO */
    or_b(0x40, &PAIORH); /* ..and output */
#elif CONFIG_BACKLIGHT == BL_GMINI
    P1CON |= 0x10; /* P1.4 C-MOS output mode */
#endif    
    backlight_on();
#ifdef HAVE_REMOTE_LCD
    remote_backlight_on();
#endif
}

#else /* no backlight, empty dummy functions */

void backlight_init(void) {}
void backlight_on(void) {}
void backlight_off(void) {}
void backlight_tick(void) {}
int  backlight_get_timeout(void) {return 0;}
void backlight_set_timeout(int index) {(void)index;}
void backlight_set_on_when_charging(bool yesno) {(void)yesno;}
#ifdef HAVE_REMOTE_LCD
void remote_backlight_on(void) {}
void remote_backlight_off(void) {}
void remote_backlight_set_timeout(int index) {(void)index;}
#endif
#endif /* #ifdef CONFIG_BACKLIGHT */

