/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bertrik Sikken
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
#include "kernel.h"
#include "s5l8700.h"
#include "i2c-s5l8700.h"
#include "uda1380.h"
#include "udacodec.h"

void udacodec_reset(void)
{
    /* Reset codec using GPIO P5.0 */
    PCON5 = (PCON5 & ~0xF) | 1;
    PDAT5 |= (1 << 0);
    sleep(HZ/100);
    
    /* Set codec reset pin inactive */
    PDAT5 &= ~(1 << 0);
}

int udacodec_write(unsigned char reg, unsigned short value)
{
    unsigned char data[2];
    
    data[0] = value >> 8;
    data[1] = value & 0xFF;

    return i2c_write(UDA1380_ADDR, reg, 2, data);
}

int udacodec_write2(unsigned char reg,
                    unsigned short value1, unsigned short value2)
{
    unsigned char data[4];
    
    data[0] = value1 >> 8;
    data[1] = value1 & 0xFF;
    data[2] = value2 >> 8;
    data[3] = value2 & 0xFF;
    
    return i2c_write(UDA1380_ADDR, reg, 4, data);
}

