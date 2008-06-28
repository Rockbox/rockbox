/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * TCC specific code for Wolfson audio codecs
 *
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in December 2005
 *
 * Original file: linux/arch/armnommu/mach-ipod/audio.c
 *
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
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
#include "i2c.h"
#include "wmcodec.h"

#if defined(COWON_D2)
/* The D2's audio codec uses an I2C address of 0x34 */
#define I2C_AUDIO_ADDRESS 0x34
#else
#error wmcodec not implemented for this target!
#endif


void audiohw_init(void)
{
}

void wmcodec_write(int reg, int data)
{
    unsigned char d[2];
    d[0] = (reg << 1) | ((data & 0x100) >> 8);
    d[1] = data;
    
    i2c_write(I2C_AUDIO_ADDRESS, d, 2);
}
