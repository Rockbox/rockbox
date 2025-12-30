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

void dma_irq_handler(void) ATTR_IRQ_HANDLER;
void i2c_irq_handler(void) ATTR_IRQ_HANDLER;
void lcdc_irq_handler(void) ATTR_IRQ_HANDLER;
void otg_hs_irq_handler(void) ATTR_IRQ_HANDLER;
void sai_irq_handler(void) ATTR_IRQ_HANDLER;
void sdmmc1_irq_handler(void) ATTR_IRQ_HANDLER;
void sdmmc2_irq_handler(void) ATTR_IRQ_HANDLER;
void spi1_irq_handler(void) ATTR_IRQ_HANDLER;
void spi2_irq_handler(void) ATTR_IRQ_HANDLER;
void spi3_irq_handler(void) ATTR_IRQ_HANDLER;
void spi4_irq_handler(void) ATTR_IRQ_HANDLER;
void spi5_irq_handler(void) ATTR_IRQ_HANDLER;
void spi6_irq_handler(void) ATTR_IRQ_HANDLER;
