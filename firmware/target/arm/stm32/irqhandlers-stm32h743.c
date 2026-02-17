/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 by Aidan MacDonald
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

/*
 * NOTE: This file must be included from system-arm-micro.c!
 */

void dmamux1_irq_handler(void) ATTR_IRQ_HANDLER;
void dmamux2_irq_handler(void) ATTR_IRQ_HANDLER;
void mdma_irq_handler(void) ATTR_IRQ_HANDLER;
void dma2d_irq_handler(void) ATTR_IRQ_HANDLER;
void dma1_ch0_irq_handler(void) ATTR_IRQ_HANDLER;
void dma1_ch1_irq_handler(void) ATTR_IRQ_HANDLER;
void dma1_ch2_irq_handler(void) ATTR_IRQ_HANDLER;
void dma1_ch3_irq_handler(void) ATTR_IRQ_HANDLER;
void dma1_ch4_irq_handler(void) ATTR_IRQ_HANDLER;
void dma1_ch5_irq_handler(void) ATTR_IRQ_HANDLER;
void dma1_ch6_irq_handler(void) ATTR_IRQ_HANDLER;
void dma1_ch7_irq_handler(void) ATTR_IRQ_HANDLER;
void dma2_ch0_irq_handler(void) ATTR_IRQ_HANDLER;
void dma2_ch1_irq_handler(void) ATTR_IRQ_HANDLER;
void dma2_ch2_irq_handler(void) ATTR_IRQ_HANDLER;
void dma2_ch3_irq_handler(void) ATTR_IRQ_HANDLER;
void dma2_ch4_irq_handler(void) ATTR_IRQ_HANDLER;
void dma2_ch5_irq_handler(void) ATTR_IRQ_HANDLER;
void dma2_ch6_irq_handler(void) ATTR_IRQ_HANDLER;
void dma2_ch7_irq_handler(void) ATTR_IRQ_HANDLER;
void bdma_ch0_irq_handler(void) ATTR_IRQ_HANDLER;
void bdma_ch1_irq_handler(void) ATTR_IRQ_HANDLER;
void bdma_ch2_irq_handler(void) ATTR_IRQ_HANDLER;
void bdma_ch3_irq_handler(void) ATTR_IRQ_HANDLER;
void bdma_ch4_irq_handler(void) ATTR_IRQ_HANDLER;
void bdma_ch5_irq_handler(void) ATTR_IRQ_HANDLER;
void bdma_ch6_irq_handler(void) ATTR_IRQ_HANDLER;
void bdma_ch7_irq_handler(void) ATTR_IRQ_HANDLER;
void i2c_irq_handler(void) ATTR_IRQ_HANDLER;
void lcdc_irq_handler(void) ATTR_IRQ_HANDLER;
void otg_hs_irq_handler(void) ATTR_IRQ_HANDLER;
void sai1_irq_handler(void) ATTR_IRQ_HANDLER;
void sai2_irq_handler(void) ATTR_IRQ_HANDLER;
void sai3_irq_handler(void) ATTR_IRQ_HANDLER;
void sai4_irq_handler(void) ATTR_IRQ_HANDLER;
void sdmmc1_irq_handler(void) ATTR_IRQ_HANDLER;
void sdmmc2_irq_handler(void) ATTR_IRQ_HANDLER;
void spi1_irq_handler(void) ATTR_IRQ_HANDLER;
void spi2_irq_handler(void) ATTR_IRQ_HANDLER;
void spi3_irq_handler(void) ATTR_IRQ_HANDLER;
void spi4_irq_handler(void) ATTR_IRQ_HANDLER;
void spi5_irq_handler(void) ATTR_IRQ_HANDLER;
void spi6_irq_handler(void) ATTR_IRQ_HANDLER;
void exti0_irq_handler(void) ATTR_IRQ_HANDLER;
void exti1_irq_handler(void) ATTR_IRQ_HANDLER;
void exti2_irq_handler(void) ATTR_IRQ_HANDLER;
void exti3_irq_handler(void) ATTR_IRQ_HANDLER;
void exti4_irq_handler(void) ATTR_IRQ_HANDLER;
void exti9_5_irq_handler(void) ATTR_IRQ_HANDLER;
void exti15_10_irq_handler(void) ATTR_IRQ_HANDLER;
