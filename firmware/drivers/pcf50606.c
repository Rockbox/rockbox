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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "pcf50606.h"
#include "i2c.h"

#define PCF50606_ADDR 0x10

int pcf50606_write(int address, unsigned char val)
{
    unsigned char data[] = { address, val };
    return i2c_write(PCF50606_ADDR, data, 2);
}

int pcf50606_write_multiple(int address, const unsigned char* buf, int count)
{
    int i;

    for (i = 0; i < count; i++)
        pcf50606_write(address + i, buf[i]);

    return 0;
}

int pcf50606_read(int address)
{
    unsigned char val = -1;
    i2c_readmem(PCF50606_ADDR, address, &val, 1);
    return val;
}

int pcf50606_read_multiple(int address, unsigned char* buf, int count)
{
    return i2c_readmem(PCF50606_ADDR, address, buf, count);
}

void pcf50606_init(void)
{
    // TODO
}
