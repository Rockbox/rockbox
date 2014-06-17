/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Physical interface of the Philips TEA5767 in Samsung YH-92x
 *
 * Copyright (C) 2014 by Szymon Dziok
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
#include "cpu.h"
#include "fmradio_3wire.h"

#define CLOCK_EN      GPIO_SET_BITWISE(GPIOL_ENABLE, 0x10)
#define CLOCK_OUT     GPIO_SET_BITWISE(GPIOL_OUTPUT_EN, 0x10)
#define CLOCK_LO      GPIO_CLEAR_BITWISE(GPIOL_OUTPUT_VAL, 0x10)
#define CLOCK_HI      GPIO_SET_BITWISE(GPIOL_OUTPUT_VAL, 0x10)

#define DATA          (GPIOL_INPUT_VAL & 0x08)
#define DATA_EN       GPIO_SET_BITWISE(GPIOL_ENABLE, 0x08)
#define DATA_IN       GPIO_CLEAR_BITWISE(GPIOL_OUTPUT_EN, 0x08)
#define DATA_OUT      GPIO_SET_BITWISE(GPIOL_OUTPUT_EN, 0x08)
#define DATA_LO       GPIO_CLEAR_BITWISE(GPIOL_OUTPUT_VAL, 0x08)
#define DATA_HI       GPIO_SET_BITWISE(GPIOL_OUTPUT_VAL, 0x08)

#define READWRITE_EN  GPIO_SET_BITWISE(GPIOL_ENABLE, 0x01)
#define READWRITE_OUT GPIO_SET_BITWISE(GPIOL_OUTPUT_EN, 0x01)
#define READWRITE_LO  GPIO_CLEAR_BITWISE(GPIOL_OUTPUT_VAL, 0x01)
#define READWRITE_HI  GPIO_SET_BITWISE(GPIOL_OUTPUT_VAL, 0x01)

#define DELAY1   udelay(1)
#define DELAY2   udelay(2)

void fmradio_3wire_init(void)
{
    READWRITE_EN;
    READWRITE_OUT;

    DATA_EN;
    DATA_IN;

    CLOCK_EN;
    CLOCK_HI;
    CLOCK_OUT;

    DELAY2;
}

int fmradio_3wire_write(const unsigned char* buf)
{
    int i, j;

    CLOCK_LO;
    READWRITE_LO;
    DELAY2;
    READWRITE_HI;
    DELAY1;
    DATA_OUT;

    /* 5 bytes to the tuner */
    for (j = 0; j < 5; j++)
    {
        for (i = 7; i >= 0; i--)
        {
            if ((buf[j] >> i) & 0x1) DATA_HI;
                else DATA_LO;
            DELAY1;
            CLOCK_HI;
            DELAY2;
            CLOCK_LO;
            DELAY1;
        }
    }
    READWRITE_LO;

    return j;
}

int fmradio_3wire_read(unsigned char* buf)
{
    int i, j;

    CLOCK_LO;
    READWRITE_HI;
    DELAY2;
    READWRITE_LO;
    DELAY2;
    DATA_IN;

    /* 5 bytes from the tuner */
    for (j = 0; j < 5; j++)
    {
        buf[j] = 0;
        for (i = 7; i >= 0; i--)
        {
            buf[j] |= ((DATA >> 3) << i);

            CLOCK_HI;
            DELAY2;
            CLOCK_LO;
            DELAY2;
        }
    }
    READWRITE_HI;

    return j;
}
