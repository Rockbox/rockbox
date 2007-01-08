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

#include <stdlib.h>
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

static int const remote_buttons[] =
{
    BUTTON_NONE,    /* Headphones connected - remote disconnected */
    BUTTON_SELECT,
    BUTTON_MENU,    /* could be changed to BUTTON_A */
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_UP,      /* could be changed to BUTTON_VOL_UP */
    BUTTON_DOWN,    /* could be changed to BUTTON_VOL_DOWN */
    BUTTON_NONE,    /* Remote control attached - no buttons pressed */
    BUTTON_NONE,    /* Nothing in the headphone socket */
};





void button_init_device(void)
{
    /* Power, Remote Play & Hold switch */
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

    /* See header for ADC values when remote control buttons are pressed */
    /* Only one button can be sensed at a time on the remote. */
    /* Need to filter the remote button because the ADC is so fast */
    remote_adc = adc_read(ADC_HPREMOTE);
    btn = remote_buttons[(remote_adc + 64) / 128];
    if (btn != lastbutton)
    {
        /* if the buttons dont agree twice in a row, then its none */
        lastbutton = btn;
        btn = BUTTON_NONE;
    }

    /* Check for hold first - exit if asserted with no button pressed */
    if (button_hold())
        return btn;

    /* the side buttons - Check before doing all of the work on each bit */
    buttons = GPGDAT & 0x1F;
    if (buttons)
    {
        if (buttons & (1 << 0))
            btn |= BUTTON_POWER;

        if (buttons & (1 << 1))
            btn |= BUTTON_MENU;

        if (buttons & (1 << 2))
            btn |= BUTTON_VOL_UP;

        if (buttons & (1 << 3))
            btn |= BUTTON_VOL_DOWN;

        if (buttons & (1 << 4))
            btn |= BUTTON_A;
    }
    
    /* the touchpad */
    touchpad = GPJDAT & 0x10C9;
    if (touchpad)
    {
        if (touchpad & (1 << 0))
            btn |= BUTTON_UP;

        if (touchpad & (1 << 12))
            btn |= BUTTON_RIGHT;

        if (touchpad & (1 << 6))
            btn |= BUTTON_DOWN;

        if (touchpad & (1 << 7))
            btn |= BUTTON_LEFT;

        if (touchpad & (1 << 3))
            btn |= BUTTON_SELECT;
    }
    
    return btn;
}



bool headphones_inserted(void)
{
    unsigned short remote_adc = adc_read(ADC_HPREMOTE);
    if (remote_adc != ADC_READ_ERROR)
    {
        /* If there is nothing in the headphone socket, the ADC reads high */
        if (remote_adc < 940)
            headphones_detect = true;
        else
            headphones_detect = false;
    }
    return headphones_detect;
}
