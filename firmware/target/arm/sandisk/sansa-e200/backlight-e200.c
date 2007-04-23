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
#include "backlight-target.h"
#include "system.h"
#include "lcd.h"
#include "backlight.h"
#include "i2c-pp.h"

static unsigned short backlight_brightness;
static bool backlight_is_on;

int __backlight_is_on(void)
{
    return (int)backlight_is_on;
}

void __backlight_set_brightness(int brightness)
{
    backlight_brightness = brightness;
    pp_i2c_send( 0x46, 0x23, backlight_brightness);
    backlight_is_on = true;
}

void __backlight_on(void)
{
    lcd_enable(true); /* power on lcd */
    pp_i2c_send( 0x46, 0x23, backlight_brightness);
    backlight_is_on = true;
}

void __backlight_off(void)
{
    pp_i2c_send( 0x46, 0x23, 0x0);
    lcd_enable(false); /* power off lcd */
    backlight_is_on = false;
}


void __button_backlight_on(void)
{
    GPIOG_OUTPUT_VAL |=0x80;
}

void __button_backlight_off(void)
{
    GPIOG_OUTPUT_VAL &=~ 0x80;
}
