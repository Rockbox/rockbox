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
#include "config.h"
#include "logf.h"
#include "sdmmc.h"

static const unsigned char sd_mantissa[] = {  /* *10 */
    0,  10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80 };
static const unsigned int sd_exponent[] = {  /* use varies, div10 */
    1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000 };

void sd_parse_csd(tCardInfo *card)
{
    unsigned int c_size, c_mult;
    const int csd_version = card_extract_bits(card->csd, 127, 2);
    if(csd_version == 0)
    {
        /* CSD version 1.0 */
        int max_read_bl_len;

        c_size = card_extract_bits(card->csd, 73, 12) + 1;
        c_mult = 4 << card_extract_bits(card->csd, 49, 3);
        max_read_bl_len = 1 << card_extract_bits(card->csd, 83, 4);
        card->numblocks = c_size * c_mult * (max_read_bl_len/512);
    }
    else if(csd_version == 1)
    {
        /* CSD version 2.0 */
        c_size = card_extract_bits(card->csd, 69, 22) + 1;
        card->numblocks = c_size << 10;
    }
    else if(csd_version == 2)
    {
        /* CSD version 3.0 */
        c_size = card_extract_bits(card->csd, 75, 28) + 1;
        card->numblocks = c_size << 10;
    }
    card->sd2plus = csd_version >= 1;

    card->blocksize = 512;  /* Always use 512 byte blocks */

    card->speed = sd_mantissa[card_extract_bits(card->csd, 102, 4)] *
                  sd_exponent[card_extract_bits(card->csd,  98, 3) + 4];

    card->nsac = 100 * card_extract_bits(card->csd, 111, 8);

    card->taac = sd_mantissa[card_extract_bits(card->csd, 118, 4)] *
                 sd_exponent[card_extract_bits(card->csd, 114, 3)];

    card->r2w_factor = card_extract_bits(card->csd, 28, 3);

    logf("CSD%d.0 numblocks:%lld speed:%ld", csd_version+1, card->numblocks, card->speed);
    logf("nsac: %d taac: %ld r2w: %d", card->nsac, card->taac, card->r2w_factor);
}


/* helper function to extract n (<=32) bits from an arbitrary position.
   counting from MSB to LSB */
unsigned long card_extract_bits(
    const unsigned long *p, /* the start of the bitfield array */
    unsigned int start,     /* bit no. to start reading  */
    unsigned int size)      /* how many bits to read */
{
    unsigned int long_index, bit_index;
    unsigned long result;

    /* we assume words of CSD/CID are stored most significant word first */
    start = 127 - start;

    long_index = start / 32;
    bit_index = start % 32;

    result = p[long_index] << bit_index;

    if (bit_index + size > 32)    /* crossing longword boundary */
        result |= p[long_index+1] >> (32 - bit_index);

    result >>= 32 - size;

    return result;
}
