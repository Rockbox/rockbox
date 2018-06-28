/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
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

#include "system.h"
#include "cs4398.h"
#include "config.h"
#include "audio.h"
#include "audiohw.h"
#include "i2c.h"

void cs4398_write_reg(uint8_t reg, uint8_t val)
{
    unsigned char buf[2] = {reg, val};
    i2c_write(CS4398_I2C_ADDR, (unsigned char *)&buf, 2);
}

uint8_t cs4398_read_reg(uint8_t reg)
{
    unsigned char buf[2] = {reg, 0xff};
    i2c_write(CS4398_I2C_ADDR, (unsigned char *)&buf[0], 1);
    i2c_read(CS4398_I2C_ADDR, (unsigned char *)&buf[1], 1);
    return buf[1];
}
