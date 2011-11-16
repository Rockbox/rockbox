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

#include "config.h"
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "dma-target.h"
#include "dm320.h"
#include <stdbool.h>

void dma_init(void)
{
    /* TODO */
}

/*
   Requests channel for peripheral.
   Returns channel assigned for caller which must be released after
   transfer complete using dma_release_channel().
*/
int dma_request_channel(int peripheral, int mode)
{
    /* TODO: proper checking if channel is already taken
       currently only SDMMC and DSP uses DMA on this target */
    int channel = -1;

    if (peripheral == DMA_PERIPHERAL_MMCSD)
    {
        /* Set first DMA channel */
        IO_SDRAM_SDDMASEL = (IO_SDRAM_SDDMASEL & 0xFFE0) | peripheral | 
                            (mode << 3);
        channel = 1;
    }
    else if (peripheral == DMA_PERIPHERAL_DSP)
    {
        /* Set second DMA channel */
        IO_SDRAM_SDDMASEL = (IO_SDRAM_SDDMASEL & 0xFC1F) | 
                            (peripheral << 5) |
                            (mode << 8);
        channel = 2;
    }
    else if (peripheral == DMA_PERIPHERAL_SIF)
    {
        IO_SDRAM_SDDMASEL = (IO_SDRAM_SDDMASEL & 0x83FF) |
                            (peripheral << 10) |
                            (mode << 13);
        channel = 3;
    }

    return channel;
}

void dma_release_channel(int channel)
{
    (void)channel;
    /* TODO */
}


