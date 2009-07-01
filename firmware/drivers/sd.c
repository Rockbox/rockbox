/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2009 by Rafaël Carré
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
#include "hotswap.h"

static const unsigned char sd_mantissa[] = {  /* *10 */
    0,  10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80 };
static const unsigned int sd_exponent[] = {  /* use varies */
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
#ifdef HAVE_MULTIVOLUME
    else if(csd_version == 1)
    {
        /* CSD version 2.0 */
        c_size = card_extract_bits(card->csd, 69, 22) + 1;
        card->numblocks = c_size << 10;
    }
#endif

    card->blocksize = 512;  /* Always use 512 byte blocks */

    card->speed = sd_mantissa[card_extract_bits(card->csd, 102, 4)] *
                  sd_exponent[card_extract_bits(card->csd,  98, 3) + 4];

    card->nsac = 100 * card_extract_bits(card->csd, 111, 8);

    card->taac = sd_mantissa[card_extract_bits(card->csd, 118, 4)] *
                 sd_exponent[card_extract_bits(card->csd, 114, 3)];

    card->r2w_factor = card_extract_bits(card->csd, 28, 3);
}
