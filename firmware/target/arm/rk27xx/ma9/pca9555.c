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

    i2c_write(0x40, PCA9555_POL_INV_CMD, sizeof(data), data);

    data[0] = 0x00; /* port0 - pins 0-7 output */
    data[1] = 0xf8; /* port1 - pins 0-2 input, pins 3-7 output */
    i2c_write(0x40, PCA9555_CFG_CMD, sizeof(data), data);

    /*
     *   IO0-7    IO0-6   IO0-5    IO0_4    IO0_3    IO0_2     IO0_1      IO0_0
     * USB_SEL   KP_LED     POP   DAC_EN   AMP_EN   SPI_CS   SPI_CLK   SPI_DATA
     *   1:MSD    1:OFF       1        0        0        1         1          1
     */
    data[0] = ((1<<7)|(1<<6)|(1<<5)|(0<<4)|(0<<3)|(1<<2)|(1<<1)|(1<<0));

    /*
     *  IO1-2    IO1_1       IO1_0
     * CHG_EN   DAC_EN   DF1704_CS
     *      1        0           1
     */
    data[1] = ((1<<2)|(0<<1)|(1<<0));
    i2c_write(0x40, PCA9555_OUT_CMD, sizeof(data), data);
}

int pca9555_get_back_key(void)
{
    unsigned char data[2];
    if ((GPIO_PFDR&0x04) == 0) {
        i2c_read(0x40, PCA9555_IN_CMD, sizeof(data), data);
        if ((data[1]&0x80) == 0) {
            return 1;
        }
    }
    return 0;
}
