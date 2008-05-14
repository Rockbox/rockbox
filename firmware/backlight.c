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
#include "lcd.h"

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#ifndef SIMULATOR
#include "backlight-target.h"
#endif

#if !defined(BOOTLOADER)
/* The whole driver should be built */
#define BACKLIGHT_FULL_INIT
#endif

#ifdef SIMULATOR
/* TODO: find a better way to do it but we need a kernel thread somewhere to
   handle this */
extern void screen_dump(void);

static inline void _backlight_on(void)
{
    sim_backlight(100);
}

static inline void _backlight_off(void)
{
    sim_backlight(0);
}

static inline void _backlight_set_brightness(int val)
{
    (void)val;
}

static inline void _buttonlight_on(void)
{
}

static inline void _buttonlight_off(void)
{
}

static inline void _buttonlight_set_brightness(int val)
{
    (void)val;
}
#ifdef HAVE_REMOTE_LCD
static inline void _remote_backlight_on(void)
{
    sim_remote_backlight(100);
}

static inline void _remote_backlight_off(void)
{
    sim_remote_backlight(0);
}
#endif /* HAVE_REMOTE_LCD */

#endif /* SIMULATOR */

#if defined(HAVE_BACKLIGHT) && defined(BACKLIGHT_FULL_INIT)

enum {
    BACKLIGHT_ON,
    BACKLIGHT_OFF,
#ifdef HAVE_REMOTE_LCD
    REMOTE_BACKLIGHT_ON,
    REMOTE_BACKLIGHT_OFF,
#endif
#if defined(_BACKLIGHT_FADE_BOOST) || defined(_BACKLIGHT_FADE_ENABLE)
    BACKLIGHT_FADE_FINISH,
#endif
#ifdef HAVE_LCD_SLEEP
    LCD_SLEEP,
#endif
#ifdef HAVE_BUTTON_LIGHT
    BUTTON_LIGHT_ON,
    BUTTON_LIGHT_OFF,
#endif
#ifdef BACKLIGHT_DRIVER_CLOSE
    BACKLIGHT_QUIT,
#endif
};

static void backlight_thread(void);
static long backlight_stack[DEFAULT_STACK_SIZE/sizeof(long)];
static const char backlight_thread_name[] = "backlight";
static struct event_queue backlight_queue;
#ifdef BACKLIGHT_DRIVER_CLOSE
static struct thread_entry *backlight_thread_p = NULL;
#endif

static int backlight_timer SHAREDBSS_ATTR;
static int backlight_timeout SHAREDBSS_ATTR;
static int backlight_timeout_normal = 5*HZ;
#if CONFIG_CHARGING
static int backlight_timeout_plugged = 5*HZ;
#endif
#ifdef HAS_BUTTON_HOLD
static int backlight_on_button_hold = 0;
#endif

#ifdef HAVE_BUTTON_LIGHT
static int buttonlight_timer;
int _buttonlight_timeout = 5*HZ;

/* Update state of buttonlight according to timeout setting */
static void buttonlight_update_state(void)
{
    buttonlight_timer = _buttonlight_timeout;

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

void buttonlight_set_timeout(int value)
{
    _buttonlight_timeout = HZ * value;
    buttonlight_update_state();
}

#endif /* HAVE_BUTTON_LIGHT */

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
#endif /* HAVE_REMOTE_LCD */

#ifdef HAVE_LCD_SLEEP
#ifdef HAVE_LCD_SLEEP_SETTING
const signed char lcd_sleep_timeout_value[10] =
{
    -1, 0, 5, 10, 15, 20, 30, 45, 60, 90
};
static int lcd_sleep_timeout = 10*HZ;
#else
/* Target defines needed value */
static const int lcd_sleep_timeout = LCD_SLEEP_TIMEOUT;
#endif

static int lcd_sleep_timer = 0;

void backlight_lcd_sleep_countdown(bool start)
{
    if (!start)
    {
        /* Cancel the LCD sleep countdown */
        lcd_sleep_timer = 0;
        return;
    }

    /* Start LCD sleep countdown */
    if (lcd_sleep_timeout < 0)
    {
        lcd_sleep_timer = 0; /* Setting == Always */
        lcd_sleep();
    }
    else
    {
        lcd_sleep_timer = lcd_sleep_timeout;
    }
}
#endif /* HAVE_LCD_SLEEP */

#if defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR)
/* backlight fading */
#define BL_PWM_INTERVAL 5  /* Cycle interval in ms */
#define BL_PWM_BITS     8
#define BL_PWM_COUNT    (1<<BL_PWM_BITS)

/* s15.16 fixed point variables */
static int32_t bl_fade_in_step  = ((BL_PWM_INTERVAL*BL_PWM_COUNT)<<16)/300;
static int32_t bl_fade_out_step = ((BL_PWM_INTERVAL*BL_PWM_COUNT)<<16)/2000;
static int32_t bl_dim_fraction  = 0;     

static int bl_dim_target  = 0;
static int bl_dim_current = 0;
static enum {DIM_STATE_START, DIM_STATE_MAIN} bl_dim_state = DIM_STATE_START;
static bool bl_timer_active = false;

static void backlight_isr(void)
{
    int timer_period = (TIMER_FREQ*BL_PWM_INTERVAL/1000);
    bool idle = false;

    switch (bl_dim_state)
    {
      /* New cycle */
      case DIM_STATE_START:
        bl_dim_current = bl_dim_fraction >> 16;

        if (bl_dim_current > 0 && bl_dim_current < BL_PWM_COUNT)
        {
            _backlight_on_isr();
            timer_period = (timer_period * bl_dim_current) >> BL_PWM_BITS;
            bl_dim_state = DIM_STATE_MAIN;
        }
        else
        {
            if (bl_dim_current)
                _backlight_on_isr();
            else
                _backlight_off_isr();
            if (bl_dim_current == bl_dim_target)
                idle = true;
        }
        if (bl_dim_current < bl_dim_target)
        {
            bl_dim_fraction = MIN(bl_dim_fraction + bl_fade_in_step,
                                  (BL_PWM_COUNT<<16));
        }
        else if (bl_dim_current > bl_dim_target)
        {
            bl_dim_fraction = MAX(bl_dim_fraction - bl_fade_out_step, 0);
        }
        break;

      /* Dim main screen */
      case DIM_STATE_MAIN:
        _backlight_off_isr();
        timer_period = (timer_period * (BL_PWM_COUNT - bl_dim_current))
                     >> BL_PWM_BITS;
        bl_dim_state = DIM_STATE_START;
        break ;
    }
    if (idle)
    {
#if defined(_BACKLIGHT_FADE_BOOST) || defined(_BACKLIGHT_FADE_ENABLE)
        queue_post(&backlight_queue, BACKLIGHT_FADE_FINISH, 0);
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
        _backlight_on_normal();
        bl_dim_fraction = (BL_PWM_COUNT<<16);
    }
    else
    {
        _backlight_off_normal();
        bl_dim_fraction = 0;
    }
}

static void backlight_release_timer(void)
{
#ifdef _BACKLIGHT_FADE_BOOST
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

    if (timer_register(0, backlight_release_timer, 2, 0, backlight_isr
                       IF_COP(, CPU)))
    {
#ifdef _BACKLIGHT_FADE_BOOST
        /* Prevent cpu frequency changes while dimming. */
        cpu_boost(true);
#endif
        bl_timer_active = true;
    }
    else
        backlight_switch();
}

static void _backlight_on(void)
{
#ifdef HAVE_LCD_SLEEP
    backlight_lcd_sleep_countdown(false);
#endif

    if (bl_fade_in_step > 0)
    {
#ifdef _BACKLIGHT_FADE_ENABLE
        _backlight_hw_enable(true);
#endif
        backlight_dim(BL_PWM_COUNT);
    }
    else
    {
        bl_dim_target = BL_PWM_COUNT;
        bl_dim_fraction = (BL_PWM_COUNT<<16);
        _backlight_on_normal();
    }
}

static void _backlight_off(void)
{
    if (bl_fade_out_step > 0)
    {
        backlight_dim(0);
    }
    else
    {
        bl_dim_target = bl_dim_fraction = 0;
        _backlight_off_normal();
    }

#ifdef HAVE_LCD_SLEEP
    backlight_lcd_sleep_countdown(true);
#endif
}

void backlight_set_fade_in(int value)
{
    if (value > 0)
        bl_fade_in_step = ((BL_PWM_INTERVAL*BL_PWM_COUNT)<<16) / value;
    else
        bl_fade_in_step = 0;
}

void backlight_set_fade_out(int value)
{
    if (value > 0)
        bl_fade_out_step = ((BL_PWM_INTERVAL*BL_PWM_COUNT)<<16) / value;
    else
        bl_fade_out_step = 0;
}
#endif /* defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR) */

/* Update state of backlight according to timeout setting */
static void backlight_update_state(void)
{
#ifdef HAS_BUTTON_HOLD
    if ((backlight_on_button_hold != 0)
#ifdef HAVE_REMOTE_LCD_AS_MAIN
        && remote_button_hold()
#else
        && button_hold()
#endif
        )
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
        _remote_backlight_off();
    }
    else
    {
        remote_backlight_timer = remote_backlight_timeout;
        _remote_backlight_on();
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
#ifdef _BACKLIGHT_FADE_BOOST
            case BACKLIGHT_FADE_FINISH:
                cpu_boost(false);
                break;
#endif
#ifdef _BACKLIGHT_FADE_ENABLE
            case BACKLIGHT_FADE_FINISH:
                _backlight_hw_enable((bl_dim_current|bl_dim_target) != 0);
                break;
#endif

#ifndef SIMULATOR
            /* Here for now or else the aggressive init messes up scrolling */
#ifdef HAVE_REMOTE_LCD
            case SYS_REMOTE_PLUGGED:
                lcd_remote_on();
                lcd_remote_update();
                break;

            case SYS_REMOTE_UNPLUGGED:
                lcd_remote_off();
                break;
#elif defined HAVE_REMOTE_LCD_AS_MAIN
            case SYS_REMOTE_PLUGGED:
                lcd_on();
                lcd_update();
                break;

            case SYS_REMOTE_UNPLUGGED:
                lcd_off();
                break;
#endif /* HAVE_REMOTE_LCD/ HAVE_REMOTE_LCD_AS_MAIN */
#endif /* !SIMULATOR */
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

#ifdef BACKLIGHT_DRIVER_CLOSE
            /* Get out of here */
            case BACKLIGHT_QUIT:
                return;
#endif
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
                _remote_backlight_off();
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
    if (_backlight_init())
    {
# ifdef HAVE_BACKLIGHT_PWM_FADING
        /* If backlight is already on, don't fade in. */
        bl_dim_target = BL_PWM_COUNT;
        bl_dim_fraction = (BL_PWM_COUNT<<16);
# endif
    }
#endif
    /* Leave all lights as set by the bootloader here. The settings load will
     * call the appropriate backlight_set_*() functions, only changing light
     * status if necessary. */
#ifdef BACKLIGHT_DRIVER_CLOSE
    backlight_thread_p =
#endif
    create_thread(backlight_thread, backlight_stack,
                  sizeof(backlight_stack), 0, backlight_thread_name
                  IF_PRIO(, PRIORITY_USER_INTERFACE)
                  IF_COP(, CPU));
    tick_add_task(backlight_tick);
}

#ifdef BACKLIGHT_DRIVER_CLOSE
void backlight_close(void)
{
    struct thread_entry *thread = backlight_thread_p;

    /* Wait for thread to exit */
    if (thread == NULL)
        return;

    backlight_thread_p = NULL;

    queue_post(&backlight_queue, BACKLIGHT_QUIT, 0);
    thread_wait(thread);
}
#endif /* BACKLIGHT_DRIVER_CLOSE */

void backlight_on(void)
{
    queue_remove_from_head(&backlight_queue, BACKLIGHT_ON);
    queue_post(&backlight_queue, BACKLIGHT_ON, 0);
}

void backlight_off(void)
{
    queue_post(&backlight_queue, BACKLIGHT_OFF, 0);
}

/* returns true when the backlight is on,
 * and optionally when it's set to always off. */
bool is_backlight_on(bool ignore_always_off)
{
    return (backlight_timer > 0)   /* countdown */
        || (backlight_timeout == 0) /* always on */
        || ((backlight_timeout < 0) && !ignore_always_off);
}

/* return value in ticks; 0 means always on, <0 means always off */
int backlight_get_current_timeout(void)
{
    return backlight_timeout;
}

void backlight_set_timeout(int value)
{
    backlight_timeout_normal = HZ * value;
    backlight_update_state();
}

#if CONFIG_CHARGING
void backlight_set_timeout_plugged(int value)
{
    backlight_timeout_plugged = HZ * value;
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

#ifdef HAVE_LCD_SLEEP_SETTING
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
#endif /* HAVE_LCD_SLEEP_SETTING */

#ifdef HAVE_REMOTE_LCD
void remote_backlight_on(void)
{
    queue_post(&backlight_queue, REMOTE_BACKLIGHT_ON, 0);
}

void remote_backlight_off(void)
{
    queue_post(&backlight_queue, REMOTE_BACKLIGHT_OFF, 0);
}

void remote_backlight_set_timeout(int value)
{
    remote_backlight_timeout_normal = HZ * value;
    remote_backlight_update_state();
}

#if CONFIG_CHARGING
void remote_backlight_set_timeout_plugged(int value)
{
    remote_backlight_timeout_plugged = HZ * value;
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

/* returns true when the backlight is on, and
 * optionally  when it's set to always off */
bool is_remote_backlight_on(bool ignore_always_off)
{
    return (remote_backlight_timer > 0)   /* countdown */
        || (remote_backlight_timeout == 0) /* always on */
        || ((remote_backlight_timeout < 0) && !ignore_always_off);
}

#endif /* HAVE_REMOTE_LCD */

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
void backlight_set_brightness(int val)
{
    if (val < MIN_BRIGHTNESS_SETTING)
        val = MIN_BRIGHTNESS_SETTING;
    else if (val > MAX_BRIGHTNESS_SETTING)
        val = MAX_BRIGHTNESS_SETTING;

    _backlight_set_brightness(val);
}
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
void buttonlight_set_brightness(int val)
{
    if (val < MIN_BRIGHTNESS_SETTING)
        val = MIN_BRIGHTNESS_SETTING;
    else if (val > MAX_BRIGHTNESS_SETTING)
        val = MAX_BRIGHTNESS_SETTING;

    _buttonlight_set_brightness(val);
}
#endif /* HAVE_BUTTONLIGHT_BRIGHTNESS */

#else /* !defined(HAVE_BACKLIGHT) || !defined(BACKLIGHT_FULL_INIT)
    -- no backlight, empty dummy functions */

#if defined(HAVE_BACKLIGHT) && !defined(BACKLIGHT_FULL_INIT)
void backlight_init(void)
{
    (void)_backlight_init();
    _backlight_on();
}
#endif

void backlight_on(void) {}
void backlight_off(void) {}
void buttonlight_on(void) {}
void backlight_set_timeout(int value) {(void)value;}

bool is_backlight_on(bool ignore_always_off)
{
    (void)ignore_always_off;
    return true;
}
#ifdef HAVE_REMOTE_LCD
void remote_backlight_on(void) {}
void remote_backlight_off(void) {}
void remote_backlight_set_timeout(int value) {(void)value;}

bool is_remote_backlight_on(bool ignore_always_off)
{
    (void)ignore_always_off;
    return true;
}
#endif /* HAVE_REMOTE_LCD */
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
void backlight_set_brightness(int val) { (void)val; }
#endif
#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
void buttonlight_set_brightness(int val) { (void)val; }
#endif
#endif /* defined(HAVE_BACKLIGHT) && defined(BACKLIGHT_FULL_INIT) */
