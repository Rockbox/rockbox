/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Marcin Bukat
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
#include "i2c-rk27xx.h"
#include "wmcodec.h"

/* The WM8750's control interface in 2-wire mode, CSB low */
#define WM8750_I2C_ADDR     0x34

/* A register write is two bytes: the 7-bit register number over bit 8 of
 * the value, then the value's low byte. */
void wmcodec_write(int reg, int data)
{
    unsigned char lo = data & 0xff;

    i2c_write(WM8750_I2C_ADDR, (reg << 1) | ((data >> 8) & 1), 1, &lo);
}
