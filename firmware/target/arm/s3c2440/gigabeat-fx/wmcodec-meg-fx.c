/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Gigabeat specific code for the Wolfson codec
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
#include "cpu.h"
#include "kernel.h"
#include "sound.h"
#include "i2c-meg-fx.h"
#include "wmcodec.h"

void audiohw_init(void)
{
    /* GPC5 controls headphone output */
    GPCCON &= ~(0x3 << 10);
    GPCCON |= (1 << 10);
    GPCDAT |= (1 << 5);

    audiohw_preinit();
}

void wmcodec_write(int reg, int data)
{
    unsigned char d[2];
    d[0] = (reg << 1) | ((data & 0x100) >> 8);
    d[1] = data;
    i2c_write(0x34, d, 2);
}
