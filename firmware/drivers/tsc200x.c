/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Jonas Aaberg, Rob Purchase
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

#include "tsc200x.h"
#include "cpu.h"
#include "i2c.h"

#define TSC_SLAVE_ADDR 0x90
#define TSC_REG_READ_X 0x80
#define TSC_REG_READ_Y 0x90

#ifdef COWON_D2
#define TSCPRES_GPIO     GPIOC
#define TSCPRES_GPIO_DIR GPIOC_DIR
#define TSCPRES_BIT      (1<<26)
#endif

void tsc200x_init(void)
{
    /* Configure GPIOC 26 (TSC pressed) for input */
    TSCPRES_GPIO_DIR &= ~TSCPRES_BIT;
}

bool tsc200x_is_pressed(void)
{
    return !(TSCPRES_GPIO & TSCPRES_BIT);
}

bool tsc200x_read_coords(short* x, short* y)
{
    int rc = 0;
    unsigned char x_val[2], y_val[2];
    
    rc |= i2c_readmem(TSC_SLAVE_ADDR, TSC_REG_READ_Y, y_val, 2);
    rc |= i2c_readmem(TSC_SLAVE_ADDR, TSC_REG_READ_X, x_val, 2);

    if (rc >= 0)
    {
        /* Keep the 10 most significant bits */
        *x = ((((short)x_val[0]) << 8) | x_val[1]) >> 6;
        *y = ((((short)y_val[0]) << 8) | y_val[1]) >> 6;
        return true;
    }

    return false;
}
