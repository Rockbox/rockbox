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

#define DMA_EVENT_INTERRUPT 1
#define DMA_EVENT_COMPLETE  2
#define DMA_EVENT_ERROR     3

struct dma_desc {
    unsigned cm; /* meaning and layout same as DMA_CHN_CM */
    unsigned sa; /* source address */
    unsigned ta; /* target address */
    unsigned tc; /* low 24 bits: transfer count
                  * upper 8 bits: offset to next descriptor
                  */
    unsigned sd; /* same as DMA_CHN_SD */
    unsigned rt; /* request type, same as DMA_CHN_RT */
    unsigned pad0;
    unsigned pad1;
} __attribute__((aligned(32)));

typedef struct dma_desc dma_desc;

typedef void(*dma_cb_func)(int event);

extern void dma_init(void);
extern void dma_set_callback(int chn, dma_cb_func cb);

#endif /* __DMA_X1000_H__ */
