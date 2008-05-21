/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Mark Arigo
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "system.h"
#include "button.h"
#include "backlight.h"

void button_init_device(void)
{
    /* TODO...for now, hardware initialisation is done by the c200 bootloader */
}

bool button_hold(void)
{
    return !(GPIOL_INPUT_VAL & 0x40);
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    static bool hold_button = false;
    bool hold_button_old;

    /* Hold */
    hold_button_old = hold_button;
    hold_button = button_hold();

#ifndef BOOTLOADER
    if (hold_button != hold_button_old)
        backlight_hold_changed(hold_button);
#endif

    /* device buttons */
    if (!hold_button)
    {
#if 0
        if (!(GPIOB_INPUT_VAL & 0x20)) btn |= BUTTON_POWER;
        if (!(GPIOF_INPUT_VAL & 0x10)) btn |= BUTTON_VOL_UP;
        if (!(GPIOF_INPUT_VAL & 0x04)) btn |= BUTTON_VOL_DOWN;
#endif
        /* A hack until the touchpad works */
        if (!(GPIOB_INPUT_VAL & 0x20)) btn |= BUTTON_SELECT;
        if (!(GPIOF_INPUT_VAL & 0x10)) btn |= BUTTON_UP;
        if (!(GPIOF_INPUT_VAL & 0x04)) btn |= BUTTON_DOWN;
    }

    return btn;
}

bool headphones_inserted(void)
{
    return (GPIOB_INPUT_VAL & 0x10) ? false : true;
}
