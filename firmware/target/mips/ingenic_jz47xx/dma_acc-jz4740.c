/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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

#include "system.h"

void memset(void *target, unsigned char c, size_t len)
{
    int ch = DMA_CHANNEL;
    unsigned int d;
    unsigned char *dp;
    
    if(len < 32)
        _memset(target,c,len);
    else
    {
        if(((unsigned int)target < 0xa0000000) && len)
             dma_cache_wback_inv((unsigned long)target, len);
        
        dp = (unsigned char *)((unsigned int)(&d) | 0xa0000000);
        *(dp + 0) = c;
        *(dp + 1) = c;
        *(dp + 2) = c;
        *(dp + 3) = c;
        REG_DMAC_DSAR(ch) = PHYSADDR((unsigned long)dp);
        REG_DMAC_DTAR(ch) = PHYSADDR((unsigned long)target);
        REG_DMAC_DTCR(ch) = len / 32;
        REG_DMAC_DRSR(ch) = DMAC_DRSR_RS_AUTO;
        REG_DMAC_DCMD(ch) = DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 | DMAC_DCMD_DS_32BYTE;
        REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
        
        while (REG_DMAC_DTCR(ch));
        if(len % 32)
        {
            dp = (unsigned char *)((unsigned int)target + (len & (32 - 1)));
            for(d = 0;d < (len % 32); d++)
                *dp++ = c;
            
        }
    }    
}

void memset16(void *target, unsigned short c, size_t len)
{
    int ch = DMA_CHANNEL;
    unsigned short d;
    unsigned short *dp;
    
    if(len < 32)
        _memset16(target,c,len);
    else
    {
        if(((unsigned int)target < 0xa0000000) && len)
             dma_cache_wback_inv((unsigned long)target, len);
        
        d = c;
        REG_DMAC_DSAR(ch) = PHYSADDR((unsigned long)&d);
        REG_DMAC_DTAR(ch) = PHYSADDR((unsigned long)target);
        REG_DMAC_DTCR(ch) = len / 32;
        REG_DMAC_DRSR(ch) = DMAC_DRSR_RS_AUTO;
        REG_DMAC_DCMD(ch) = DMAC_DCMD_DAI | DMAC_DCMD_SWDH_16 | DMAC_DCMD_DWDH_16 | DMAC_DCMD_DS_32BYTE;
        REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
        
        while (REG_DMAC_DTCR(ch));
        if(len % 32)
        {
            dp = (unsigned short *)((unsigned int)target + (len & (32 - 1)));
            for(d = 0; d < (len % 32); d++)
                *dp++ = c;
        }
    }    
}

void memcpy(void *target, const void *source, size_t len)
{
    int ch = DMA_CHANNEL;
    unsigned char *dp;
    
    if(len < 4)
        _memcpy(target, source, len);
    
    if(((unsigned int)source < 0xa0000000) && len)
        dma_cache_wback_inv((unsigned long)source, len);
    
    if(((unsigned int)target < 0xa0000000) && len)
        dma_cache_wback_inv((unsigned long)target, len);
    
    REG_DMAC_DSAR(ch) = PHYSADDR((unsigned long)source);
    REG_DMAC_DTAR(ch) = PHYSADDR((unsigned long)target); 
    REG_DMAC_DTCR(ch) = len / 4;
    REG_DMAC_DRSR(ch) = DMAC_DRSR_RS_AUTO;
    REG_DMAC_DCMD(ch) = DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 | DMAC_DCMD_DS_32BIT;
    
    REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
    while (REG_DMAC_DTCR(ch));
    if(len % 4)
    {
        dp = (unsigned char*)((unsigned int)target + (len & (4 - 1)));
        for(i = 0; i < (len % 4); i++)
            *dp++ = *source;
    }
}
