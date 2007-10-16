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
#include "button.h"
#include "timer.h"
#include "backlight.h"

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
#include "lcd.h" /* for lcd_enable() and lcd_sleep() */
#endif
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#ifndef SIMULATOR
#include "backlight-target.h"
#endif

#ifdef SIMULATOR
/* TODO: find a better way to do it but we need a kernel thread somewhere to
   handle this */
extern void screen_dump(void);

static inline void __backlight_on(void)
{
    sim_backlight(100);
}

static inline void __backlight_off(void)
{
    sim_backlight(0);
}

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
static inline void __backlight_set_brightness(int val)
{
    (void)val;
}
#endif

#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
static inline void __buttonlight_set_brightness(int val)
{
    (void)val;
}
#endif

#endif /* SIMULATOR */

#if defined(HAVE_BACKLIGHT) && !defined(BOOTLOADER)

const signed char backlight_timeout_value[19] =
{
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 45, 60, 90
};

#define BACKLIGHT_ON 1
#define BACKLIGHT_OFF 2
#define REMOTE_BACKLIGHT_ON 3
#define REMOTE_BACKLIGHT_OFF 4
#define BACKLIGHT_UNBOOST_CPU 5
#ifdef HAVE_LCD_SLEEP
#define LCD_SLEEP 6
#endif
#ifdef HAVE_BUTTON_LIGHT
#define BUTTON_LIGHT_ON 7
#define BUTTON_LIGHT_OFF 8
#endif

static void backlight_thread(void);
static long backlight_stack[DEFAULT_STACK_SIZE/sizeof(long)];
static const char backlight_thread_name[] = "backlight";
static struct event_queue backlight_queue NOCACHEBSS_ATTR;

static int backlight_timer;
static int backlight_timeout;
static int backlight_timeout_normal = 5*HZ;
#if CONFIG_CHARGING
static int backlight_timeout_plugged = 5*HZ;
#endif
#ifdef HAS_BUTTON_HOLD
static int backlight_on_button_hold = 0;
#endif

#ifdef HAVE_BUTTON_LIGHT
static int buttonlight_timer;
static int buttonlight_timeout = 5*HZ;

/* internal interface */
static void _buttonlight_on(void)
{
#ifndef SIMULATOR
#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
    __buttonlight_dim(false);
#else
    __buttonlight_on();
#endif
#endif
}

void _buttonlight_off(void)
{
#ifndef SIMULATOR
#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
    if(buttonlight_timeout>0)
        __buttonlight_dim(true);
    else
#endif
        __buttonlight_off();
#endif
}

/* Update state of buttonlight according to timeout setting */
static void buttonlight_update_state(void)
{
    buttonlight_timer = buttonlight_timeout;

    /* Buttonlight == OFF in the setting? */
    if (buttonlight_timer < 0)
    {
        buttonlight_timer = 0; /* Disable the timeout */
        _buttonlight_off();
    }
    else
        _buttonlight_on();
}

/* external interface */
void buttonlight_on(void)
{
    queue_remove_from_head(&backlight_queue, BUTTON_LIGHT_ON);
    queue_post(&backlight_queue, BUTTON_LIGHT_ON, 0);
}

void buttonlight_off(void)
{
    queue_post(&backlight_queue, BUTTON_LIGHT_OFF, 0);
}

void buttonlight_set_timeout(int index)
{
    if((unsigned)index >= sizeof(backlight_timeout_value))
        /* if given a weird value, use default */
        index = 6;
    buttonlight_timeout = HZ * backlight_timeout_value[index];
    buttonlight_update_state();
}

#endif

#ifdef HAVE_REMOTE_LCD
static int remote_backlight_timer;
static int remote_backlight_timeout;
static int remote_backlight_timeout_normal = 5*HZ;
#if CONFIG_CHARGING
static int remote_backlight_timeout_plugged = 5*HZ;
#endif
#ifdef HAS_REMOTE_BUTTON_HOLD
static int remote_backlight_on_button_hold = 0;
#endif
#endif

#ifdef HAVE_LCD_SLEEP
const signed char lcd_sleep_timeout_value[10] =
{
    -1, 0, 5, 10, 15, 20, 30, 45, 60, 90
};
static int lcd_sleep_timer;
static int lcd_sleep_timeout = 10*HZ;
#endif

#if defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR)
/* backlight fading */
#define BL_PWM_INTERVAL 5000  /* Cycle interval in s */
#define BL_PWM_COUNT    100
static const char backlight_fade_value[8] = { 0, 1, 2, 4, 6, 8, 10, 20 };
static int fade_in_count = 1;
static int fade_out_count = 4;

static bool bl_timer_active = false;
static int bl_dim_current = 0;
static int bl_dim_target  = 0;
static int bl_pwm_counter = 0;
static volatile int bl_cycle_counter = 0;
static enum {DIM_STATE_START, DIM_STATE_MAIN} bl_dim_state = DIM_STATE_START;

static void backlight_isr(void)
{
    int timer_period;
    bool idle = false;

    timer_period = TIMER_FREQ / 1000 * BL_PWM_INTERVAL / 1000;
    switch (bl_dim_state)
    {
      /* New cycle */
      case DIM_STATE_START:
        bl_pwm_counter = 0;
        bl_cycle_counter++;

        if (bl_dim_current > 0 && bl_dim_current < BL_PWM_COUNT)
        {
            __backlight_on();
            bl_pwm_counter = bl_dim_current;
            timer_period = timer_period * bl_pwm_counter / BL_PWM_COUNT;
            bl_dim_state = DIM_STATE_MAIN;
        }
        else
        {
            if (bl_dim_current)
                __backlight_on();
            else
                __backlight_off();
            if (bl_dim_current == bl_dim_target)
                idle = true;
        }

        break ;

      /* Dim main screen */
      case DIM_STATE_MAIN:
        __backlight_off();
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
#ifdef CPU_COLDFIRE
        queue_post(&backlight_queue, BACKLIGHT_UNBOOST_CPU, 0);
#endif
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
        __backlight_on();
        bl_dim_current = BL_PWM_COUNT;
    }
    else
    {
        __backlight_off();
        bl_dim_current = 0;
    }
}

static void backlight_release_timer(void)
{
#ifdef CPU_COLDFIRE
    cpu_boost(false);
#endif
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

    if (timer_register(0, backlight_release_timer, 2, 0, backlight_isr))
    {
#ifdef CPU_COLDFIRE
        /* Prevent cpu frequency changes while dimming. */
        cpu_boost(true);
#endif
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
#endif /* defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR) */

static void _backlight_on(void)
{
#if defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR)
    if (fade_in_count > 0)
        backlight_dim(BL_PWM_COUNT);
    else
    {
        bl_dim_target = bl_dim_current = BL_PWM_COUNT;
        __backlight_on();
    }
#elif defined(HAVE_BACKLIGHT_SET_FADING) && !defined(SIMULATOR)
    /* call the enable from here - it takes longer than the disable */
    lcd_enable(true);
    __backlight_dim(false);
#else
    __backlight_on();
#endif
#ifdef HAVE_LCD_SLEEP
    lcd_sleep_timer = 0; /* LCD should be awake already */
#endif
}

static void _backlight_off(void)
{
#if defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR)
    if (fade_out_count > 0)
        backlight_dim(0);
    else
    {
        bl_dim_target = bl_dim_current = 0;
        __backlight_off();
    }
#elif defined(HAVE_BACKLIGHT_SET_FADING) && !defined(SIMULATOR)
    __backlight_dim(true);
#else
    __backlight_off();
#endif

#ifdef HAVE_LCD_SLEEP
    /* Start LCD sleep countdown */
    if (lcd_sleep_timeout < 0)
    {
        lcd_sleep_timer = 0; /* Setting == Always */
        lcd_sleep();
    }
    else
        lcd_sleep_timer = lcd_sleep_timeout;
#endif
}

#ifdef HAVE_REMOTE_LCD
#ifdef SIMULATOR
static void __remote_backlight_on(void)
{
    sim_remote_backlight(100);
}

static void __remote_backlight_off(void)
{
    sim_remote_backlight(0);
}
#endif /* SIMULATOR */
#endif /* HAVE_REMOTE_LCD */

/* Update state of backlight according to timeout setting */
static void backlight_update_state(void)
{
#ifdef HAS_BUTTON_HOLD
    if (button_hold() && (backlight_on_button_hold != 0))
        backlight_timeout = (backlight_on_button_hold == 2) ? 0 : -1;
        /* always on or always off */
    else
#endif
#if CONFIG_CHARGING
        if (charger_inserted()
#ifdef HAVE_USB_POWER
                || usb_powered()
#endif
                )
            backlight_timeout = backlight_timeout_plugged;
        else
#endif
            backlight_timeout = backlight_timeout_normal;

    /* Backlight == OFF in the setting? */
    if (backlight_timeout < 0)
    {
        backlight_timer = 0; /* Disable the timeout */
        _backlight_off();
    }
    else
    {
        backlight_timer = backlight_timeout;
        _backlight_on();
    }
}

#ifdef HAVE_REMOTE_LCD
/* Update state of remote backlight according to timeout setting */
static void remote_backlight_update_state(void)
{
#ifdef HAS_REMOTE_BUTTON_HOLD
    if (remote_button_hold() && (remote_backlight_on_button_hold != 0))
        remote_backlight_timeout = (remote_backlight_on_button_hold == 2)
                                 ? 0 : -1;  /* always on or always off */
    else
#endif
#if CONFIG_CHARGING
        if (charger_inserted()
#ifdef HAVE_USB_POWER
            || usb_powered()
#endif
            )
            remote_backlight_timeout = remote_backlight_timeout_plugged;
        else
#endif
            remote_backlight_timeout = remote_backlight_timeout_normal;

    /* Backlight == OFF in the setting? */
    if (remote_backlight_timeout < 0)
    {
        remote_backlight_timer = 0; /* Disable the timeout */
        __remote_backlight_off();
    }
    else
    {
        remote_backlight_timer = remote_backlight_timeout;
        __remote_backlight_on();
    }
}
#endif /* HAVE_REMOTE_LCD */

void backlight_thread(void)
{
    struct queue_event ev;
    bool locked = false;

    while(1)
    {
        queue_wait(&backlight_queue, &ev);
        switch(ev.id)
        {   /* These events must always be processed */
#if defined(HAVE_BACKLIGHT_PWM_FADING) && defined(CPU_COLDFIRE) \
    && !defined(SIMULATOR)
            case BACKLIGHT_UNBOOST_CPU:
                cpu_boost(false);
                break;
#endif

#if defined(HAVE_REMOTE_LCD) && !defined(SIMULATOR)
            /* Here for now or else the aggressive init messes up scrolling */
            case SYS_REMOTE_PLUGGED:
                lcd_remote_on();
                lcd_remote_update();
                break;

            case SYS_REMOTE_UNPLUGGED:
                lcd_remote_off();
                break;
#endif /* defined(HAVE_REMOTE_LCD) && !defined(SIMULATOR) */
#ifdef SIMULATOR
            /* This one here too for lack of a better place */
            case SYS_SCREENDUMP:
                screen_dump();
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
        if (locked)
            continue;

        switch(ev.id)
        {   /* These events are only processed if backlight isn't locked */
#ifdef HAVE_REMOTE_LCD
            case REMOTE_BACKLIGHT_ON:
                remote_backlight_update_state();
                break;

            case REMOTE_BACKLIGHT_OFF:
                remote_backlight_timer = 0; /* Disable the timeout */
                __remote_backlight_off();
                break;
#endif /* HAVE_REMOTE_LCD */

            case BACKLIGHT_ON:
                backlight_update_state();
                break;

            case BACKLIGHT_OFF:
                backlight_timer = 0; /* Disable the timeout */
                _backlight_off();
                break;

#ifdef HAVE_LCD_SLEEP
            case LCD_SLEEP:
                lcd_sleep();
                break;
#endif
#ifdef HAVE_BUTTON_LIGHT
            case BUTTON_LIGHT_ON:
                buttonlight_update_state();
                break;

            case BUTTON_LIGHT_OFF:
                buttonlight_timer = 0;
                _buttonlight_off();
                break;
#endif

            case SYS_POWEROFF:  /* Lock backlight on poweroff so it doesn't */
                locked = true;      /* go off before power is actually cut. */
                /* fall through */
#if CONFIG_CHARGING
            case SYS_CHARGER_CONNECTED:
            case SYS_CHARGER_DISCONNECTED:
#endif
                backlight_update_state();
#ifdef HAVE_REMOTE_LCD
                remote_backlight_update_state();
#endif
                break;
        }
    } /* end while */
}

static void backlight_tick(void)
{
    if(backlight_timer)
    {
        backlight_timer--;
        if(backlight_timer == 0)
        {
            backlight_off();
        }
    }
#ifdef HAVE_LCD_SLEEP
    else if(lcd_sleep_timer)
    {
        lcd_sleep_timer--;
        if(lcd_sleep_timer == 0)
        {
            /* Queue on bl thread or freeze! */
            queue_post(&backlight_queue, LCD_SLEEP, 0);
        }
    }
#endif /* HAVE_LCD_SLEEP */

#ifdef HAVE_REMOTE_LCD
    if(remote_backlight_timer)
    {
        remote_backlight_timer--;
        if(remote_backlight_timer == 0)
        {
            remote_backlight_off();
        }
    }
#endif /* HAVE_REMOVE_LCD */
#ifdef HAVE_BUTTON_LIGHT
    if (buttonlight_timer)
    {
        buttonlight_timer--;
        if (buttonlight_timer == 0)
        {
            buttonlight_off();
        }
    }
#endif /* HAVE_BUTTON_LIGHT */
}

void backlight_init(void)
{
    queue_init(&backlight_queue, true);

#ifndef SIMULATOR
    if (__backlight_init())
    {
# ifdef HAVE_BACKLIGHT_PWM_FADING
        /* If backlight is already on, don't fade in. */
        bl_dim_current = BL_PWM_COUNT;
        bl_dim_target = BL_PWM_COUNT;
# endif
    }
#endif
    /* Leave all lights as set by the bootloader here. The settings load will
     * call the appropriate backlight_set_*() functions, only changing light
     * status if necessary. */

    create_thread(backlight_thread, backlight_stack,
                  sizeof(backlight_stack), 0, backlight_thread_name
                  IF_PRIO(, PRIORITY_SYSTEM)
                  IF_COP(, CPU));
    tick_add_task(backlight_tick);
}

void backlight_on(void)
{
    queue_remove_from_head(&backlight_queue, BACKLIGHT_ON);
    queue_post(&backlight_queue, BACKLIGHT_ON, 0);
}

void backlight_off(void)
{
    queue_post(&backlight_queue, BACKLIGHT_OFF, 0);
}

/* returns true when the backlight is on OR when it's set to always off */
bool is_backlight_on(void)
{
    if (backlight_timer || backlight_timeout <= 0)
        return true;
    else
        return false;
}

/* return value in ticks; 0 means always on, <0 means always off */
int backlight_get_current_timeout(void)
{
    return backlight_timeout;
}

void backlight_set_timeout(int index)
{
    if((unsigned)index >= sizeof(backlight_timeout_value))
        /* if given a weird value, use default */
        index = 6;
    backlight_timeout_normal = HZ * backlight_timeout_value[index];
    backlight_update_state();
}

#if CONFIG_CHARGING
void backlight_set_timeout_plugged(int index)
{
    if((unsigned)index >= sizeof(backlight_timeout_value))
        /* if given a weird value, use default */
        index = 6;
    backlight_timeout_plugged = HZ * backlight_timeout_value[index];
    backlight_update_state();
}
#endif /* CONFIG_CHARGING */

#ifdef HAS_BUTTON_HOLD
/* Hold button change event handler. */
void backlight_hold_changed(bool hold_button)
{
    if (!hold_button || (backlight_on_button_hold > 0))
        /* if unlocked or override in effect */
        backlight_on();
}

void backlight_set_on_button_hold(int index)
{
    if ((unsigned)index >= 3)
        /* if given a weird value, use default */
        index = 0;

    backlight_on_button_hold = index;
    backlight_update_state();
}
#endif /* HAS_BUTTON_HOLD */

#ifdef HAVE_LCD_SLEEP
void lcd_set_sleep_after_backlight_off(int index)
{
    if ((unsigned)index >= sizeof(lcd_sleep_timeout_value))
        /* if given a weird value, use default */
        index = 3;

    lcd_sleep_timeout = HZ * lcd_sleep_timeout_value[index];

    if (backlight_timer > 0 || backlight_get_current_timeout() == 0)
        /* Timer will be set when bl turns off or bl set to on. */
        return;

    /* Backlight is Off */
    if (lcd_sleep_timeout < 0)
        lcd_sleep_timer = 1; /* Always - sleep next tick */
    else
        lcd_sleep_timer = lcd_sleep_timeout; /* Never, other */
}
#endif /* HAVE_LCD_SLEEP */

#ifdef HAVE_REMOTE_LCD
void remote_backlight_on(void)
{
    queue_post(&backlight_queue, REMOTE_BACKLIGHT_ON, 0);
}

void remote_backlight_off(void)
{
    queue_post(&backlight_queue, REMOTE_BACKLIGHT_OFF, 0);
}

void remote_backlight_set_timeout(int index)
{
    if((unsigned)index >= sizeof(backlight_timeout_value))
        /* if given a weird value, use default */
        index=6;
    remote_backlight_timeout_normal = HZ * backlight_timeout_value[index];
    remote_backlight_update_state();
}

#if CONFIG_CHARGING
void remote_backlight_set_timeout_plugged(int index)
{
    if((unsigned)index >= sizeof(backlight_timeout_value))
        /* if given a weird value, use default */
        index=6;
    remote_backlight_timeout_plugged = HZ * backlight_timeout_value[index];
    remote_backlight_update_state();
}
#endif /* CONFIG_CHARGING */

#ifdef HAS_REMOTE_BUTTON_HOLD
/* Remote hold button change event handler. */
void remote_backlight_hold_changed(bool rc_hold_button)
{
    if (!rc_hold_button || (remote_backlight_on_button_hold > 0))
        /* if unlocked or override */
        remote_backlight_on();
}

void remote_backlight_set_on_button_hold(int index)
{
    if ((unsigned)index >= 3)
        /* if given a weird value, use default */
        index = 0;

    remote_backlight_on_button_hold = index;
    remote_backlight_update_state();
}
#endif /* HAS_REMOTE_BUTTON_HOLD */

/* return value in ticks; 0 means always on, <0 means always off */
int remote_backlight_get_current_timeout(void)
{
    return remote_backlight_timeout;
}

/* returns true when the backlight is on OR when it's set to always off */
bool is_remote_backlight_on(void)
{
    if (remote_backlight_timer != 0 || remote_backlight_timeout <= 0)
        return true;
    else
        return false;
}

#endif /* HAVE_REMOTE_LCD */

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
void backlight_set_brightness(int val)
{
    if (val < MIN_BRIGHTNESS_SETTING)
        val = MIN_BRIGHTNESS_SETTING;
    else if (val > MAX_BRIGHTNESS_SETTING)
        val = MAX_BRIGHTNESS_SETTING;

    __backlight_set_brightness(val);
}
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
void buttonlight_set_brightness(int val)
{
    if (val < MIN_BRIGHTNESS_SETTING)
        val = MIN_BRIGHTNESS_SETTING;
    else if (val > MAX_BRIGHTNESS_SETTING)
        val = MAX_BRIGHTNESS_SETTING;

    __buttonlight_set_brightness(val);
}
#endif /* HAVE_BUTTONLIGHT_BRIGHTNESS */

#else /* !defined(HAVE_BACKLIGHT) || defined(BOOTLOADER)
    -- no backlight, empty dummy functions */

#if defined(BOOTLOADER) && defined(HAVE_BACKLIGHT)
void backlight_init(void)
{
    (void)__backlight_init();
    __backlight_on();
}
#endif

void backlight_on(void) {}
void backlight_off(void) {}
void buttonlight_on(void) {}
void backlight_set_timeout(int index) {(void)index;}
bool is_backlight_on(void) {return true;}
#ifdef HAVE_REMOTE_LCD
void remote_backlight_on(void) {}
void remote_backlight_off(void) {}
void remote_backlight_set_timeout(int index) {(void)index;}
bool is_remote_backlight_on(void) {return true;}
#endif /* HAVE_REMOTE_LCD */
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
void backlight_set_brightness(int val) { (void)val; }
#endif
#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
void buttonlight_set_brightness(int val) { (void)val; }
#endif
#endif /* defined(HAVE_BACKLIGHT) && !defined(BOOTLOADER) */
