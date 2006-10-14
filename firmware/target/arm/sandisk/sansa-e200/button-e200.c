/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* Taken from button-h10.c by Barry Wardell and reverse engineering by MrH. */

#include <stdlib.h>
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "system.h"


void button_init_device(void)
{
    /* Enable all buttons except the wheel */
    GPIOF_ENABLE |= 0xff;
}

bool button_hold(void)
{
    return (GPIOF_INPUT_VAL & 0x80)?true:false;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    unsigned char state;
    static bool hold_button = false;
    bool hold_button_old;

    /* Hold */
    hold_button_old = hold_button;
    hold_button = button_hold();

#if 0
#ifndef BOOTLOADER
    /* light handling */
    if (hold_button != hold_button_old)
    {
        backlight_hold_changed(hold_button);
    }
#endif
#endif

    /* device buttons */
    if (!hold_button)
    {
        /* Read normal buttons */
        state = GPIOF_INPUT_VAL & 0xff;
        if ((state & 0x1) == 0) btn |= BUTTON_REC;
        if ((state & 0x2) == 0) btn |= BUTTON_DOWN;
        if ((state & 0x4) == 0) btn |= BUTTON_RIGHT;
        if ((state & 0x8) == 0) btn |= BUTTON_LEFT;
        if ((state & 0x10) == 0) btn |= BUTTON_SELECT; /* The centre button */
        if ((state & 0x20) == 0) btn |= BUTTON_UP; /* The "play" button */
        if ((state & 0x40) == 1) btn |= BUTTON_POWER;
    }

    return btn;
}
