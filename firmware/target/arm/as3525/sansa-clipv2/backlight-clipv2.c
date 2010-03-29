/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2009 Rafaël Carré
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

void _backlight_on(void)
{
    ascodec_write(0x25, ascodec_read(0x25) | 2);    /* lcd power */
    ascodec_write(0x1c, 8|1);
    ascodec_write(0x1b, 0x90);
    lcd_enable(true);
}

void _backlight_off(void)
{
    lcd_enable(false);
    ascodec_write(0x25, ascodec_read(0x25) & ~2);    /* lcd power */
    ascodec_write(0x1c, 8|1);
    ascodec_write(0x1b, 0);
}

void _buttonlight_on(void)
{
    GPIOA_DIR |= (1<<5);
    GPIOA_PIN(5) = (1<<5);  /* set pin a5 high */
}

void _buttonlight_off(void)
{
    GPIOA_DIR |= (1<<5);
    GPIOA_PIN(5) = 0;       /* set pin a5 low */
}
