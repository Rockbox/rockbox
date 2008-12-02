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
#include "backlight-target.h"
#include "backlight.h"
#include "lcd.h"
#include "sc606-meg-fx.h"
#include "power.h"

#define BUTTONLIGHT_MENU (SC606_LED_B1)
#define BUTTONLIGHT_ALL  (SC606_LED_B1 | SC606_LED_B2 | \
                          SC606_LED_C1 | SC606_LED_C2)

/* Dummy value at index 0, 1-12 used. */
static const unsigned char log_brightness[13] =
    {0,0,1,2,3,5,7,10,15,22,31,44,63};

#ifndef BOOTLOADER
static void led_control_service(void);

static enum sc606_states
{
    SC606_CONTROL_IDLE,
    SC606_CONTROL_A12,
    SC606_CONTROL_B12,
    SC606_CONTROL_C12,
    SC606_CONTROL_CONF
} sc606_control;
#endif /* BOOTLOADER */

static enum backlight_states
{
    BACKLIGHT_CONTROL_IDLE,
    BACKLIGHT_CONTROL_OFF,
    BACKLIGHT_CONTROL_ON,
    BACKLIGHT_CONTROL_SET,
    BACKLIGHT_CONTROL_FADE
} backlight_control;

enum buttonlight_states
{
    BUTTONLIGHT_CONTROL_IDLE,
    BUTTONLIGHT_CONTROL_OFF,
    BUTTONLIGHT_CONTROL_ON,
    BUTTONLIGHT_CONTROL_SET,
    BUTTONLIGHT_CONTROL_FADE,
} buttonlight_control;

static unsigned char _backlight_brightness;
static unsigned char buttonlight_brightness;
static unsigned char backlight_target;
static unsigned char buttonlight_target;

static unsigned short buttonlight_trigger_now;

/* Assumes that the backlight has been initialized */
void _backlight_set_brightness(int brightness)
{
    /* stop the interrupt from messing us up */
    backlight_control = BACKLIGHT_CONTROL_IDLE;
    _backlight_brightness = log_brightness[brightness];
    backlight_control = BACKLIGHT_CONTROL_SET;
}

/* only works if the buttonlight mode is set to triggered mode */
void __buttonlight_trigger(void)
{
    buttonlight_trigger_now = 1;
}

/* map the mode from the command into the state machine entries */
void __buttonlight_mode(enum buttonlight_mode mode)
{
    /* choose stop to setup mode */
    buttonlight_control = BUTTONLIGHT_CONTROL_IDLE;

    /* which mode to use */
    switch (mode)
    {
        case BUTTONLIGHT_OFF:
        buttonlight_control = BUTTONLIGHT_CONTROL_OFF;
        break;

        case BUTTONLIGHT_ON:
        buttonlight_control = BUTTONLIGHT_CONTROL_ON;
        break;
        
        case BUTTONLIGHT_FOLLOW:
        buttonlight_control = BUTTONLIGHT_CONTROL_FADE;
        break;

        default:
        return; /* unknown mode */
    }
}

/* led_control_service runs in interrupt context - be brief!
 * This service is called once per interrupt timer tick - 100 times a second.
 *
 * There should be at most only one i2c operation per call - if more are need
 *  the calls should be spread across calls.
 *
 * Putting all led servicing in one thread means that we wont step on any
 * i2c operations - they are all serialized here in the ISR tick. It also
 * insures that we get called at equal timing for good visual effect.
 */
#ifndef BOOTLOADER
static void led_control_service(void)
{
    static unsigned char
        sc606regAval=DEFAULT_BRIGHTNESS_SETTING,
        sc606regBval=0,
        sc606regCval=0,
        sc606regCONFval=0x03;

    static bool sc606_changed=false, sc606_CONF_changed=false;

    if(sc606_changed==false)
    {
        switch (backlight_control)
        {
            case BACKLIGHT_CONTROL_IDLE:
                backlight_control = BACKLIGHT_CONTROL_IDLE;
                break;
            case BACKLIGHT_CONTROL_OFF:
                sc606_changed=true;
                sc606_CONF_changed=true;
                sc606regCONFval &= ~0x03;
                sc606regAval=0;
                backlight_control = BACKLIGHT_CONTROL_IDLE;
                break;
            case BACKLIGHT_CONTROL_ON:
                sc606_changed=true;
                sc606_CONF_changed=true;
                sc606regCONFval |= 0x03;
                sc606regAval=_backlight_brightness;
                backlight_control = BACKLIGHT_CONTROL_IDLE;
                break;
            case BACKLIGHT_CONTROL_SET:
                if(!(sc606regCONFval&0x03))
                    break;
                sc606_changed=true;
                sc606regAval = _backlight_brightness;
                backlight_control = BACKLIGHT_CONTROL_IDLE;
                break;
            case BACKLIGHT_CONTROL_FADE:
                /* Was this mode set while the backlight is already on/off? */
                if(backlight_target==sc606regAval)
                {
                    backlight_control = BACKLIGHT_CONTROL_IDLE;
                    break;
                }
                sc606_changed=true;
                if(!(sc606regCONFval&0x03))
                {
                    sc606_CONF_changed=true;
                    sc606regCONFval |= 0x03;
                }
                if(backlight_target>sc606regAval)
                {
                    sc606regAval++;
                    if(backlight_target==sc606regAval)
                        backlight_control = BACKLIGHT_CONTROL_IDLE;
                }
                else
                {
                    sc606regAval--;
                    if(sc606regAval==0)
                        backlight_control = BACKLIGHT_CONTROL_OFF;
                    else if (backlight_target==sc606regAval)
                        backlight_control = BACKLIGHT_CONTROL_IDLE;
                }

                break;
            default:
                backlight_control = BACKLIGHT_CONTROL_IDLE;
                break;
        }
        switch (buttonlight_control)
        {
            case BUTTONLIGHT_CONTROL_IDLE:
                buttonlight_control=BUTTONLIGHT_CONTROL_IDLE;
                break;
            case BUTTONLIGHT_CONTROL_OFF:
                sc606_changed=true;
                sc606_CONF_changed=true;
                sc606regCONFval &= ~BUTTONLIGHT_ALL;
                sc606regBval=sc606regCval=0;
                buttonlight_control=BUTTONLIGHT_CONTROL_IDLE;
                break;
            case BUTTONLIGHT_CONTROL_ON:
                sc606_changed=true;
                sc606_CONF_changed=true;
                sc606regCONFval |= BUTTONLIGHT_ALL;
                sc606regBval=sc606regCval=buttonlight_brightness;
                buttonlight_control=BUTTONLIGHT_CONTROL_IDLE;
                break;
            case BUTTONLIGHT_CONTROL_SET:
                if(!(sc606regCONFval&BUTTONLIGHT_ALL))
                    break;
                sc606_changed=true;
                sc606regBval=sc606regCval=buttonlight_brightness;
                buttonlight_control = BUTTONLIGHT_CONTROL_IDLE;
                break;
            case BUTTONLIGHT_CONTROL_FADE:
                /* Was this mode set while the button light is already on? */
                if(buttonlight_target==sc606regBval)
                {
                    buttonlight_control = BUTTONLIGHT_CONTROL_IDLE;
                    break;
                }
                sc606_changed=true;
                if(!(sc606regCONFval&BUTTONLIGHT_ALL))
                {
                    sc606_CONF_changed=true;
                    sc606regCONFval |= BUTTONLIGHT_ALL;
                }
                if(buttonlight_target>sc606regBval)
                {
                    sc606regCval=++sc606regBval;
                    if(buttonlight_target==sc606regBval)
                        buttonlight_control = BUTTONLIGHT_CONTROL_IDLE;
                }
                else
                {
                    sc606regCval=--sc606regBval;
                    if(sc606regBval==0)
                        buttonlight_control = BUTTONLIGHT_CONTROL_OFF;
                    else if (buttonlight_target==sc606regBval)
                        backlight_control = BACKLIGHT_CONTROL_IDLE;
                }

                break;
            default:
                buttonlight_control = BUTTONLIGHT_CONTROL_IDLE;
                break;
        }
    }

    switch (sc606_control)
    {
        case SC606_CONTROL_IDLE:
            if(sc606_changed)
                sc606_control=SC606_CONTROL_A12;
            else
                sc606_control=SC606_CONTROL_IDLE;
            break;

        case SC606_CONTROL_A12:
            sc606_write(SC606_REG_A , sc606regAval);
            sc606_control=SC606_CONTROL_B12;
            break;

        case SC606_CONTROL_B12:
            sc606_write(SC606_REG_B , sc606regBval);
            sc606_control=SC606_CONTROL_C12;
            break;

        case SC606_CONTROL_C12:
            sc606_write(SC606_REG_C , sc606regCval);
            if(sc606_CONF_changed!=true)
            {
                sc606_changed=false;
                if(backlight_control != BACKLIGHT_CONTROL_IDLE ||
                    buttonlight_control != BUTTONLIGHT_CONTROL_IDLE)
                {
                    sc606_control=SC606_CONTROL_A12;
                }
                else
                {
                    sc606_control=SC606_CONTROL_IDLE;
                }
            }
            else
            {
                sc606_control=SC606_CONTROL_CONF;
            }
            break;

        case SC606_CONTROL_CONF:
            sc606_write(SC606_REG_CONF , sc606regCONFval);
            sc606_changed=false;
            sc606_CONF_changed=false;
            if(backlight_control != BACKLIGHT_CONTROL_IDLE ||
                buttonlight_control != BUTTONLIGHT_CONTROL_IDLE)
            {
                sc606_control=SC606_CONTROL_A12;
            }
            else
            {
                sc606_control=SC606_CONTROL_IDLE;
            }
            break;

        default:
            sc606_control=SC606_CONTROL_IDLE;
            break;
    }
}
#endif /* BOOTLOADER */

static void __backlight_dim(bool dim_now)
{
    /* dont let the interrupt tick happen */
    backlight_control = BACKLIGHT_CONTROL_IDLE;
    backlight_target = (dim_now == true) ? 0 : _backlight_brightness;
    if(backlight_target==0 && _backlight_brightness==0)
    {
        if(dim_now == false)
            backlight_control = BACKLIGHT_CONTROL_ON;
        else
            backlight_control = BACKLIGHT_CONTROL_OFF;
    }
    else
        backlight_control = BACKLIGHT_CONTROL_FADE;
}

void _backlight_on(void)
{
#ifdef HAVE_LCD_SLEEP
    backlight_lcd_sleep_countdown(false); /* stop counter */
#endif
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
    __backlight_dim(false);
}    

void _backlight_off(void)
{
    __backlight_dim(true);
#ifdef HAVE_LCD_SLEEP
    /* Disable lcd after fade completes (when lcd_sleep timeout expires) */
    backlight_lcd_sleep_countdown(true); /* start countdown */
#endif
}

static inline void __buttonlight_on(void)
{
    buttonlight_control = BUTTONLIGHT_CONTROL_ON;
}

static inline void __buttonlight_off(void)
{
    buttonlight_control = BUTTONLIGHT_CONTROL_OFF;
}

static void __buttonlight_dim(bool dim_now)
{
    buttonlight_control = BUTTONLIGHT_CONTROL_IDLE;
    buttonlight_target = (dim_now == true) ? 0 : buttonlight_brightness;
    if(buttonlight_target==0 && buttonlight_brightness==0)
    {
        if(dim_now == false)
            buttonlight_control = BUTTONLIGHT_CONTROL_ON;
        else
            buttonlight_control = BUTTONLIGHT_CONTROL_OFF;
    }
    else
        buttonlight_control = BUTTONLIGHT_CONTROL_FADE;
}

void _buttonlight_on(void)
{
    __buttonlight_dim(false);
}

void _buttonlight_off(void)
{
#ifndef BOOTLOADER
    if(_buttonlight_timeout>0)
        __buttonlight_dim(true);
    else
#endif
        __buttonlight_off();
}

void _buttonlight_set_brightness(int brightness)
{
    /* stop the interrupt from messing us up */
    buttonlight_control = BUTTONLIGHT_CONTROL_IDLE;
    buttonlight_brightness = log_brightness[brightness];
    buttonlight_control = BUTTONLIGHT_CONTROL_SET;
}

bool _backlight_init(void)
{
    unsigned char brightness = log_brightness[DEFAULT_BRIGHTNESS_SETTING];
    buttonlight_brightness = brightness;
    _backlight_brightness = brightness;

    buttonlight_control = BUTTONLIGHT_CONTROL_IDLE;
    backlight_control = BACKLIGHT_CONTROL_ON;

    /* Set the backlight up in a known state */
    sc606_init();
    sc606_write(SC606_REG_A , brightness);
    sc606_write(SC606_REG_B , 0);
    sc606_write(SC606_REG_C , 0);
    sc606_write(SC606_REG_CONF , 0x03);
#ifndef BOOTLOADER
    /* put the led control on the tick list */
    tick_add_task(led_control_service);
#endif
    return true;
}
