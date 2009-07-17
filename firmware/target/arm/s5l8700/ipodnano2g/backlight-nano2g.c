/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Dave Chapman
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
#include <stdbool.h>

#include "config.h"
#include "backlight.h"
#include "backlight-target.h"
#include "i2c-s5l8700.h"


static unsigned char bl_i2c_readbyte(int address)
{
    unsigned char tmp;

    i2c_read(0xe6, address, 1, &tmp);

    return tmp;
}

static void bl_i2c_writebyte(int address, unsigned char val)
{
    i2c_write(0xe6, address, 1, &val);
}

void _backlight_set_brightness(int brightness)
{
    (void)brightness;
}

void _backlight_on(void)
{
    bl_i2c_writebyte(0x2b,10);
    bl_i2c_writebyte(0x2a,6);
    bl_i2c_writebyte(0x28,0x2e);
    bl_i2c_writebyte(0x29,(bl_i2c_readbyte(0x29)&0xf0)|1);
    bl_i2c_writebyte(0x2a,7);
}

void _backlight_off(void)
{
    bl_i2c_writebyte(0x2b,10);
    bl_i2c_writebyte(0x29,bl_i2c_readbyte(0x29)&0xf0);
    bl_i2c_writebyte(0x2a,7);
}

bool _backlight_init(void)
{
    _backlight_on();
}
