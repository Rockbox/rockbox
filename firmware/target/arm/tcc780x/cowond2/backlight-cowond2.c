/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Rob Purchase
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
#include "backlight.h"
#include "pcf50606.h"
#include "tcc780x.h"

static unsigned short backlight_brightness = DEFAULT_BRIGHTNESS_SETTING;

int _backlight_init(void)
{
    _backlight_set_brightness(DEFAULT_BRIGHTNESS_SETTING);
    return true;
}

void _backlight_set_brightness(int brightness)
{
    backlight_brightness = brightness;

    int level = disable_irq_save();
    pcf50606_write(PCF5060X_PWMC1, 0xe1 | (14-backlight_brightness)<<1);
    pcf50606_write(PCF5060X_GPOC1, 0x3);
    restore_irq(level);
    
    if (brightness > 0)
        _backlight_on();
    else
        _backlight_off();
}

void _backlight_on(void)
{
    GPIOA_SET = (1<<6);
}

void _backlight_off(void)
{
    GPIOA_CLEAR = (1<<6);
}
