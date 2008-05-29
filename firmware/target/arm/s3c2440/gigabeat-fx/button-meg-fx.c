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
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "adc.h"
#include "system.h"
#include "backlight-target.h"

static bool headphones_detect;
static bool hold_button        = false;

#define TOUCHPAD_SENS_NORMAL ((1 << 12) | /* right++ */ \
                              (1 <<  7) | /* left++ */ \
                              (1 <<  6) | /* down++*/ \
                              (1 <<  0) | /* up++ */ \
                              (1 <<  3))  /* center */

#define TOUCHPAD_SENS_HIGH   (((1 << 12) | (1 << 11)) | /* right++, right+ */ \
                              ((1 <<  8) | (1 <<  7)) | /* left+, left++ */ \
                              ((1 <<  6) | (1 <<  5)) | /* down++, down+ */ \
                              ((1 <<  1) | (1 <<  0)) | /* up+, up++ */ \
                                           (1 <<  3))   /* Center */

static int touchpad_mask = TOUCHPAD_SENS_NORMAL;

static int const remote_buttons[] =
{
    BUTTON_NONE,        /* Headphones connected - remote disconnected */
    BUTTON_RC_PLAY,
    BUTTON_RC_DSP,
    BUTTON_RC_REW,
    BUTTON_RC_FF,
    BUTTON_RC_VOL_UP,
    BUTTON_RC_VOL_DOWN,
    BUTTON_NONE,        /* Remote control attached - no buttons pressed */
    BUTTON_NONE,        /* Nothing in the headphone socket */
};

void button_init_device(void)
{
    /* Some button inputs need the internal pullups disabled to function */
    GPGUP|=0x031F;
    GPJUP|=0x1FFF;
}

inline bool button_hold(void)
{
    return (GPGDAT & (1 << 15));
}

int button_read_device(void)
{
    int touchpad;
    int buttons;
    static int lastbutton;
    unsigned short remote_adc;
    int btn = BUTTON_NONE;
    bool hold_button_old;

    /* normal buttons */
    hold_button_old = hold_button;
    hold_button = button_hold();

#ifndef BOOTLOADER
    /* give BL notice if HB state chaged */
    if (hold_button != hold_button_old)
        backlight_hold_changed(hold_button);
#endif

    /* See header for ADC values when remote control buttons are pressed */
    /* Only one button can be sensed at a time on the remote. */
    /* Need to filter the remote button because the ADC is so fast */
    remote_adc = adc_read(ADC_HPREMOTE);

    if (remote_adc != ADC_READ_ERROR)
    {
        /* If there is nothing in the headphone socket, the ADC reads high */
        if (remote_adc < 940)
            headphones_detect = true;
        else
            headphones_detect = false;
    }

    btn = remote_buttons[(remote_adc + 64) / 128];
    if (btn != lastbutton)
    {
        /* if the buttons dont agree twice in a row, then its none */
        lastbutton = btn;
        btn = BUTTON_NONE;
        buttonlight_on();
    }

    /* Check for hold first - exit if asserted with no button pressed */
    if (hold_button)
        return btn;

    /* the side buttons - Check before doing all of the work on each bit */
    buttons = GPGDAT & 0x1F;
    if (buttons)
    {
        btn |= buttons;
        buttonlight_on();
    }
    
    /* the touchpad - only watch the lines we actually read */
    touchpad = GPJDAT & touchpad_mask;

    if (touchpad)
    {
        if (touchpad & (1 << 3))
        {
            btn |= BUTTON_SELECT;
            /* Desensitize all but outer detectors and still allow a combo if
             * that's really intended. */
            touchpad &= TOUCHPAD_SENS_NORMAL;
        }

        /* Simply include all lines in checks since "touchpad" has been
         * masked to desired sensitivity already - allows any mask to be
         * used without changing this code. */
        if (touchpad & ((1 << 2) | (1 << 1) | (1 << 0)))
            btn |= BUTTON_UP;

        if (touchpad & ((1 << 12) | (1 << 11) | (1 << 10)))
            btn |= BUTTON_RIGHT;

        if (touchpad & ((1 << 6) | (1 << 5) | (1 << 4)))
            btn |= BUTTON_DOWN;

        if (touchpad & ((1 << 9) | (1 << 8) | (1 << 7)))
            btn |= BUTTON_LEFT;

        buttonlight_on();
    }
    
    return btn;
}

void touchpad_set_sensitivity(int level)
{
    static const int masks[] =
    {
        TOUCHPAD_SENS_NORMAL,
        TOUCHPAD_SENS_HIGH
    };

    if ((unsigned)level >= ARRAYLEN(masks))
        level = 0;

    touchpad_mask = masks[level];
}

bool headphones_inserted(void)
{
    return headphones_detect;
}
