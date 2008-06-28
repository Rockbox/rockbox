/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Antonius Hellmann
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
#ifndef ATA_SD_TARGET_H
#define ATA_SD_TARGET_H

#include "inttypes.h"
#include "hotswap.h"

typedef struct
{
    int initialized;

    unsigned int ocr;            /* OCR register */
    unsigned int csd[4];         /* CSD register */
    unsigned int cid[4];         /* CID register */
    unsigned int rca;

    uint64_t capacity;           /* size in bytes */
    unsigned long numblocks;     /* size in flash blocks */
    unsigned int block_size;     /* block size in bytes */
    unsigned int max_read_bl_len;/* max read data block length */
    unsigned int block_exp;      /* block size exponent */
    unsigned char current_bank;  /* The bank that we are working with */
} tSDCardInfo;

tCardInfo *card_get_info_target(int card_no);
bool       card_detect_target(void);

#ifdef HAVE_HOTSWAP
void       card_enable_monitoring_target(bool on);
void       microsd_int(void);
#endif

#endif
