/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2009 Bertrik Sikken
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

#include <stdlib.h>

/* generic DMA definitions */
#define DMA_IO_TO_MEM       0
#define DMA_MEM_TO_IO       1

/* IISOUT DMA settings */
#define DMA_IISOUT_CHANNEL  0
#define DMA_IISOUT_SELECT   0
#define DMA_IISOUT_DSIZE    1   /* 1 = 16-bit/transfer */
#define DMA_IISOUT_BLEN     0   /* 0 = 1 transfer/burst */

void dma_init(void);

void dma_setup_channel(int channel, int sel, int dir, int dsize, int blen,
                       void *addr, size_t size, void (*callback)(void));

void dma_enable_channel(int channel);
void dma_disable_channel(int channel);

