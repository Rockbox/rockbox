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

#include "system.h"
#include "i2c-rk27xx.h"
#include "pca9555.h"

void pca9555_init()
{
    unsigned char data[2] = {0, 0};

    i2c_write(0x40, 4, 2, data);
    data[0] = 0x00;
    data[1] = 0xff^0x07;
    i2c_write(0x40, 6, 2, data);

    data[0] = ((1<<7)|(1<<6)|(1<<5)|(0<<4)|(0<<3)|(1<<2)|(1<<1)|(1<<0));
    /*
     * IO0-7         IO0-6   IO0-5   IO0_4   IO0_3   IO0_2   IO0_1   IO0_0
     * USB_SEL       KP_LED  POP     DAC_EN  AMP_EN  SPI_CS  SPI_CLK SPI_DATA
     * 1:MSD         1: OFF  1       1        1        1      1         1
     */
    data[1] = ((1<<2)|(0<<1)|(1<<0));
    /* IO1-2         IO1_1   IO1_0
     * CHG_EN        DAC_EN  1704_CS
     * 1              0       1
     */
    i2c_write(0x40, 2, 2, data);
}

int pca9555_get_back_key(void)
{
    unsigned char data[2];

    if ((GPIO_PFDR&(1<<2)) == 0) {
        i2c_read(0x40, 0, 2, data);
        return ((data[1]&0x80) == 0);
    }
    return 0;
}
