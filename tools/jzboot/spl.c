/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "mkjzboot.h"

#define SPL_HEADER_SIZE 512
#define SPL_KEY_SIZE    1536
#define SPL_CODE_OFFSET (SPL_HEADER_SIZE + SPL_KEY_SIZE)

#define SPL_MAX_SIZE    (12 * 1024)
#define SPL_CODE_SIZE   (SPL_MAX_SIZE - SPL_CODE_OFFSET)

static const uint8_t flash_magic[8] =
    {0x06, 0x05, 0x04, 0x03, 0x02, 0x55, 0xaa, 0x55};

uint8_t* mkspl_flash(const uint8_t* spl_data, size_t spl_length,
                     int flashtype, uint8_t ppb, uint8_t bpp,
                     size_t* outsize)
{
    if(flashtype != FLASHTYPE_NAND &&
       flashtype != FLASHTYPE_NOR) {
        fprintf(stderr, "Invalid flash type %02x\n", flashtype);
        return NULL;
    }

    if(spl_length > SPL_CODE_SIZE) {
        fprintf(stderr, "SPL code too big (%zu bytes, max %d)\n",
                spl_length, SPL_CODE_SIZE);
        return NULL;
    }

    size_t outsz = SPL_HEADER_SIZE + SPL_KEY_SIZE + spl_length;
    uint8_t* outbuf = malloc(outsz);
    if(!outbuf) {
        fprintf(stderr, "Out of memory\n");
        return NULL;
    }

    /* Zero the unused parts of the buffer */
    memset(outbuf, 0, outsz);

    /* Fill header */
    memcpy(outbuf, flash_magic, 8);
    outbuf[8] = flashtype & 0xff;
    outbuf[9] = calc_crc7(spl_data, spl_length);
    outbuf[10] = ppb;
    outbuf[11] = bpp;
    to_le32(&outbuf[12], outsz);

    /* Copy code to correct location */
    memcpy(&outbuf[SPL_CODE_OFFSET], spl_data, spl_length);

    *outsize = outsz;
    return outbuf;
}
