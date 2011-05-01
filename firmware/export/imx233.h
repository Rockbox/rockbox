/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#ifndef __IMX233_H__
#define __IMX233_H__

#define IRAM_ORIG           0
#define IRAM_SIZE           0x8000
#define DRAM_ORIG           0x40000000
#define DRAM_SIZE           0x20000000

#define TTB_BASE_ADDR (DRAM_ORIG + (MEMORYSIZE*0x100000) - TTB_SIZE)
#define TTB_SIZE      (0x4000)
#define TTB_BASE      ((unsigned long *)TTB_BASE_ADDR)
#define FRAME_SIZE    (240*320*2)

/* USBOTG */
#define USB_QHARRAY_ATTR    __attribute__((section(".qharray"),nocommon,aligned(2048)))
#define USB_NUM_ENDPOINTS   5
#define USB_DEVBSS_ATTR     NOCACHEBSS_ATTR
#define USB_BASE            0x80080000
/*
#define QHARRAY_SIZE        ((64*USB_NUM_ENDPOINTS*2 + 2047) & (0xffffffff - 2047))
#define QHARRAY_PHYS_ADDR   ((FRAME_PHYS_ADDR - QHARRAY_SIZE) & (0xffffffff - 2047))
*/

#define __REG_SET(reg)  (*((volatile uint32_t *)(&reg + 1)))
#define __REG_CLR(reg)  (*((volatile uint32_t *)(&reg + 2)))
#define __REG_TOG(reg)  (*((volatile uint32_t *)(&reg + 3)))

#define __BLOCK_SFTRST  (1 << 31)
#define __BLOCK_CLKGATE (1 << 30)


#endif /* __IMX233_H__ */
