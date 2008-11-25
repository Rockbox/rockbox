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

/* Note: since the base address is not specified, you need to define DMAC_BASE
 * before including this file */

/* ARM PrimeCell PL081 Single Master DMA controller */

#define DMAC_INT_STATUS           (*(volatile unsigned long*)(DMAC_BASE+0x000))
#define DMAC_INT_TC_STATUS        (*(volatile unsigned long*)(DMAC_BASE+0x004))
#define DMAC_INT_TC_CLEAR         (*(volatile unsigned long*)(DMAC_BASE+0x008))
#define DMAC_INT_ERROR_STATUS     (*(volatile unsigned long*)(DMAC_BASE+0x00C))
#define DMAC_INT_ERR_CLEAR        (*(volatile unsigned long*)(DMAC_BASE+0x010))
#define DMAC_RAW_INT_TC_STATUS    (*(volatile unsigned long*)(DMAC_BASE+0x014))
#define DMAC_RAW_INT_ERROR_STATUS (*(volatile unsigned long*)(DMAC_BASE+0x018))
#define DMAC_ENBLD_CHANS          (*(volatile unsigned long*)(DMAC_BASE+0x01C))
#define DMAC_SOFT_B_REQ           (*(volatile unsigned long*)(DMAC_BASE+0x020))
#define DMAC_SOFT_S_REQ           (*(volatile unsigned long*)(DMAC_BASE+0x024))
#define DMAC_SOFT_LB_REQ          (*(volatile unsigned long*)(DMAC_BASE+0x028))
#define DMAC_SOFT_LS_REQ          (*(volatile unsigned long*)(DMAC_BASE+0x02C))
#define DMAC_CONFIGURATION        (*(volatile unsigned long*)(DMAC_BASE+0x030))
#define DMAC_SYNC                 (*(volatile unsigned long*)(DMAC_BASE+0x034))

/* Channel registers (0 & 1) */
#define DMAC_CH_SRC_ADDR(c)       (*(volatile unsigned long*)(DMAC_BASE+0x100+(0x20*c)))
#define DMAC_CH_DST_ADDR(c)       (*(volatile unsigned long*)(DMAC_BASE+0x104+(0x20*c)))
#define DMAC_CH_LLI(c)            (*(volatile unsigned long*)(DMAC_BASE+0x108+(0x20*c)))
#define DMAC_CH_CONTROL(c)        (*(volatile unsigned long*)(DMAC_BASE+0x10C+(0x20*c)))
#define DMAC_CH_CONFIGURATION(c)  (*(volatile unsigned long*)(DMAC_BASE+0x110+(0x20*c)))

/* Test registers */
#define DMAC_ITCR                 (*(volatile unsigned long*)(DMAC_BASE+0x500))
#define DMAC_ITOP1                (*(volatile unsigned long*)(DMAC_BASE+0x504))
#define DMAC_ITOP2                (*(volatile unsigned long*)(DMAC_BASE+0x508))
#define DMAC_ITOP3                (*(volatile unsigned long*)(DMAC_BASE+0x50C))

/* Flow controllers */

/* Controller is DMAC */
#define DMAC_FLOWCTRL_DMAC_MEM_TO_MEM       0
#define DMAC_FLOWCTRL_DMAC_MEM_TO_PERI      1
#define DMAC_FLOWCTRL_DMAC_PERI_TO_MEM      2
#define DMAC_FLOWCTRL_DMAC_PERI_TO_PERI     3

/* Controller is peripheral */
#define DMAC_FLOWCTRL_SRC_PERI_PERI_TO_PERI 4
#define DMAC_FLOWCTRL_PERI_MEM_TO_PERI      5
#define DMAC_FLOWCTRL_PERI_PERI_TO_MEM      6
#define DMAC_FLOWCTRL_DST_PERI_PERI_TO_PERI 7

/* Transfer request sizes */
#define DMA_S1      0
#define DMA_S4      1
#define DMA_S8      2
#define DMA_S16     3
#define DMA_S32     4
#define DMA_S64     5
#define DMA_S128    6
#define DMA_S256    7
