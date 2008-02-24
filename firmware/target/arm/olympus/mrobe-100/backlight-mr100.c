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

#include "backlight-target.h"
#include "system.h"
#include "lcd.h"
#include "backlight.h"
#include "i2c-pp.h"

void _backlight_on(void)
{
    GPO32_ENABLE |= 0x1000000;
}

void _backlight_off(void)
{
    GPO32_ENABLE &= ~0x1000000;
}

void _buttonlight_on(void)
{
    /* turn on all touchpad leds */
    GPIOA_OUTPUT_VAL |= BUTTONLIGHT_ALL;

#if 0
    /* Writing to 0x7000a010 controls the brightness of the leds.
       This routine fades the leds from dim to bright, like when
       you first turn the unit on. */
    unsigned long val = 0x80ff08ff;
    int i = 0;
    for (i = 0; i < 16; i++)
        outl(val, 0x7000a010);
        udelay(100000);
        val -= 0x110000;
    }
#endif
}

void _buttonlight_off(void)
{
    /* turn off all touchpad leds */
    GPIOA_OUTPUT_VAL &= ~BUTTONLIGHT_ALL;
}
