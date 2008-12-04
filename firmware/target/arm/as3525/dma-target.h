/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
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

#include <stdbool.h>
#include <stdlib.h>

/* DMA request lines (16 max): not specified in AS3525 datasheet, but common to
 * all AS3525 based models (made by SanDisk) supported by rockbox. */

#define DMA_PERI_SD_SLOT    2
#define DMA_PERI_I2SOUT     3
#define DMA_PERI_I2SIN      4
#define DMA_PERI_SD         5   /* embedded storage */
#define DMA_PERI_DBOP       8

void dma_init(void);
void dma_enable_channel(int channel, void *src, void *dst, int peri,
                        int flow_controller, bool src_inc, bool dst_inc,
                        size_t size, int nwords, void (*callback)(void));
inline void dma_disable_channel(int channel);
inline void dma_wait_transfer(int channel);

void dma_retain(void);
void dma_release(void);
