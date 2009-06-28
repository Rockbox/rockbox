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
#include "i2c-coldfire.h"
#include "pcf50606.h"
#include "uda1380.h"
#include "udacodec.h"

void udacodec_reset(void)
{
#ifdef IRIVER_H300_SERIES
    int mask = disable_irq_save();
    pcf50606_write(0x3b, 0x00);  /* GPOOD2 high Z */
    pcf50606_write(0x3b, 0x07);  /* GPOOD2 low */
    restore_irq(mask);
#else
    /* RESET signal */
    or_l(1<<29, &GPIO_OUT);
    or_l(1<<29, &GPIO_ENABLE);
    or_l(1<<29, &GPIO_FUNCTION);
    sleep(HZ/100);
    and_l(~(1<<29), &GPIO_OUT);
#endif
}

int udacodec_write(unsigned char reg, unsigned short value)
{
    unsigned char data[3];

    data[0] = reg;
    data[1] = value >> 8;
    data[2] = value & 0xff;

    if (i2c_write(I2C_IFACE_0, UDA1380_ADDR, data, 3) != 3) {
        return -1;
    }
    return 0;
}

int udacodec_write2(unsigned char reg, 
                    unsigned short value1, unsigned short value2) 
{
    unsigned char data[5];
    
    data[0] = reg;
    data[1] = value1 >> 8;
    data[2] = value1 & 0xFF;
    data[3] = value2 >> 8;
    data[4] = value2 & 0xFF;

    if (i2c_write(I2C_IFACE_0, UDA1380_ADDR, data, 5) != 5) {
        return -1;
    }
    return 0;
}                            

