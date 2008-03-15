/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "system.h"
#include "button.h"
#include "backlight.h"
#include "adc.h"

static bool hold_button        = false;
static bool remote_hold_button = false;

void button_init_device(void)
{
    /* Remote Play */
    GPIO_FUNCTION |= 0x80000000;
    GPIO_ENABLE &= ~0x80000000;
}

bool button_hold(void)
{
    return (GPIO1_READ & 0x00000200) == 0;
}

bool remote_button_hold(void)
{
    return remote_hold_button;
}

int button_read_device(void)
{
    int  btn = BUTTON_NONE;
    bool hold_button_old;
    bool remote_hold_button_old;
    int  data;

    /* normal buttons */
    hold_button_old = hold_button;
    hold_button = button_hold();

    if (!hold_button)
    {
        data = adc_scan(ADC_BUTTONS);

        if (data < 0xc0)
        {
            if (data < 0x67)
                if (data < 0x37)
                    btn = BUTTON_VOL_DOWN;
                else
                    if (data < 0x51)
                        btn = BUTTON_MODE;
                    else
                        btn = BUTTON_VOL_UP;
            else
                if (data < 0x7f)
                    btn = BUTTON_REC;
                else
                    if (data < 0x98)
                        btn = BUTTON_LEFT;
                    else
                        btn = BUTTON_RIGHT;
        }
        if (!(GPIO1_READ & 0x00000002))
            btn |= BUTTON_PLAY;
    }

    /* remote buttons */
    data = remote_detect() ? adc_scan(ADC_REMOTE) : 0xff;

    remote_hold_button_old = remote_hold_button;
    remote_hold_button = data < 0x14;

#ifndef BOOTLOADER
    if (remote_hold_button != remote_hold_button_old)
        backlight_hold_changed(remote_hold_button);
#endif

    if (!remote_hold_button)
    {
        if (data < 0xd0)
        {
            if (data < 0x67)
                if (data < 0x37)
                    btn |= BUTTON_RC_FF;
                else
                    if (data < 0x51)
                        btn |= BUTTON_RC_REW;
                    else
                        btn |= BUTTON_RC_MODE;
            else
                if (data < 0x98)
                    if (data < 0x7f)
                        btn |= BUTTON_RC_REC;
                    else
                        btn |= BUTTON_RC_MENU;
                else
                    if (data < 0xb0)
                        btn |= BUTTON_RC_VOL_UP;
                    else
                        btn |= BUTTON_RC_VOL_DOWN;
        }
        if ((GPIO_READ & 0x80000000) == 0)
            btn |= BUTTON_RC_PLAY;
    }

    return btn;
}
