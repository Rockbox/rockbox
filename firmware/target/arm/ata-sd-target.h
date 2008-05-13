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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef ATA_SD_TARGET_H
#define ATA_SD_TARGET_H

#include "inttypes.h"

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
void       card_enable_monitoring_target(bool on);

#endif
