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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __HOTSWAP_H__
#define __HOTSWAP_H__

typedef struct
{  
    int initialized;
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
    unsigned long size;           /* size in bytes */
    unsigned long numblocks;      /* size in flash blocks */
    unsigned int blocksize;       /* block size in bytes */
    unsigned int block_exp;       /* block size exponent */
} tCardInfo;

#ifdef TARGET_TREE
#ifdef HAVE_HOTSWAP
#include "ata-sd-target.h"
#endif
#define card_detect            card_detect_target
#define card_get_info          card_get_info_target
#define card_enable_monitoring card_enable_monitoring_target
#else /* HAVE_MMC */
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
