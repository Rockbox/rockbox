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

#include <stdlib.h>
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "backlight-target.h"
#include "system.h"

void button_init_device(void)
{
    /* taken from the mr-100 bootloader (offset 0x1e72) */
    //~ DEV_EN |= 0x20000;                  /* enable the touchpad ?? */

    /* enable touchpad leds */
    GPIOA_ENABLE |= 0xff;
    GPIOA_OUTPUT_EN  |= BUTTONLIGHT_ALL;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    
    if(~GPIOA_INPUT_VAL & 0x40)
        btn |= BUTTON_POWER;
    
    return btn;
}

bool button_hold(void)
{
    return (GPIOD_INPUT_VAL & 0x10) ? false : true;
}

bool headphones_inserted(void)
{
    return (GPIOD_INPUT_VAL & 0x80) ? true : false;
}
