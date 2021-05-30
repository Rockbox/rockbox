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

#ifndef __DMA_X1000_H__
#define __DMA_X1000_H__

#include "x1000/dma.h"
#include "x1000/dma_chn.h"
#include <stdint.h>

/* Events passed to DMA callbacks */
#define DMA_EVENT_NONE      0   /* Not used by DMA code but can be used as
                                 * a sentinel value to indicate "no event" */
#define DMA_EVENT_INTERRUPT 1   /* Interrupt on a non-final descriptor */
#define DMA_EVENT_COMPLETE  2   /* Completed the final descriptor */
#define DMA_EVENT_ERROR     3   /* Some kind of error occurred */

/* All DMA channels which use interrupts must be statically defined here.
 * The channel numbering should be contiguous, and lower channel numbers
 * will have lower interrupt latency because they're serviced first.
 *
 * Channels >= DMA_NUM_USED_CHANNELS will NOT have interrupts serviced!
 * Due to the possibility of address error interrupts that can occur even
 * if no interrupts are requested on the channel, the unallocated channels
 * cannot be used safely.
 */
#define DMA_CHANNEL_AUDIO     0
#define DMA_CHANNEL_RECORD    1
#define DMA_CHANNEL_FBCOPY    2
#define DMA_NUM_USED_CHANNELS 3

struct dma_desc {
    uint32_t cm; /* meaning and layout same as DMA_CHN_CM */
    uint32_t sa; /* source address */
    uint32_t ta; /* target address */
    uint32_t tc; /* low 24 bits: transfer count
                  * upper 8 bits: offset to next descriptor
                  */
    uint32_t sd; /* same as DMA_CHN_SD */
    uint32_t rt; /* request type, same as DMA_CHN_RT */
    uint32_t pad0;
    uint32_t pad1;
} __attribute__((aligned(32)));

typedef struct dma_desc dma_desc;

typedef void(*dma_cb_func)(int event);

extern void dma_init(void);
extern void dma_set_callback(int chn, dma_cb_func cb);

#endif /* __DMA_X1000_H__ */
