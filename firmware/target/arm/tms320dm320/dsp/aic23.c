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

#include "audio.h"
#include "registers.h"

/* based on http://archopen.svn.sourceforge.net/viewvc/archopen/ArchOpen/trunk/libdsp/aic23.c?revision=213&view=markup */
void audiohw_init(void)
{
    /* port config */
#if 0
    SPCR10 = 0;                  /* DLB      = 0 ** RJUST    = 0 ** CLKSTP   = 0 ** DXENA    = 0 ** ABIS     = 0 ** RINTM    = 0 ** RSYNCER  = 0 ** RFULL    = 0 ** RRDY     = 0 ** RRST     = 0 */
    SPCR20 = (1 << 9);           /* FREE     = 1 ** SOFT     = 0 ** FRST     = 0 ** GRST     = 0 ** XINTM    = 0 ** XSYNCER  = 0 ** XEMPTY   = 0 ** XRDY     = 0 ** XRST     = 0 */
    RCR10 = (1 << 8) | (2 << 5); /* RFRLEN1  = 1 ** RWDLEN1  = 2 */
    RCR20 = 0;                   /* RPHASE   = 0 ** RFRLEN2  = 0 ** RWDLEN2  = 0 ** RCOMPAND = 0 ** RFIG     = 0 ** RDATDLY  = 0 */
    XCR10 = (1 << 8) | (2 << 5); /* XFRLEN1  = 1 ** XWDLEN1  = 2 */
    XCR20 = 0;                   /* XPHASE   = 0 ** XFRLEN2  = 0 ** XWDLEN2  = 0 ** XCOMPAND = 0 ** XFIG     = 0 ** XDATDLY  = 0 */
    SRGR10 = 0;                  /* FWID     = 0 ** CLKGDV   = 0 */
    SRGR20 = 0;                  /* FREE     = 0 ** CLKSP    = 0 ** CLKSM    = 0 ** FSGM     = 0 ** FPER     = 0 */             
    PCR0 = (1 << 1) | 1;         /* IDLEEN   = 0 ** XIOEN    = 0 ** RIOEN    = 0 ** FSXM     = 0 ** FSRM     = 0 ** SCLKME   = 0 ** CLKSSTAT = 0 ** DXSTAT   = 0 ** DRSTAT   = 0 ** CLKXM    = 0 ** CLKRM    = 0 ** FSXP     = 0 ** FSRP     = 0 ** CLKXP    = 1 ** CLKRP    = 1 */
#else
    SPCR10 = 0;
    SPCR20 = 0x0200; /* SPCR : free running mode */
    
    RCR10 = 0x00A0;
    RCR20 = 0x00A1; /* RCR  : 32 bit receive data length */
    
    XCR10 = 0x00A0;
    XCR20 = 0x00A0; /* XCR  : 32 bit transmit data length */
    
    SRGR10 = 0;
    SRGR20 = 0x3000; /* SRGR 1 & 2 */
    
    PCR0 = 0x000E - 8; /* PCR  : FSX, FSR active low, external FS/CLK source */
#endif
}

void audiohw_postinit(void)
{
    /* Trigger first XEVT0 */
    SPCR20 |= 1;
}
