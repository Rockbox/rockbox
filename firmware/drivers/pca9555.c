/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 Andrew Ryabinin
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

#include "i2c-rk27xx.h"
#include "pca9555.h"
#include "system.h"

static unsigned short pca9555_out_ports;
static unsigned short pca9555_config = 0xffff;

unsigned short pca9555_read_input(void)
{
    unsigned short ret;
    i2c_read(PCA9555_I2C_ADDR, PCA9555_IN_CMD, sizeof(ret), (void*)&ret);
    return letoh16(ret);
}

unsigned short pca9555_read_output(void)
{
    return pca9555_out_ports;
}

unsigned short pca9555_read_config(void)
{
    return pca9555_config;
}

void pca9555_write_output(const unsigned short data, const unsigned short mask)
{
    unsigned short le_data = htole16((data&mask) | (pca9555_out_ports&~(mask)));
    i2c_write(PCA9555_I2C_ADDR, PCA9555_OUT_CMD, sizeof(le_data), (void*)&le_data);
    pca9555_out_ports = letoh16(le_data);
}

void pca9555_write_config(const unsigned short data, const unsigned short mask)
{
    unsigned short le_data = htole16((data&mask) | (pca9555_config&~(mask)));
    i2c_write(PCA9555_I2C_ADDR, PCA9555_CFG_CMD, sizeof(le_data), (void*)&le_data);
    pca9555_config = letoh16(le_data);
}


void pca9555_init(void)
{
    pca9555_target_init();
}
