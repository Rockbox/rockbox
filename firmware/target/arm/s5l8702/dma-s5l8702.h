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
#define S5L8702_DMAC0_BASE          DMA0_BASE
#define S5L8702_DMAC1_BASE          DMA1_BASE

/* S5L7802 DMAC0 peripherals */
#define S5L8702_DMAC0_PERI_IIS2_TX          0x0
#define S5L8702_DMAC0_PERI_IIS2_RX          0x1
#if CONFIG_CPU == S5L8702
#define S5L8702_DMAC0_PERI_SPDIF_TX         0x2
#endif
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
#if CONFIG_CPU == S5L8702
#define S5L8702_DMAC1_PERI_CEATA_RD         0x1
#endif
#define S5L8702_DMAC1_PERI_IIS1_TX          0x2
#define S5L8702_DMAC1_PERI_IIS1_RX          0x3
#define S5L8702_DMAC1_PERI_IIS2_TX          0x4
#define S5L8702_DMAC1_PERI_IIS2_RX          0x5
#if CONFIG_CPU == S5L8702
#define S5L8702_DMAC1_PERI_SPI1_TX          0x6
#define S5L8702_DMAC1_PERI_SPI1_RX          0x7
#endif
#define S5L8702_DMAC1_PERI_UART2_TX         0x8
#define S5L8702_DMAC1_PERI_UART2_RX         0x9
#define S5L8702_DMAC1_PERI_SPI0_TX          0xA
#define S5L8702_DMAC1_PERI_SPI0_RX          0xB
#define S5L8702_DMAC1_PERI_UART3_TX         0xC
#define S5L8702_DMAC1_PERI_UART3_RX         0xD
#if CONFIG_CPU == S5L8702
#define S5L8702_DMAC1_PERI_SPI2_TX          0xE
#define S5L8702_DMAC1_PERI_SPI2_RX          0xF
#endif

/* used when src and/or dst peripheral is memory */
#define S5L8702_DMAC0_PERI_MEM              DMAC_PERI_NONE
#define S5L8702_DMAC1_PERI_MEM              DMAC_PERI_NONE

/* s5l8702 peripheral DMA R/W addesses */
#define S5L8702_DADDR_PERI_LCD_WR           (LCD_BASE + 0x40)
#if CONFIG_CPU == S5L8702
#define S5L8702_DADDR_PERI_SPDIF_TX         (SPD_BASE + 0x10)  /* TBC */
#endif
#define S5L8702_DADDR_PERI_UART_TX(i)       (UARTC_BASE_ADDR + UARTC_PORT_OFFSET * (i) + 0x20)
#define S5L8702_DADDR_PERI_UART_RX(i)       (UARTC_BASE_ADDR + UARTC_PORT_OFFSET * (i) + 0x24)
#define S5L8702_DADDR_PERI_UART0_TX         S5L8702_DADDR_PERI_UART_TX(0)
#define S5L8702_DADDR_PERI_UART0_RX         S5L8702_DADDR_PERI_UART_RX(0)
#if CONFIG_CPU == S5L8702
#define S5L8702_DADDR_PERI_UART1_TX         S5L8702_DADDR_PERI_UART_TX(1)
#define S5L8702_DADDR_PERI_UART1_RX         S5L8702_DADDR_PERI_UART_RX(1)
#define S5L8702_DADDR_PERI_UART2_TX         S5L8702_DADDR_PERI_UART_TX(2)
#define S5L8702_DADDR_PERI_UART2_RX         S5L8702_DADDR_PERI_UART_RX(2)
#define S5L8702_DADDR_PERI_UART3_TX         S5L8702_DADDR_PERI_UART_TX(3)
#define S5L8702_DADDR_PERI_UART3_RX         S5L8702_DADDR_PERI_UART_RX(3)
#elif CONFIG_CPU == S5L8720
#define S5L8720_DADDR_PERI_UART_TX(i)       (UARTC_DMA_BASE_ADDR + UARTC_DMA_PORT_OFFSET * (i - 1) + 0x20)
#define S5L8720_DADDR_PERI_UART_RX(i)       (UARTC_DMA_BASE_ADDR + UARTC_DMA_PORT_OFFSET * (i - 1) + 0x24)
#define S5L8702_DADDR_PERI_UART1_TX         S5L8720_DADDR_PERI_UART_TX(1)
#define S5L8702_DADDR_PERI_UART1_RX         S5L8720_DADDR_PERI_UART_RX(1)
#define S5L8702_DADDR_PERI_UART2_TX         S5L8720_DADDR_PERI_UART_TX(2)
#define S5L8702_DADDR_PERI_UART2_RX         S5L8720_DADDR_PERI_UART_RX(2)
#define S5L8702_DADDR_PERI_UART3_TX         S5L8720_DADDR_PERI_UART_TX(3)
#define S5L8702_DADDR_PERI_UART3_RX         S5L8720_DADDR_PERI_UART_RX(3)
#endif
#define S5L8702_DADDR_PERI_IIS_OFFSET(i)    ((i) == 2 ? I2S_INTERFACE2_OFFSET : \
                                             (i) == 1 ? I2S_INTERFACE1_OFFSET : \
                                                        0)
#define S5L8702_DADDR_PERI_IIS_TX(i)        (I2S_BASE + S5L8702_DADDR_PERI_IIS_OFFSET(i) + 0x10)
#define S5L8702_DADDR_PERI_IIS_RX(i)        (I2S_BASE + S5L8702_DADDR_PERI_IIS_OFFSET(i) + 0x38)
#define S5L8702_DADDR_PERI_IIS0_TX          S5L8702_DADDR_PERI_IIS_TX(0)
#define S5L8702_DADDR_PERI_IIS0_RX          S5L8702_DADDR_PERI_IIS_RX(0)
#define S5L8702_DADDR_PERI_IIS1_TX          S5L8702_DADDR_PERI_IIS_TX(1)
#define S5L8702_DADDR_PERI_IIS1_RX          S5L8702_DADDR_PERI_IIS_RX(1)
#define S5L8702_DADDR_PERI_IIS2_TX          S5L8702_DADDR_PERI_IIS_TX(2)
#define S5L8702_DADDR_PERI_IIS2_RX          S5L8702_DADDR_PERI_IIS_RX(2)
#define S5L8702_DADDR_PERI_CEATA_WR         (ATA_UNKNOWN_BASE + 0x80)
#if CONFIG_CPU == S5L8702
#define S5L8702_DADDR_PERI_CEATA_RD         (ATA_UNKNOWN_BASE + 0x4000 + 0x80)
#endif
#define S5L8702_DADDR_PERI_SPI_TX(i)        (SPIBASE(i) + 0x10)
#define S5L8702_DADDR_PERI_SPI_RX(i)        (SPIBASE(i) + 0x20)
#define S5L8702_DADDR_PERI_SPI0_TX          S5L8702_DADDR_PERI_SPI_TX(0)
#define S5L8702_DADDR_PERI_SPI0_RX          S5L8702_DADDR_PERI_SPI_RX(0)
#define S5L8702_DADDR_PERI_SPI1_TX          S5L8702_DADDR_PERI_SPI_TX(1)
#define S5L8702_DADDR_PERI_SPI1_RX          S5L8702_DADDR_PERI_SPI_RX(1)
#define S5L8702_DADDR_PERI_SPI2_TX          S5L8702_DADDR_PERI_SPI_TX(2)
#define S5L8702_DADDR_PERI_SPI2_RX          S5L8702_DADDR_PERI_SPI_RX(2)

/* proto */
void dma_init(void);

#endif /* _DMA_S5l8702_H */
