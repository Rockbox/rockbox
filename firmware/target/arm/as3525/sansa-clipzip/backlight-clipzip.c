/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2011 Bertrik Sikken
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "backlight-target.h"
#include "lcd.h"
#include "as3525v2.h"
#include "ascodec-target.h"

void _backlight_init()
{
    /* GPIO B1 controls backlight */
    GPIOB_DIR |= (1 << 1);
}

void _backlight_on(void)
{
    GPIOB_PIN(1) = (1 << 1);
    
    ascodec_write_pmu(AS3543_BACKLIGHT, 1, 0x91);
    sleep(1);
    ascodec_write_pmu(AS3543_BACKLIGHT, 1, 0x91);
    sleep(1);
    ascodec_write_pmu(AS3543_BACKLIGHT, 1, 0x91);

#ifdef HAVE_LCD_ENABLE
    lcd_enable(true);
#endif
}

void _backlight_off(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false);
#endif

    GPIOB_PIN(1) = 0;
    
    ascodec_write_pmu(AS3543_BACKLIGHT, 1, 0x91);
}

