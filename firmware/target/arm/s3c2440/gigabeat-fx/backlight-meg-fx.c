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
static unsigned short buttonlight_brightness;
static unsigned short backlight_target;
static unsigned short buttonlight_target;

static enum backlight_states
{
    BACKLIGHT_CONTROL_IDLE,
    BACKLIGHT_CONTROL_OFF,
    BACKLIGHT_CONTROL_ON,
    BACKLIGHT_CONTROL_SET,
    BACKLIGHT_CONTROL_FADE
} backlight_control;

static enum sc606_states
{
    SC606_CONTROL_IDLE,
    SC606_CONTROL_A12,
    SC606_CONTROL_B12,
    SC606_CONTROL_C12,
    SC606_CONTROL_CONF
} sc606_control;

enum buttonlight_states
{
    BUTTONLIGHT_CONTROL_IDLE,
    BUTTONLIGHT_CONTROL_OFF,
    BUTTONLIGHT_CONTROL_ON,
    BUTTONLIGHT_CONTROL_SET,
    BUTTONLIGHT_CONTROL_FADE,
} buttonlight_control;

static unsigned short buttonlight_trigger_now;

#define CHARGING_LED_COUNT 60
unsigned char charging_leds[] = { 0x00, 0x20, 0x38, 0x3C };

bool __backlight_init(void)
{
    backlight_brightness=DEFAULT_BRIGHTNESS_SETTING;
    backlight_control = BACKLIGHT_CONTROL_IDLE;

    buttonlight_control = BUTTONLIGHT_CONTROL_IDLE;

    /* put the led control on the tick list */
    tick_add_task(led_control_service);

    return true;
}

void __backlight_on(void)
{
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
    backlight_brightness = brightness;
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

/*
 * The button lights have 'modes' of operation. Each mode must setup and
 * execute its own operation - taking care that this is all done in an ISR.
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
 */
static void led_control_service(void)
{
    static unsigned char
        sc606regAval=DEFAULT_BRIGHTNESS_SETTING,
        sc606regBval=DEFAULT_BRIGHTNESS_SETTING,
        sc606regCval=DEFAULT_BRIGHTNESS_SETTING,
        sc606regCONFval=3;

    static bool sc606_changed=true;

    if(sc606_changed==false)
    {
        switch (backlight_control)
        {
            case BACKLIGHT_CONTROL_IDLE:
                backlight_control = BACKLIGHT_CONTROL_IDLE;
                break;
            case BACKLIGHT_CONTROL_OFF:
                sc606_changed=true;
                sc606regCONFval &= ~0x03;
                backlight_control = BACKLIGHT_CONTROL_IDLE;
                break;
            case BACKLIGHT_CONTROL_ON:
                sc606_changed=true;
                sc606regCONFval |= 0x03;
                backlight_control = BACKLIGHT_CONTROL_IDLE;
                break;
            case BACKLIGHT_CONTROL_SET:
                sc606regAval=backlight_brightness;
                sc606_changed=true;
                backlight_control = BACKLIGHT_CONTROL_ON;
                break;
            case BACKLIGHT_CONTROL_FADE:
                /* Was this mode set while the backlight is already on/off? */
                if(backlight_target==sc606regAval)
                {
                    backlight_control = BACKLIGHT_CONTROL_IDLE;
                    break;
                }
                sc606_changed=true;
                sc606regCONFval |= 0x03;
                if(backlight_target>sc606regAval)
                {
                    sc606regAval++;
                    if(backlight_target==sc606regAval)
                        backlight_control = BACKLIGHT_CONTROL_ON;
                }
                else
                {
                    sc606regAval--;
                    if(backlight_target==sc606regAval)
                        backlight_control = BACKLIGHT_CONTROL_OFF;
                }

                break;
            default:
                break;
        }
        switch (buttonlight_control)
        {
            case BUTTONLIGHT_CONTROL_IDLE:
                buttonlight_control=BUTTONLIGHT_CONTROL_IDLE;
                break;
            case BUTTONLIGHT_CONTROL_OFF:
                sc606_changed=true;
                sc606regCONFval &= ~0x3C;
                buttonlight_control=BUTTONLIGHT_CONTROL_IDLE;
                break;
            case BUTTONLIGHT_CONTROL_ON:
                sc606_changed=true;
                sc606regBval=sc606regCval=buttonlight_brightness;
                sc606regCONFval |= 0x3C;
                buttonlight_control=BUTTONLIGHT_CONTROL_IDLE;
                break;
            case BUTTONLIGHT_CONTROL_SET:
                sc606regBval=sc606regCval=buttonlight_brightness;
                sc606_changed=true;
                buttonlight_control = BUTTONLIGHT_CONTROL_ON;
                break;
            case BUTTONLIGHT_CONTROL_FADE:
                /* Was this mode set while the button light is already on/off? */
                if(buttonlight_target==sc606regBval)
                {
                    buttonlight_control=BUTTONLIGHT_CONTROL_IDLE;
                    break;
                }
                sc606_changed=true;
                sc606regCONFval |= 0x3C;
                if(buttonlight_target>sc606regBval)
                {
                    sc606regCval=++sc606regBval;
                    if(buttonlight_target==sc606regBval)
                        buttonlight_control = BUTTONLIGHT_CONTROL_ON;
                }
                else
                {
                    sc606regCval=--sc606regBval;
                    if(buttonlight_target==sc606regBval)
                        buttonlight_control = BUTTONLIGHT_CONTROL_OFF;
                }

                break;
            default:
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
            sc606_control=SC606_CONTROL_CONF;
            break;
        case SC606_CONTROL_CONF:
            sc606_write(SC606_REG_CONF , sc606regCONFval);
            sc606_changed=false;
            if(backlight_control != BACKLIGHT_CONTROL_IDLE && buttonlight_control != BUTTONLIGHT_CONTROL_IDLE)
                sc606_control=SC606_CONTROL_A12;
            else
                sc606_control=SC606_CONTROL_IDLE;
            break;
        default:
            sc606_control=SC606_CONTROL_A12;
            break;
    }

    if(sc606regCONFval&0x03)
        lcd_enable(true);
    else
        lcd_enable(false);
}

void __button_backlight_on(void)
{
    buttonlight_control = BUTTONLIGHT_CONTROL_IDLE;
    buttonlight_target = buttonlight_brightness;
    if(buttonlight_brightness==0)
        buttonlight_control = BUTTONLIGHT_CONTROL_ON;
    else
        buttonlight_control = BUTTONLIGHT_CONTROL_FADE;
}

void __button_backlight_off(void)
{
    buttonlight_control = BUTTONLIGHT_CONTROL_IDLE;
    buttonlight_target = 0;
    if(buttonlight_brightness==0)
        buttonlight_control = BUTTONLIGHT_CONTROL_OFF;
    else
        buttonlight_control = BUTTONLIGHT_CONTROL_FADE;
}

void __backlight_dim(bool dim_now)
{
    /* dont let the interrupt tick happen */
    backlight_control = BACKLIGHT_CONTROL_IDLE;
    backlight_target = (dim_now == true) ? 0 : backlight_brightness;
    if(backlight_target==0 && backlight_brightness==0)
    {
        if(dim_now == false)
            backlight_control = BACKLIGHT_CONTROL_ON;
        else
            backlight_control = BACKLIGHT_CONTROL_OFF;
    }
    else
        backlight_control = BACKLIGHT_CONTROL_FADE;
}

void __buttonlight_set_brightness(int brightness)
{
    /* stop the interrupt from messing us up */
    buttonlight_control = BUTTONLIGHT_CONTROL_IDLE;
    buttonlight_brightness = brightness;
    buttonlight_control = BUTTONLIGHT_CONTROL_SET;
}
