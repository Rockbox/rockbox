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
#include "cpu.h"
#include "system.h"
#include "backlight.h"
#include "lcd.h"

void _backlight_on(void)
{
#ifdef HAVE_LCD_SLEEP
    lcd_enable(true);
    _lcd_sleep_timer = 0;
#endif
    GPIO_SET_BITWISE(GPIOL_OUTPUT_VAL, 0x20);
}

void _backlight_off(void)
{
    GPIO_CLEAR_BITWISE(GPIOL_OUTPUT_VAL, 0x20);
#ifdef HAVE_LCD_SLEEP
    lcd_enable(false);
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
