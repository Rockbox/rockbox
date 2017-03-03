/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: wmcodec-s5l8700.c 22025 2009-07-25 00:49:13Z dave $
 *
 * S5L8702-specific code for Cirrus codecs
 *
 * Copyright (c) 2010 Michael Sparmann
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
#include "audiohw.h"
#include "i2c-s5l8702.h"
#include "s5l8702.h"
#include "cscodec.h"

void audiohw_init(void)
{
#ifdef HAVE_CS42L55
    audiohw_preinit();
#endif
}

unsigned char cscodec_read(int reg)
{
    unsigned char data;
    i2c_read(0, 0x94, reg, 1, &data);
    return data;
}

void cscodec_write(int reg, unsigned char data)
{
    i2c_write(0, 0x94, reg, 1, &data);
}

void cscodec_power(bool state)
{
    (void)state; //TODO: Figure out which LDO this is
}

void cscodec_reset(bool state)
{
    if (state) PDAT(3) &= ~8;
    else PDAT(3) |= 8;
}

void cscodec_clock(bool state)
{
    if (state) CLKCON3 &= ~0xffff;
    else CLKCON3 |= 0x8000;
}
