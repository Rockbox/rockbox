/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2016 by Amaury Pouly
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
#include "storage.h"

static const unsigned char sd_mantissa[] = {  /* *10 */
    0,  10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80 };
static const unsigned int sd_exponent[] = {  /* use varies */
    1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000 };

void mmc_parse_csd(tCardInfo *card, unsigned char *extended_csd)
{
    /* if we have an extended CSD, always use it because the CSD can only be
     * used for size up to 4GB in theory and spec limits it to 2GB */
    if(extended_csd)
    {
        card->numblocks = extended_csd[212] | extended_csd[213] << 8;
        card->numblocks |= extended_csd[214] << 16 | extended_csd[215] << 24;
    }
    else
    {
        unsigned c_size = card_extract_bits(card->csd, 73, 12) + 1;
        unsigned c_mult = 4 << card_extract_bits(card->csd, 49, 3);
        unsigned max_read_bl_len = 1 << card_extract_bits(card->csd, 83, 4);
        card->numblocks = c_size * c_mult * (max_read_bl_len/512);
    }

    card->blocksize = 512;  /* Always use 512 byte blocks */

    card->speed = sd_mantissa[card_extract_bits(card->csd, 102, 4)] *
                  sd_exponent[card_extract_bits(card->csd,  98, 3) + 4];

    card->nsac = 100 * card_extract_bits(card->csd, 111, 8);

    card->taac = sd_mantissa[card_extract_bits(card->csd, 118, 4)] *
                 sd_exponent[card_extract_bits(card->csd, 114, 3)];

    card->r2w_factor = card_extract_bits(card->csd, 28, 3);
}

void mmc_sleep(void)
{
}

void mmc_spin(void)
{
}

void mmc_spindown(int seconds)
{
    (void)seconds;
}

#ifdef STORAGE_GET_INFO
void mmc_get_info(IF_MD(int drive,) struct storage_info *info)
{
#ifndef HAVE_MULTIDRIVE
    const int drive=0;
#endif

    tCardInfo *card = mmc_card_info(drive);

    info->sector_size=card->blocksize;
    info->num_sectors=card->numblocks;
    info->vendor="Rockbox";
#if CONFIG_STORAGE == STORAGE_SD
    info->product = (drive==0) ? "Internal Storage" : "MMC Card Slot";
#else   /* Internal storage is not SD */
    info->product = "MMC Card Slot";
#endif
    info->revision="0.00";
}
#endif

 
