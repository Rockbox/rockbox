/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Jens Arnold
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
#include <stdbool.h>
#include "config.h"
#ifdef HAVE_MMC
#include "ata_mmc.h"
#else
#include "hotswap.h"
#endif

/* helper function to extract n (<=32) bits from an arbitrary position.
   counting from MSB to LSB */
unsigned long card_extract_bits(
    const unsigned long *p, /* the start of the bitfield array */
    unsigned int start,     /* bit no. to start reading  */
    unsigned int size)      /* how many bits to read */
{
    unsigned int long_index = start / 32;
    unsigned int bit_index = start % 32;
    unsigned long result;
    
    result = p[long_index] << bit_index;

    if (bit_index + size > 32)    /* crossing longword boundary */
        result |= p[long_index+1] >> (32 - bit_index);
        
    result >>= 32 - size;

    return result;
}
