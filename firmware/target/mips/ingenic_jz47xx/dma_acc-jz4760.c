/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
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

#include "dma-target.h"

#define MDMA_CHANNEL 0

void memset_dma(void *target, int c, size_t len, unsigned int bits)
{
    unsigned int d;
    unsigned char *dp;

    if(((unsigned int)target < 0xa0000000) && len)
         dma_cache_wback_inv((unsigned long)target, len);

    dp = (unsigned char *)((unsigned int)(&d) | 0xa0000000);
    *(dp + 0) = c;
    *(dp + 1) = c;
    *(dp + 2) = c;
    *(dp + 3) = c;

    REG_MDMAC_DCCSR(MDMA_CHANNEL) = 0;
    REG_MDMAC_DSAR(MDMA_CHANNEL) = PHYSADDR((unsigned long)dp);
    REG_MDMAC_DTAR(MDMA_CHANNEL) = PHYSADDR((unsigned long)target);
    REG_MDMAC_DRSR(MDMA_CHANNEL) = DMAC_DRSR_RS_AUTO;
    switch (bits)
    {
        case 8:
            REG_MDMAC_DTCR(MDMA_CHANNEL) = len;
            REG_MDMAC_DCMD(MDMA_CHANNEL) = DMAC_DCMD_DAI | DMAC_DCMD_SWDH_8 | DMAC_DCMD_DWDH_8 | DMAC_DCMD_DS_8BIT;
            break;
        case 16:
            REG_MDMAC_DTCR(MDMA_CHANNEL) = len / 2;
            REG_MDMAC_DCMD(MDMA_CHANNEL) = DMAC_DCMD_DAI | DMAC_DCMD_SWDH_16 | DMAC_DCMD_DWDH_16 | DMAC_DCMD_DS_16BIT;
            break;
        case 32:
            REG_MDMAC_DTCR(MDMA_CHANNEL) = len / 4;
            REG_MDMAC_DCMD(MDMA_CHANNEL) = DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 | DMAC_DCMD_DS_32BIT;
            break;
        default:
            return;
    }
    REG_MDMAC_DCCSR(MDMA_CHANNEL) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;

    while (REG_MDMAC_DTCR(MDMA_CHANNEL));

    REG_MDMAC_DCCSR(MDMA_CHANNEL) = 0;
}

void memcpy_dma(void *target, const void *source, size_t len, unsigned int bits)
{
    if(((unsigned int)source < 0xa0000000) && len)
        dma_cache_wback_inv((unsigned long)source, len);
    
    if(((unsigned int)target < 0xa0000000) && len)
        dma_cache_wback_inv((unsigned long)target, len);
    
    REG_MDMAC_DCCSR(MDMA_CHANNEL) = 0;
    REG_MDMAC_DSAR(MDMA_CHANNEL) = PHYSADDR((unsigned long)source);
    REG_MDMAC_DTAR(MDMA_CHANNEL) = PHYSADDR((unsigned long)target); 
    REG_MDMAC_DRSR(MDMA_CHANNEL) = DMAC_DRSR_RS_AUTO;
    switch (bits)
    {
        case 8:
            REG_MDMAC_DTCR(MDMA_CHANNEL) = len;
            REG_MDMAC_DCMD(MDMA_CHANNEL) = DMAC_DCMD_SAI | DMAC_DCMD_DAI | DMAC_DCMD_SWDH_8 | DMAC_DCMD_DWDH_8 | DMAC_DCMD_DS_8BIT;
            break;
        case 16:
            REG_MDMAC_DTCR(MDMA_CHANNEL) = len / 2;
            REG_MDMAC_DCMD(MDMA_CHANNEL) = DMAC_DCMD_SAI | DMAC_DCMD_DAI | DMAC_DCMD_SWDH_16 | DMAC_DCMD_DWDH_16 | DMAC_DCMD_DS_16BIT;
            break;
        case 32:
            REG_MDMAC_DTCR(MDMA_CHANNEL) = len / 4;
            REG_MDMAC_DCMD(MDMA_CHANNEL) = DMAC_DCMD_SAI | DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 | DMAC_DCMD_DS_32BIT;
            break;
        default:
            return;
    }
    REG_MDMAC_DCCSR(MDMA_CHANNEL) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;

    while (REG_MDMAC_DTCR(MDMA_CHANNEL));

    REG_MDMAC_DCCSR(MDMA_CHANNEL) = 0;
}
