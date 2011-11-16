/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2011 by Tomasz Moń
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

#ifndef DMA_TARGET_H
#define DMA_TARGET_H

/* These defines match DMA Select bits */
#define DMA_PERIPHERAL_MTC   0
#define DMA_PERIPHERAL_SIF   1
#define DMA_PERIPHERAL_MS    2
#define DMA_PERIPHERAL_MMCSD 3
#define DMA_PERIPHERAL_DSP   4

/* These defines match DMA Burst bits */
/* 1 burst DMA - address must be 4 byte aligned */
#define DMA_MODE_1_BURST 0
/* 4 burst DMA - address must be 16 byte aligned */
#define DMA_MODE_4_BURST 1
/* 8 burst DMA - address must be 32 byte aligned */
#define DMA_MODE_8_BURST 2

void dma_init(void);
int dma_request_channel(int peripheral, int mode);
void dma_release_channel(int channel);

#endif
