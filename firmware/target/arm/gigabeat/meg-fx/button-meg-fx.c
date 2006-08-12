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
}

bool button_hold(void)
{
    return (GPGDAT & (1 << 15));
}

int button_read_device(void)
{
    int btn = BUTTON_NONE;
    int touchpad = GPJDAT;
    int buttons = GPGDAT;

    /* Check for hold first */
    if (buttons & (1 << 15))
        return btn;

    /* the side buttons */
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

    /* the touchpad */
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

    return btn;
}

