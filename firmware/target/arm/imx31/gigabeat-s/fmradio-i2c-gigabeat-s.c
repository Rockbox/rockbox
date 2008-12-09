/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Physical interface of the SI4700 in the Gigabeat S
 *
 * Copyright (C) 2008 by Nils Wallménius
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
#include "system.h"
#include "i2c-imx31.h"
#include "fmradio_i2c.h"

struct i2c_node si4700_i2c_node =
{
    .num  = I2C2_NUM,
    .ifdr = I2C_IFDR_DIV192, /* 66MHz/.4MHz = 165, closest = 192 = 343750Hz */
                             /* Just hard-code for now - scaling may require 
                              * updating */
    .addr = (0x20),
};

int fmradio_i2c_write(unsigned char address, const unsigned char* buf, int count)
{
    (void)address;
    i2c_write(&si4700_i2c_node, buf, count);
    return 0;
}

int fmradio_i2c_read(unsigned char address, unsigned char* buf, int count)
{
    (void)address;
    i2c_read(&si4700_i2c_node, -1, buf, count);
    return 0;
}

