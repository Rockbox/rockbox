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
#include "config.h"
#include "backlight-target.h"
#include "system.h"
#include "lcd.h"
#include "backlight.h"
#include "i2c-pp.h"
#include "as3514.h"

static unsigned short backlight_brightness = DEFAULT_BRIGHTNESS_SETTING;

void _backlight_set_brightness(int brightness)
{
    backlight_brightness = brightness;

    if (brightness > 0)
        _backlight_on();
    else
        _backlight_off();
}

void _backlight_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd */
#endif
#if defined(HAVE_LCD_SLEEP) && !defined(BOOTLOADER)
    _lcd_sleep_timer = 0; /* LCD should be awake already */
#endif
    pp_i2c_send(AS3514_I2C_ADDR, DCDC15, backlight_brightness);
}

void _backlight_off(void)
{
    pp_i2c_send(AS3514_I2C_ADDR, DCDC15, 0x0);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false); /* power off lcd */
#endif
#if defined(HAVE_LCD_SLEEP) && !defined(BOOTLOADER)
    /* Start LCD sleep countdown */
    if (_lcd_sleep_timeout < 0)
    {
        _lcd_sleep_timer = 0; /* Setting == Always */
        lcd_sleep();
    }
    else
        _lcd_sleep_timer = _lcd_sleep_timeout;
#endif
}

void _buttonlight_on(void)
{
    GPIO_SET_BITWISE(GPIOG_OUTPUT_VAL, 0x80);
#ifdef SANSA_C200
    GPIO_SET_BITWISE(GPIOB_OUTPUT_VAL, 0x10); /* The "menu" backlight */
#endif
}

void _buttonlight_off(void)
{
    GPIO_CLEAR_BITWISE(GPIOG_OUTPUT_VAL, 0x80);
#ifdef SANSA_C200
    GPIO_CLEAR_BITWISE(GPIOB_OUTPUT_VAL, 0x10); /* The "menu" backlight */
#endif
}
