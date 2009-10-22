/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2009 by Bob Cousins
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

#ifndef _DMA_TARGET_H
#define _DMA_TARGET_H

#include <stdbool.h>
#include <stdlib.h>

/* DMA Channel assignments */
#ifdef GIGABEAT_F
#define DMA_CHAN_ATA        0
#define DMA_CHAN_AUDIO_OUT  2
#elif defined(MINI2440)
#define DMA_CHAN_SD         0
#define DMA_CHAN_AUDIO_OUT  2
#else
#error Unsupported target
#endif

struct dma_request 
{
    volatile void *source_addr;
    volatile void *dest_addr;
    unsigned long count;
    unsigned long source_control;
    unsigned long dest_control;
    unsigned long source_map;
    unsigned long control;
    void (*callback)(void);
};

void dma_init(void);
void dma_enable_channel(int channel, struct dma_request *request);

inline void dma_disable_channel(int channel);

void dma_retain(void);
void dma_release(void);

#endif
