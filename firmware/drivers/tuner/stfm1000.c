/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by amaury Pouly
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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
#include <string.h>
#include "kernel.h"
#include "power.h"
#include "tuner.h" /* tuner abstraction interface */
#include "fmradio.h"
#include "fmradio_i2c.h" /* physical interface driver */
#include "stfm1000.h"

#define STFM100_I2C_ADDR    0xc0

#define CHIPID  0x80

static int stfm1000_read_reg(uint8_t reg, uint32_t *val)
{
    uint8_t buf[4];
    buf[0] = reg;
    int ret = fmradio_i2c_write(STFM100_I2C_ADDR, buf, 1);
    if(ret < 0) return ret;
    ret = fmradio_i2c_read(STFM100_I2C_ADDR, buf, 4);
    *val = buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
    return ret;
}

static int stfm1000_write_reg(uint8_t reg, uint32_t val)
{
    uint8_t buf[5];
    buf[0] = reg;
    buf[1] = val & 0xff; buf[2] = (val >> 8) & 0xff;
    buf[3] = (val >> 16) & 0xff; buf[4] = (val >> 24) & 0xff;
    return fmradio_i2c_write(STFM100_I2C_ADDR, buf, 5);
}

void stfm1000_dbg_info(struct stfm1000_dbg_info *nfo)
{
    memset(nfo, 0, sizeof(struct stfm1000_dbg_info));
    stfm1000_read_reg(CHIPID, &nfo->chipid);
}

void stfm1000_init(void)
{
}

int stfm1000_set(int setting, int value)
{
    (void) setting;
    (void) value;
    return -1;
}

int stfm1000_get(int setting)
{
    (void) setting;
    return -1;
}