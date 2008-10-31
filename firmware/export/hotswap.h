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
#ifndef __HOTSWAP_H__
#define __HOTSWAP_H__

#include <stdbool.h>

typedef struct
{  
    bool initialized;
    unsigned char bitrate_register;
    unsigned long read_timeout;   /* n * 8 clock cycles */
    unsigned long write_timeout;  /* n * 8 clock cycles */

    unsigned long ocr;            /* OCR register */
    unsigned long csd[4];         /* CSD register, 16 bytes */
    unsigned long cid[4];         /* CID register, 16 bytes */
    unsigned long speed;          /* bit/s */
    unsigned int nsac;            /* clock cycles */
    unsigned long tsac;           /* n * 0.1 ns */
    unsigned int r2w_factor;
    unsigned long numblocks;      /* size in flash blocks */
    unsigned int blocksize;       /* block size in bytes */
} tCardInfo;

#if (CONFIG_STORAGE & STORAGE_SD)
#include "ata-sd-target.h"
#define card_detect            card_detect_target
#define card_get_info          card_get_info_target
#ifdef HAVE_HOTSWAP
#define card_enable_monitoring card_enable_monitoring_target
#endif
#else /* STORAGE_MMC */
#include "ata_mmc.h"
#define card_detect            mmc_detect
#define card_get_info          mmc_card_info
#define card_touched           mmc_touched
#define card_enable_monitoring mmc_enable_monitoring
#endif

/* helper function to extract n (<=32) bits from an arbitrary position.
   counting from MSB to LSB */
unsigned long card_extract_bits(
        const unsigned long *p, /* the start of the bitfield array */
        unsigned int start,     /* bit no. to start reading  */
        unsigned int size);     /* how many bits to read */
#endif
