/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "backlight-target.h"
#include "backlight.h"
#include "lcd.h"
#include "sc606-meg-fx.h"
#include "power.h"


#define FLICKER_PERIOD 15
#define BUTTONLIGHT_MENU (SC606_LED_B1)
#define BUTTONLIGHT_ALL  (SC606_LED_B1 | SC606_LED_B2 | SC606_LED_C1 | SC606_LED_C2)

static void led_control_service(void);
static unsigned short backlight_brightness;
static unsigned short backlight_current;
static unsigned short backlight_target;
static unsigned short time_til_fade;
static unsigned short fade_interval;
static unsigned short initial_tick_delay;
static unsigned char backlight_leds;

static enum backlight_states
{
    BACKLIGHT_CONTROL_IDLE,
    BACKLIGHT_CONTROL_OFF,
    BACKLIGHT_CONTROL_ON,
    BACKLIGHT_CONTROL_SET,
    BACKLIGHT_CONTROL_FADE_OFF,
    BACKLIGHT_CONTROL_FADE_ON,
    BACKLIGHT_CONTROL_FADE_ON_FROM_OFF
} backlight_control;



enum buttonlight_states
{
    /* turn button lights off */
    BUTTONLIGHT_MODE_OFF_ENTRY,
    BUTTONLIGHT_MODE_OFF,

    /* turns button lights on to setting */
    BUTTONLIGHT_MODE_ON_ENTRY,
    BUTTONLIGHT_MODE_ON,

    /* turns button lights on to minimum */
    BUTTONLIGHT_MODE_FAINT_ENTRY,
    BUTTONLIGHT_MODE_FAINT,

    /* allows button lights to flicker when triggered */
    BUTTONLIGHT_MODE_FLICKER_ENTRY,
    BUTTONLIGHT_MODE_FLICKER,
    BUTTONLIGHT_MODE_FLICKERING,

    /* button lights solid */
    BUTTONLIGHT_MODE_SOLID_ENTRY,
    BUTTONLIGHT_MODE_SOLID,

    /* button light charing */
    BUTTONLIGHT_MODE_CHARGING_ENTRY,
    BUTTONLIGHT_MODE_CHARGING,
    BUTTONLIGHT_MODE_CHARGING_WAIT,

    /* internal use only */
    BUTTONLIGHT_HELPER_SET,
    BUTTONLIGHT_HELPER_SET_FINAL,
    BUTTONLIGHT_MODE_STOP,

    /* buttonlights follow the backlight settings */
    BUTTONLIGHT_MODE_FOLLOW_ENTRY,
    BUTTONLIGHT_MODE_FOLLOW,
};



static char buttonlight_leds;
static unsigned short buttonlight_setting;
static unsigned short buttonlight_current;
static unsigned char buttonlight_selected;
static enum buttonlight_states buttonlight_state;
static enum buttonlight_states buttonlight_saved_state;
static unsigned short buttonlight_flickering;

static unsigned short buttonlight_trigger_now;
static unsigned short buttonlight_trigger_brightness;



static unsigned short charging_led_index;
static unsigned short buttonlight_charging_counter;

#define CHARGING_LED_COUNT 60
unsigned char charging_leds[] = { 0x00, 0x20, 0x38, 0x3C };



bool __backlight_init(void)
{
    backlight_control = BACKLIGHT_CONTROL_IDLE;

    backlight_current = DEFAULT_BRIGHTNESS_SETTING;

    buttonlight_state = BUTTONLIGHT_MODE_OFF;

    buttonlight_selected = 0x04;

    /* delay 4 seconds before any fading */
    initial_tick_delay = 400;
    /* put the led control on the tick list */
    tick_add_task(led_control_service);
    
    return true;
}



void __backlight_on(void)
{
    /* now go turn the backlight on */
    backlight_control = BACKLIGHT_CONTROL_ON;
}



void __backlight_off(void)
{
    backlight_control = BACKLIGHT_CONTROL_OFF;
}



/* Assumes that the backlight has been initialized */
void __backlight_set_brightness(int brightness)
{
    /* stop the interrupt from messing us up */
    backlight_control = BACKLIGHT_CONTROL_IDLE;

    backlight_brightness = brightness + 1;

    /* only set the brightness if it is different from the current */
    if (backlight_brightness != backlight_current)
    {
        backlight_control = BACKLIGHT_CONTROL_SET;
    }
}



/* only works if the buttonlight mode is set to triggered mode */
void __buttonlight_trigger(void)
{
    buttonlight_trigger_now = 1;
}




/* map the mode from the command into the state machine entries */
void __buttonlight_mode(enum buttonlight_mode mode,
                        enum buttonlight_selection selection,
                        unsigned short brightness)
{
    /* choose stop to setup mode */
    buttonlight_state = BUTTONLIGHT_MODE_STOP;


    /* clip brightness */
    if (brightness > MAX_BRIGHTNESS_SETTING)
    {
        brightness = MAX_BRIGHTNESS_SETTING;
    }

    brightness++;

    /* Select which LEDs to use */
    switch (selection)
    {
        case BUTTONLIGHT_LED_ALL:
        buttonlight_selected = BUTTONLIGHT_ALL;
        break;

        case BUTTONLIGHT_LED_MENU:
        buttonlight_selected = BUTTONLIGHT_MENU;
        break;
    }

    /* which mode to use */
    switch (mode)
    {
        case BUTTONLIGHT_OFF:
        buttonlight_state = BUTTONLIGHT_MODE_OFF_ENTRY;
        break;

        case BUTTONLIGHT_ON:
        buttonlight_trigger_brightness = brightness;
        buttonlight_state = BUTTONLIGHT_MODE_ON_ENTRY;
        break;

        /* faint is just a quick way to set ON to 1 */
        case BUTTONLIGHT_FAINT:
        buttonlight_trigger_brightness = 1;
        buttonlight_state = BUTTONLIGHT_MODE_ON_ENTRY;
        break;

        case BUTTONLIGHT_FLICKER:
        buttonlight_trigger_brightness = brightness;
        buttonlight_state = BUTTONLIGHT_MODE_FLICKER_ENTRY;
        break;

        case BUTTONLIGHT_SIGNAL:
        buttonlight_trigger_brightness = brightness;
        buttonlight_state = BUTTONLIGHT_MODE_SOLID_ENTRY;
        break;

        case BUTTONLIGHT_FOLLOW:
        buttonlight_state = BUTTONLIGHT_MODE_FOLLOW_ENTRY;
        break;

        case BUTTONLIGHT_CHARGING:
        buttonlight_state = BUTTONLIGHT_MODE_CHARGING_ENTRY;
        break;

        default:
        return; /* unknown mode */
    }


}



/*
 * The button lights have 'modes' of operation. Each mode must setup and
 * execute its own operation - taking care that this is all done in an ISR.
 *
 */



/* led_control_service runs in interrupt context - be brief!
 * This service is called once per interrupt timer tick - 100 times a second.
 *
 * There should be at most only one i2c operation per call - if more are need
 *  the calls should be spread across calls.
 *
 * Putting all led servicing in one thread means that we wont step on any
 * i2c operations - they are all serialized here in the ISR tick. It also
 * insures that we get called at equal timing for good visual effect.
 *
 * The buttonlight service runs only after all backlight services have finished.
 * Fading the buttonlights is possible, but not recommended because of the
 * additional calls needed during the ISR
 */
static void led_control_service(void)
{
    if(initial_tick_delay) {
        initial_tick_delay--;
        return;
    }
    switch (backlight_control)
    {
        case BACKLIGHT_CONTROL_IDLE:
        switch (buttonlight_state)
        {
            case BUTTONLIGHT_MODE_STOP: break;

            /* Buttonlight mode: OFF */
            case BUTTONLIGHT_MODE_OFF_ENTRY:
            if (buttonlight_current)
            {
                buttonlight_leds = 0x00;
                sc606_write(SC606_REG_CONF, backlight_leds);
                buttonlight_current = 0;
            }
            buttonlight_state = BUTTONLIGHT_MODE_OFF;
            break;

            case BUTTONLIGHT_MODE_OFF:
            break;


            /* button mode: CHARGING - show charging sequence */
            case BUTTONLIGHT_MODE_CHARGING_ENTRY:
            /* start turned off */
            buttonlight_leds = 0x00;
            sc606_write(SC606_REG_CONF, backlight_leds);
            buttonlight_current = 0;

            /* temporary save for the next mode - then to do settings */
            buttonlight_setting = DEFAULT_BRIGHTNESS_SETTING;
            buttonlight_saved_state = BUTTONLIGHT_MODE_CHARGING_WAIT;
            buttonlight_state = BUTTONLIGHT_HELPER_SET;
            break;


            case BUTTONLIGHT_MODE_CHARGING:
            if (--buttonlight_charging_counter == 0)
            {
                /* change led */
                if (charging_state())
                {
                    buttonlight_leds = charging_leds[charging_led_index];
                    if (++charging_led_index >= sizeof(charging_leds))
                    {
                        charging_led_index = 0;
                    }
                    sc606_write(SC606_REG_CONF, backlight_leds | buttonlight_leds);
                    buttonlight_charging_counter = CHARGING_LED_COUNT;
                }
                else
                {
                    buttonlight_state = BUTTONLIGHT_MODE_CHARGING_ENTRY;
                }
            }
            break;

            /* wait for the charget to be plugged in */
            case BUTTONLIGHT_MODE_CHARGING_WAIT:
            if (charging_state())
            {
                charging_led_index = 0;
                buttonlight_charging_counter = CHARGING_LED_COUNT;
                buttonlight_state = BUTTONLIGHT_MODE_CHARGING;
            }
            break;


            /* Buttonlight mode: FOLLOW - try to stay current with backlight
             * since this runs in the idle of the backlight it will not really
             * follow in real time
             */
            case BUTTONLIGHT_MODE_FOLLOW_ENTRY:
            /* case 1 - backlight on, but buttonlight is off */
            if (backlight_current)
            {
                /* Turn the buttonlights on */
                buttonlight_leds = buttonlight_selected;
                sc606_write(SC606_REG_CONF, backlight_leds | buttonlight_leds);

                /* temporary save for the next mode - then to do settings */
                buttonlight_setting = backlight_current;
                buttonlight_saved_state = BUTTONLIGHT_MODE_FOLLOW;
                buttonlight_state = BUTTONLIGHT_HELPER_SET;
            }
            /* case 2 - backlight off, but buttonlight is on */
            else
            {
                buttonlight_current = 0;
                buttonlight_leds = 0x00;
                sc606_write(SC606_REG_CONF, backlight_leds);
                buttonlight_state = BUTTONLIGHT_MODE_FOLLOW;
            }
            break;

            case BUTTONLIGHT_MODE_FOLLOW:
            if (buttonlight_current != backlight_current)
            {
                /* case 1 - backlight on, but buttonlight is off */
                if (backlight_current)
                {
                    if (0 == buttonlight_current)
                    {
                        /* Turn the buttonlights on */
                        buttonlight_leds = buttonlight_selected;
                        sc606_write(SC606_REG_CONF, backlight_leds | buttonlight_leds);
                    }

                    /* temporary save for the next mode - then to do settings */
                    buttonlight_setting = backlight_current;
                    buttonlight_saved_state = BUTTONLIGHT_MODE_FOLLOW;
                    buttonlight_state = BUTTONLIGHT_HELPER_SET;
                }

                /* case 2 - backlight off, but buttonlight is on */
                else
                {
                    buttonlight_current = 0;
                    buttonlight_leds = 0x00;
                    sc606_write(SC606_REG_CONF, backlight_leds);
                }

            }
            break;



            /* Buttonlight mode: ON - stays at the set brightness */
            case BUTTONLIGHT_MODE_ON_ENTRY:
            buttonlight_leds = buttonlight_selected;
            sc606_write(SC606_REG_CONF, backlight_leds | buttonlight_leds);

            /* temporary save for the next mode - then to do settings */
            buttonlight_setting = buttonlight_trigger_brightness;
            buttonlight_saved_state = BUTTONLIGHT_MODE_ON;
            buttonlight_state = BUTTONLIGHT_HELPER_SET;
            break;

            case BUTTONLIGHT_MODE_ON:
            break;



            /* Buttonlight mode: FLICKER */
            case BUTTONLIGHT_MODE_FLICKER_ENTRY:
            /* already on? turn it off */
            if (buttonlight_current)
            {
                buttonlight_leds = 0x00;
                sc606_write(SC606_REG_CONF, backlight_leds);
                buttonlight_current = 0;
            }

            /* set the brightness if not already set */
            if (buttonlight_current != buttonlight_trigger_brightness)
            {
                /* temporary save for the next mode - then to do settings */
                buttonlight_setting = buttonlight_trigger_brightness;
                buttonlight_saved_state = BUTTONLIGHT_MODE_FLICKER;
                buttonlight_state = BUTTONLIGHT_HELPER_SET;
            }
            else buttonlight_state = BUTTONLIGHT_MODE_FLICKER;
            break;


            case BUTTONLIGHT_MODE_FLICKER:
            /* wait for the foreground to trigger flickering */
            if (buttonlight_trigger_now)
            {
                /* turn them on */
                buttonlight_leds = buttonlight_selected;
                buttonlight_current = buttonlight_setting;
                sc606_write(SC606_REG_CONF, backlight_leds | buttonlight_leds);

                /* reset the trigger and go flicker the LEDs */
                buttonlight_trigger_now = 0;
                buttonlight_flickering = FLICKER_PERIOD;
                buttonlight_state = BUTTONLIGHT_MODE_FLICKERING;
            }
            break;


            case BUTTONLIGHT_MODE_FLICKERING:
            /* flicker the LEDs for as long as we get triggered */
            if (buttonlight_flickering)
            {
                /* turn the leds off if they are on */
                if (buttonlight_current)
                {
                    buttonlight_leds = 0x00;
                    sc606_write(SC606_REG_CONF, backlight_leds);
                    buttonlight_current = 0;
                }

                buttonlight_flickering--;
            }
            else
            {
                /* is flickering triggered again? */
                if (!buttonlight_trigger_now)
                {
                    /* completed a cycle - no new triggers - go back and wait */
                    buttonlight_state = BUTTONLIGHT_MODE_FLICKER;
                }
                else
                {
                    /* reset flickering */
                    buttonlight_trigger_now = 0;
                    buttonlight_flickering = FLICKER_PERIOD;

                    /* turn buttonlights on */
                    buttonlight_leds = buttonlight_selected;
                    buttonlight_current = buttonlight_setting;
                    sc606_write(SC606_REG_CONF, backlight_leds | buttonlight_leds);
                }
            }
            break;


            /* Buttonlight mode: SIGNAL / SOLID */
            case BUTTONLIGHT_MODE_SOLID_ENTRY:
            /* already on? turn it off */
            if (buttonlight_current)
            {
                buttonlight_leds = 0x00;
                sc606_write(SC606_REG_CONF, backlight_leds);
                buttonlight_current = 0;
            }

            /* set the brightness if not already set */
            /* temporary save for the next mode - then to do settings */
            buttonlight_setting = buttonlight_trigger_brightness;
            buttonlight_saved_state = BUTTONLIGHT_MODE_SOLID;
            buttonlight_state = BUTTONLIGHT_HELPER_SET;
            break;


            case BUTTONLIGHT_MODE_SOLID:
            /* wait for the foreground to trigger */
            if (buttonlight_trigger_now)
            {
                /* turn them on if not already on */
                if (0 == buttonlight_current)
                {
                    buttonlight_leds = buttonlight_selected;
                    buttonlight_current = buttonlight_setting;
                    sc606_write(SC606_REG_CONF, backlight_leds | buttonlight_leds);
                }

                /* reset the trigger */
                buttonlight_trigger_now = 0;
            }
            else
            {
                if (buttonlight_current)
                {
                    buttonlight_leds = 0x00;
                    sc606_write(SC606_REG_CONF, backlight_leds);
                    buttonlight_current = 0;
                }
            }
            break;


            /* set the brightness for the buttonlights - takes 2 passes */
            case BUTTONLIGHT_HELPER_SET:
            sc606_write(SC606_REG_B, buttonlight_setting-1);
            buttonlight_state = BUTTONLIGHT_HELPER_SET_FINAL;
            break;

            case BUTTONLIGHT_HELPER_SET_FINAL:
            sc606_write(SC606_REG_C, buttonlight_setting-1);
            buttonlight_current = buttonlight_setting;
            buttonlight_state = buttonlight_saved_state;
            break;

            default:
            break;

        }
        break;


        case BACKLIGHT_CONTROL_FADE_ON_FROM_OFF:
        backlight_leds = 0x03;
        sc606_write(SC606_REG_CONF, 0x03 | buttonlight_leds);
        backlight_control = BACKLIGHT_CONTROL_FADE_ON;
        break;


        case BACKLIGHT_CONTROL_OFF:
        backlight_current = 0;
        backlight_leds = 0x00;
        sc606_write(SC606_REG_CONF, buttonlight_leds);
        backlight_control = BACKLIGHT_CONTROL_IDLE;

        break;


        case BACKLIGHT_CONTROL_ON:
        backlight_leds = 0x03;
        sc606_write(SC606_REG_CONF, 0x03 | buttonlight_leds);
        backlight_current = backlight_brightness;
        backlight_control = BACKLIGHT_CONTROL_IDLE;
        break;


        case BACKLIGHT_CONTROL_SET:
        /* The SC606 LED driver can set the brightness in 64 steps */
        sc606_write(SC606_REG_A, backlight_brightness-1);

        /* if we were turned off - turn the backlight on */
        if (backlight_current)
        {
            backlight_current = backlight_brightness;
            backlight_control = BACKLIGHT_CONTROL_IDLE;
        }
        else
        {
            backlight_control = BACKLIGHT_CONTROL_ON;
        }
        break;


        case BACKLIGHT_CONTROL_FADE_ON:
        if (--time_til_fade) return;

        /* The SC606 LED driver can set the brightness in 64 steps */
        sc606_write(SC606_REG_A, backlight_current++);

        /* have we hit the target? */
        if (backlight_current == backlight_target)
        {
            backlight_control = BACKLIGHT_CONTROL_IDLE;
        }
        else
        {
            time_til_fade = fade_interval;
        }
        break;


        case BACKLIGHT_CONTROL_FADE_OFF:
        if (--time_til_fade) return;

        /* The SC606 LED driver can set the brightness in 64 steps */
        sc606_write(SC606_REG_A, --backlight_current);

        /* have we hit the target? */
        if (backlight_current == backlight_target)
        {
            if (backlight_current)
            {
                backlight_control = BACKLIGHT_CONTROL_IDLE;
            }
            else
            {
                backlight_control = BACKLIGHT_CONTROL_OFF;
            }

        }
        else
        {
            time_til_fade = fade_interval;
        }
        break;
    }

    if(backlight_current)
        lcd_enable(true);
    else
        lcd_enable(false);
}





void __backlight_dim(bool dim_now)
{
    unsigned short target;

    /* dont let the interrupt tick happen */
    backlight_control = BACKLIGHT_CONTROL_IDLE;

    target = (dim_now == true) ? 0 : backlight_brightness;

    /* only try and fade if the target is different */
    if (backlight_current != target)
    {
        backlight_target = target;

        if (backlight_current > backlight_target)
        {
            time_til_fade = fade_interval = 4;
            backlight_control = BACKLIGHT_CONTROL_FADE_OFF;
        }
        else
        {
            time_til_fade = fade_interval = 1;
            if (backlight_current)
            {
                backlight_control = BACKLIGHT_CONTROL_FADE_ON;
            }
            else
            {
                backlight_control = BACKLIGHT_CONTROL_FADE_ON_FROM_OFF;
            }
        }
    }

}


