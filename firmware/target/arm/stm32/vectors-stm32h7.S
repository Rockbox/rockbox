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
#include "config.h"

    .syntax unified
    .text

    .section .vectors.platform,"ax",%progbits

    .global __vectors_platform
__vectors_platform:
    .word   UIE                         /* [  0] */
    .word   UIE                         /* [  1] */
    .word   UIE                         /* [  2] */
    .word   UIE                         /* [  3] */
    .word   UIE                         /* [  4] */
    .word   UIE                         /* [  5] */
    .word   UIE                         /* [  6] */
    .word   UIE                         /* [  7] */
    .word   UIE                         /* [  8] */
    .word   UIE                         /* [  9] */
    .word   UIE                         /* [ 10] */
    .word   dma_irq_handler             /* [ 11] DMA1 Stream0 */
    .word   dma_irq_handler             /* [ 12] DMA1 Stream1 */
    .word   dma_irq_handler             /* [ 13] DMA1 Stream2 */
    .word   dma_irq_handler             /* [ 14] DMA1 Stream3 */
    .word   dma_irq_handler             /* [ 15] DMA1 Stream4 */
    .word   dma_irq_handler             /* [ 16] DMA1 Stream5 */
    .word   dma_irq_handler             /* [ 17] DMA1 Stream6 */
    .word   UIE                         /* [ 18] */
    .word   UIE                         /* [ 19] */
    .word   UIE                         /* [ 20] */
    .word   UIE                         /* [ 21] */
    .word   UIE                         /* [ 22] */
    .word   UIE                         /* [ 23] */
    .word   UIE                         /* [ 24] */
    .word   UIE                         /* [ 25] */
    .word   UIE                         /* [ 26] */
    .word   UIE                         /* [ 27] */
    .word   UIE                         /* [ 28] */
    .word   UIE                         /* [ 29] */
    .word   UIE                         /* [ 30] */
    .word   i2c_irq_handler             /* [ 31] I2C1 event */
    .word   i2c_irq_handler             /* [ 32] I2C1 error */
    .word   i2c_irq_handler             /* [ 33] I2C2 event */
    .word   i2c_irq_handler             /* [ 34] I2C2 error */
    .word   spi_irq_handler             /* [ 35] SPI1 */
    .word   spi_irq_handler             /* [ 36] SPI2 */
    .word   UIE                         /* [ 37] */
    .word   UIE                         /* [ 38] */
    .word   UIE                         /* [ 39] */
    .word   UIE                         /* [ 40] */
    .word   UIE                         /* [ 41] */
    .word   UIE                         /* [ 42] */
    .word   UIE                         /* [ 43] */
    .word   UIE                         /* [ 44] */
    .word   UIE                         /* [ 45] */
    .word   UIE                         /* [ 46] */
    .word   dma_irq_handler             /* [ 47] DMA1 Stream7 */
    .word   UIE                         /* [ 48] */
    .word   sdmmc_irq_handler           /* [ 49] SDMMC1 */
    .word   UIE                         /* [ 50] */
    .word   spi_irq_handler             /* [ 51] SPI3 */
    .word   UIE                         /* [ 52] */
    .word   UIE                         /* [ 53] */
    .word   UIE                         /* [ 54] */
    .word   UIE                         /* [ 55] */
    .word   dma_irq_handler             /* [ 56] DMA2 Stream0 */
    .word   dma_irq_handler             /* [ 57] DMA2 Stream1 */
    .word   dma_irq_handler             /* [ 58] DMA2 Stream2 */
    .word   dma_irq_handler             /* [ 59] DMA2 Stream3 */
    .word   dma_irq_handler             /* [ 60] DMA2 Stream4 */
    .word   UIE                         /* [ 61] */
    .word   UIE                         /* [ 62] */
    .word   UIE                         /* [ 63] */
    .word   UIE                         /* [ 64] */
    .word   UIE                         /* [ 65] */
    .word   UIE                         /* [ 66] */
    .word   UIE                         /* [ 67] */
    .word   dma_irq_handler             /* [ 68] DMA2 Stream5 */
    .word   dma_irq_handler             /* [ 69] DMA2 Stream6 */
    .word   dma_irq_handler             /* [ 70] DMA2 Stream7 */
    .word   UIE                         /* [ 71] */
    .word   i2c_irq_handler             /* [ 72] I2C3 event */
    .word   i2c_irq_handler             /* [ 73] I2C3 error */
    .word   UIE                         /* [ 74] */
    .word   UIE                         /* [ 75] */
    .word   UIE                         /* [ 76] */
    .word   otg_hs_irq_handler          /* [ 77] OTG HS */
    .word   UIE                         /* [ 78] */
    .word   UIE                         /* [ 79] */
    .word   UIE                         /* [ 80] */
    .word   UIE                         /* [ 81] */
    .word   UIE                         /* [ 82] */
    .word   UIE                         /* [ 83] */
    .word   spi_irq_handler             /* [ 84] SPI4 */
    .word   spi_irq_handler             /* [ 85] SPI5 */
    .word   spi_irq_handler             /* [ 86] SPI6 */
    .word   sai_irq_handler             /* [ 87] SAI1 */
    .word   lcdc_irq_handler            /* [ 88] LCDC */
    .word   lcdc_irq_handler            /* [ 89] LCDC error */
    .word   dma_irq_handler             /* [ 90] DMA2D */
    .word   sai_irq_handler             /* [ 91] SAI2 */
    .word   UIE                         /* [ 92] */
    .word   UIE                         /* [ 93] */
    .word   UIE                         /* [ 94] */
    .word   i2c_irq_handler             /* [ 95] I2C4 event */
    .word   i2c_irq_handler             /* [ 96] I2C4 error */
    .word   UIE                         /* [ 97] */
    .word   UIE                         /* [ 98] */
    .word   UIE                         /* [ 99] */
    .word   UIE                         /* [100] */
    .word   UIE                         /* [101] */
    .word   dma_irq_handler             /* [102] DMAMUX1 overrun */
    .word   UIE                         /* [103] */
    .word   UIE                         /* [104] */
    .word   UIE                         /* [105] */
    .word   UIE                         /* [106] */
    .word   UIE                         /* [107] */
    .word   UIE                         /* [108] */
    .word   UIE                         /* [109] */
    .word   UIE                         /* [110] */
    .word   UIE                         /* [111] */
    .word   UIE                         /* [112] */
    .word   UIE                         /* [113] */
    .word   sai_irq_handler             /* [114] SAI3 */
    .word   UIE                         /* [115] */
    .word   UIE                         /* [116] */
    .word   UIE                         /* [117] */
    .word   UIE                         /* [118] */
    .word   UIE                         /* [119] */
    .word   UIE                         /* [120] */
    .word   UIE                         /* [121] */
    .word   dma_irq_handler             /* [122] MDMA */
    .word   UIE                         /* [123] */
    .word   sdmmc_irq_handler           /* [124] SDMMC2 */
    .word   UIE                         /* [125] */
    .word   UIE                         /* [126] */
    .word   UIE                         /* [127] */
    .word   dma_irq_handler             /* [128] DMAMUX2 overrun */
    .word   dma_irq_handler             /* [129] BDMA channel 0 */
    .word   dma_irq_handler             /* [130] BDMA channel 1 */
    .word   dma_irq_handler             /* [131] BDMA channel 2 */
    .word   dma_irq_handler             /* [132] BDMA channel 3 */
    .word   dma_irq_handler             /* [133] BDMA channel 4 */
    .word   dma_irq_handler             /* [134] BDMA channel 5 */
    .word   dma_irq_handler             /* [135] BDMA channel 6 */
    .word   dma_irq_handler             /* [136] BDMA channel 7 */
    .word   UIE                         /* [137] */
    .word   UIE                         /* [138] */
    .word   UIE                         /* [139] */
    .word   UIE                         /* [140] */
    .word   UIE                         /* [141] */
    .word   UIE                         /* [142] */
    .word   UIE                         /* [143] */
    .word   UIE                         /* [144] */
    .word   UIE                         /* [145] */
    .word   sai_irq_handler             /* [146] SAI4 */
    .word   UIE                         /* [147] */
    .word   UIE                         /* [148] */
    .word   UIE                         /* [149] */
