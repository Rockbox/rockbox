/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: system-target.h 28791 2010-12-11 09:39:33Z Buschel $
 *
 * Copyright (C) 2010 by Michael Sparmann
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
#ifndef __PCM_TARGET_H__
#define __PCM_TARGET_H__


/* S5L8702 PCM driver tunables: */
#define PCM_LLIMAX (2047)     /* Maximum number of samples per LLI */
#define PCM_CHUNKSIZE (10747) /* Maximum number of samples to handle with one IRQ */
                              /* (bigger chunks will be segmented internally)     */
#define PCM_WATERMARK (512)   /* Number of remaining samples to schedule IRQ at */


#define PCM_LLICOUNT ((PCM_CHUNKSIZE - PCM_WATERMARK + PCM_LLIMAX - 1) / PCM_LLIMAX + 1)


extern struct dma_lli pcm_lli[PCM_LLICOUNT];
extern size_t pcm_remaining;
extern size_t pcm_chunksize;


#endif /* __PCM_TARGET_H__ */
