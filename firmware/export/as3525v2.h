/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2009 Rafaël Carré
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

#ifndef __AS3525V2_H__
#define __AS3525V2_H__

#include "as3525.h"

/* insert differences here */

#define CACHEALIGN_BITS (5)

#ifndef IRAM_SIZE   /* protect in case the define name changes */
#   error IRAM_SIZE not defined !
#endif
#undef IRAM_SIZE
#define IRAM_SIZE 0x100000

#define CGU_SDSLOT         (*(volatile unsigned long *)(CGU_BASE + 0x3C))

#undef USB_NUM_ENDPOINTS
/* 7 available EPs (0b00000000010101010000000000101011), 6 used */
#define USB_NUM_ENDPOINTS   6

#define CCU_USB         (*(volatile unsigned long *)(CCU_BASE + 0x20))

#undef USB_DEVBSS_ATTR
#define USB_DEVBSS_ATTR __attribute__((aligned(32)))

/* Define this if the DWC implemented on this SoC does not support
   DMA or you want to disable it. */
// #define USB_DW_ARCH_SLAVE

#endif /* __AS3525V2_H__ */
