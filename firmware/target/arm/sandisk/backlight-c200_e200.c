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
#include "as3514.h"

static unsigned short backlight_brightness = DEFAULT_BRIGHTNESS_SETTING;

void __backlight_set_brightness(int brightness)
{
    backlight_brightness = brightness;

    if (brightness > 0)
        __backlight_on();
    else
        __backlight_off();
}

void __backlight_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd */
#endif
    pp_i2c_send(AS3514_I2C_ADDR, DCDC15, backlight_brightness);
}

void __backlight_off(void)
{
    pp_i2c_send(AS3514_I2C_ADDR, DCDC15, 0x0);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false); /* power off lcd */
#endif
}

#ifdef HAVE_BUTTON_LIGHT
void __buttonlight_on(void)
{
    GPIOG_OUTPUT_VAL |= 0x80;
#ifdef SANSA_C200
    GPIOB_OUTPUT_VAL |= 0x10; /* The "menu" backlight */
#endif
}

void __buttonlight_off(void)
{
    GPIOG_OUTPUT_VAL &=~ 0x80;
#ifdef SANSA_C200
    GPIOB_OUTPUT_VAL &=~ 0x10; /* The "menu" backlight */
#endif
}
#endif
