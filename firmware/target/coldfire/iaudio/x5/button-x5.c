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

void button_init_device(void)
{
    /* Power, Remote Play & Hold switch */
    GPIO_FUNCTION |= 0x0e000000;
    GPIO_ENABLE &= ~0x0e000000;
}

bool button_hold(void)
{
    return (GPIO_READ & 0x08000000)?false:true;
}

bool remote_button_hold(void)
{
    return false; /* TODO X5 */
}

int button_read_device(void)
{
    int data;
    int btn = BUTTON_NONE;
    static bool hold_button = false;
    static bool remote_hold_button = false;
    bool hold_button_old;

    /* normal buttons */
    hold_button_old = hold_button;
    hold_button = button_hold();

    /* give BL notice if HB state chaged */
    if (hold_button != hold_button_old)
        backlight_hold_changed(hold_button);

    if (!hold_button)
    {
        data = adc_scan(ADC_BUTTONS);
        if (data < 0xf0)
        {
            if(data < 0x7c)
                if(data < 0x42)
                    btn = BUTTON_LEFT;
                else
                    if(data < 0x62)
                        btn = BUTTON_RIGHT;
                    else
                        btn = BUTTON_SELECT;
            else
                if(data < 0xb6)
                    if(data < 0x98)
                        btn = BUTTON_REC;
                    else
                        btn = BUTTON_PLAY;
                else
                    if(data < 0xd3)
                        btn = BUTTON_DOWN;
                    else
                        btn = BUTTON_UP;
        }
    }

    /* remote buttons */

    /* TODO: add light handling for the remote */

    remote_hold_button = remote_button_hold();

    data = adc_scan(ADC_REMOTE);
    if(data < 0x17)
        remote_hold_button = true;

    if(!remote_hold_button)
    {
        if (data < 0xee)
        {
            if(data < 0x7a)
                if(data < 0x41)
                    btn |= BUTTON_RC_REW;
                else
                    if(data < 0x61)
                        btn |= BUTTON_RC_FF;
                    else
                        btn |= BUTTON_RC_MODE;
            else
                if(data < 0xb4)
                    if(data < 0x96)
                        btn |= BUTTON_RC_REC;
                    else
                        btn |= BUTTON_RC_MENU;
                else
                    if(data < 0xd1)
                        btn |= BUTTON_RC_VOL_UP;
                    else
                        btn |= BUTTON_RC_VOL_DOWN;
        }
    }

    data = GPIO_READ;
    if (!(data & 0x04000000))
        btn |= BUTTON_POWER;

    if (!(data & 0x02000000))
        btn |= BUTTON_RC_PLAY;

    return btn;
}
