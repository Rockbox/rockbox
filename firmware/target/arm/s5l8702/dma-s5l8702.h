/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Cástor Muñoz
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

#ifndef _DMA_S5l8702_H
#define _DMA_S5l8702_H

#include "pl080.h"

/*
 * s5l8702 PL080 controllers configuration
 */

extern struct dmac s5l8702_dmac0;
extern struct dmac s5l8702_dmac1;

#define S5L8702_DMAC_COUNT          2   /* N PL080 controllers */
#define S5L8702_DMAC0_BASE          0x38200000
#define S5L8702_DMAC1_BASE          0x39900000

/* S5L7802 DMAC0 peripherals */
#define S5L8702_DMAC0_PERI_IIS2_TX          0x0
#define S5L8702_DMAC0_PERI_IIS2_RX          0x1
#define S5L8702_DMAC0_PERI_UNKNOWN          0x2
#define S5L8702_DMAC0_PERI_LCD_WR           0x3
#define S5L8702_DMAC0_PERI_SPI0_TX          0x4
#define S5L8702_DMAC0_PERI_SPI0_RX          0x5
#define S5L8702_DMAC0_PERI_UART0_TX         0x6
#define S5L8702_DMAC0_PERI_UART0_RX         0x7
#define S5L8702_DMAC0_PERI_UART1_TX         0x8
#define S5L8702_DMAC0_PERI_UART1_RX         0x9
#define S5L8702_DMAC0_PERI_IIS0_TX          0xA
#define S5L8702_DMAC0_PERI_IIS0_RX          0xB
#define S5L8702_DMAC0_PERI_SPI2_TX          0xC
#define S5L8702_DMAC0_PERI_SPI2_RX          0xD
#define S5L8702_DMAC0_PERI_SPI1_TX          0xE
#define S5L8702_DMAC0_PERI_SPI1_RX          0xF

/* S5L7802 DMAC1 peripherals */
#define S5L8702_DMAC1_PERI_CEATA_WR         0x0
#define S5L8702_DMAC1_PERI_CEATA_RD         0x1
#define S5L8702_DMAC1_PERI_IIS1_TX          0x2
#define S5L8702_DMAC1_PERI_IIS1_RX          0x3
#define S5L8702_DMAC1_PERI_IIS2_TX          0x4
#define S5L8702_DMAC1_PERI_IIS2_RX          0x5
#define S5L8702_DMAC1_PERI_SPI1_TX          0x6
#define S5L8702_DMAC1_PERI_SPI1_RX          0x7
#define S5L8702_DMAC1_PERI_UART2_TX         0x8
#define S5L8702_DMAC1_PERI_UART2_RX         0x9
#define S5L8702_DMAC1_PERI_SPI0_TX          0xA
#define S5L8702_DMAC1_PERI_SPI0_RX          0xB
#define S5L8702_DMAC1_PERI_UART3_TX         0xC
#define S5L8702_DMAC1_PERI_UART3_RX         0xD
#define S5L8702_DMAC1_PERI_SPI2_TX          0xE
#define S5L8702_DMAC1_PERI_SPI2_RX          0xF

/* used when src and/or dst peripheral is memory */
#define S5L8702_DMAC0_PERI_MEM              DMAC_PERI_NONE
#define S5L8702_DMAC1_PERI_MEM              DMAC_PERI_NONE

/* s5l8702 peripheral DMA R/W addesses */
#define S5L8702_DADDR_PERI_LCD_WR           0x38300040
#define S5L8702_DADDR_PERI_UNKNOWN          0x3CB00010  /* SPDIF ??? */
#define S5L8702_DADDR_PERI_UART0_TX         0x3CC00020
#define S5L8702_DADDR_PERI_UART0_RX         0x3CC00024
#define S5L8702_DADDR_PERI_UART1_TX         0x3CC04020
#define S5L8702_DADDR_PERI_UART1_RX         0x3CC04024
#define S5L8702_DADDR_PERI_UART2_TX         0x3CC08020
#define S5L8702_DADDR_PERI_UART2_RX         0x3CC08024
#define S5L8702_DADDR_PERI_UART3_TX         0x3CC0C020
#define S5L8702_DADDR_PERI_UART3_RX         0x3CC0C024
#define S5L8702_DADDR_PERI_IIS0_TX          0x3CA00010
#define S5L8702_DADDR_PERI_IIS0_RX          0x3CA00038
#define S5L8702_DADDR_PERI_IIS1_TX          0x3CD00010
#define S5L8702_DADDR_PERI_IIS1_RX          0x3CD00038
#define S5L8702_DADDR_PERI_IIS2_TX          0x3D400010
#define S5L8702_DADDR_PERI_IIS2_RX          0x3D400038
#define S5L8702_DADDR_PERI_CEATA_WR         0x38A00080
#define S5L8702_DADDR_PERI_CEATA_RD         0x38A04080
#define S5L8702_DADDR_PERI_SPI0_TX          0x3C300010
#define S5L8702_DADDR_PERI_SPI0_RX          0x3C300020
#define S5L8702_DADDR_PERI_SPI1_TX          0x3CE00010
#define S5L8702_DADDR_PERI_SPI1_RX          0x3CE00020
#define S5L8702_DADDR_PERI_SPI2_TX          0x3D200010
#define S5L8702_DADDR_PERI_SPI2_RX          0x3D200020

/* proto */
void dma_init(void);

#endif /* _DMA_S5l8702_H */
