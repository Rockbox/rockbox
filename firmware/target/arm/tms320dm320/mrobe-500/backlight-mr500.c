/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
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

#include "config.h"
#include "cpu.h"
#include "system.h"
#include "backlight-target.h"
#include "backlight.h"
#include "lcd.h"
#include "power.h"
#include "spi-target.h"
#include "lcd-target.h"

short read_brightness = 0x0;

static void _backlight_write_brightness(int brightness)
{
    uint8_t bl_command[] = {0xA4, 0x00, brightness, 0xA4};
    
    uint8_t bl_read[] = {0xA8, 0x00};
    
    spi_block_transfer(SPI_target_BACKLIGHT, bl_read, 2, (char*)&read_brightness, 2);
    
    spi_block_transfer(SPI_target_BACKLIGHT, bl_command, 4, 0, 0);
}

void _backlight_on(void)
{
    lcd_awake(); /* power on lcd + visible display */

#if (CONFIG_BACKLIGHT_FADING == BACKLIGHT_NO_FADING)
    _backlight_write_brightness(backlight_brightness);
#endif
}

void _backlight_off(void)
{
    _backlight_write_brightness(0);
}

/* Assumes that the backlight has been initialized */
void _backlight_set_brightness(int brightness)
{
    _backlight_write_brightness(brightness);
}

void __backlight_dim(bool dim_now)
{
    _backlight_set_brightness(dim_now ?
        DEFAULT_BRIGHTNESS_SETTING :
        DEFAULT_DIMNESS_SETTING);
}

bool _backlight_init(void)
{
    _backlight_set_brightness(backlight_brightness);
    return true;
}
