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
#ifndef __ATA_MMC_H__
#define __ATA_MMC_H__

typedef struct
{  
    bool initialized;
    unsigned char bitrate_register;
    unsigned int read_timeout;    /* n * 8 clock cycles */
    unsigned int write_timeout;   /* n * 8 clock cycles */

    unsigned long ocr;            /* OCR register */
    unsigned long csd[4];         /* CSD register, 16 bytes */
    unsigned long cid[4];         /* CID register, 16 bytes */
    unsigned int speed;           /* bit/s */
    unsigned int nsac;            /* clock cycles */
    unsigned int tsac;            /* n * 0.1 ns */
    unsigned int r2w_factor;
} tCardInfo;

unsigned long mmc_extract_bits(const unsigned long *p, unsigned int start,
                               unsigned int size);
tCardInfo *mmc_card_info(int card_no);

#endif
