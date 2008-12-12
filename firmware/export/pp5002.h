/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Thom Johansen 
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
#ifndef __PP5002_H__
#define __PP5002_H__

/* Much info gleaned and/or copied from the iPodLinux project. */
#define DRAM_START       0x28000000

/* LCD bridge */
#define LCD1_BASE        0xc0001000

#define LCD1_CONTROL     (*(volatile unsigned long *)(0xc0001000))
#define LCD1_CMD         (*(volatile unsigned long *)(0xc0001008))
#define LCD1_DATA        (*(volatile unsigned long *)(0xc0001010))

#define LCD1_BUSY_MASK   0x8000

/* I2S controller */

/* FIFO slot bits 7-0 are not implemented and so use of packed samples
 * appears to be impossible. */
#define IISCONFIG        (*(volatile unsigned long *)(0xc0002500))
#define IISFIFO_CFG      (*(volatile unsigned long *)(0xc000251c))
#define IISFIFO_WR       (*(volatile unsigned long *)(0xc0002540))
#define IISFIFO_RD       (*(volatile unsigned long *)(0xc0002580))

/**
 * IISCONFIG bits:
 * |   31   |   30   |   29   |   28   |   27   |   26   |   25   |   24   |
 * |        |        |        |        |        |        |        |        |
 * |   23   |   22   |   21   |   20   |   19   |   18   |   17   |   16   |
 * |        |        |        |        |        |        |        |        |
 * |   15   |   14   |   13   |   12   |   11   |   10   |    9   |    8   |
 * |        |        |        |        |        |        |        |        |
 * |    7   |    6   |    5   |    4   |    3   |    2   |    1   |    0   |
 * |   rw   |   rw   |   rw   |   MS   |   rw   |TXFENB# |   rw   |   ENB  |
 *
 * # No effect observed on iPod 3g
 */
#define IIS_ENABLE        (1 << 0)
#define IIS_TXFIFOEN      (1 << 2)
#define IIS_MASTER        (1 << 4)

/**
 * IISFIFO_CFG bits:
 * |   31   |   30   |   29   |   28   |   27   |   26   |   25   |   24   |
 * |        |            RXFull[3:0]$           |       TXFree[3:1]        >
 * |   23   |   22   |   21   |   20   |   19   |   18   |   17   |   16   |
 * >TXFre[0]|        |        |        |        |        | RXCLR  | TXCLR  |
 * |   15   |   14   |   13   |   12   |   11   |   10   |    9   |    8   |
 * |   rw   |   rw   |   rw   |   rw   |   rw   |   rw   | IRQTX  |   rw   |
 * |    7*  |    6*  |    5*  |    4*  |    3   |    2   |    1   |    0   |
 * |RXEMPTY | RXSAFE | RXDNGR | RXFULL | TXFULL | TXSAFE | TXDNGR |TXEMPTY |
 *
 * $Could be RXFree
 * *Meaning isn't certain yet.
 * More concerted recording work will reveal.
 */
#define IIS_IRQTX_REG     IISFIFO_CFG
#define IIS_IRQRX_REG     IISFIFO_CFG

#define IIS_RX_FULL_MASK  (0xf << 27)
#define IIS_RX_FULL_COUNT ((IISFIFO_CFG & IIS_RX_FULL_MASK) >> 27)
#define IIS_TX_FREE_MASK  (0xf << 23) /* 0xf = 16 or 15 free */
#define IIS_TX_FREE_COUNT ((IISFIFO_CFG & IIS_TX_FREE_MASK) >> 23)
#define IIS_TX_IS_EMPTY   ((IISFIFO_CFG & IIS_TXEMPTY) != 0)

#define IIS_RXCLR         (1 << 17) /* Resets *could* be reversed */
#define IIS_TXCLR         (1 << 16)
#define IIS_IRQTX         (1 <<  9)
#define IIS_TXFULL        (1 <<  3) /* All slots occupied */
#define IIS_TXSAFE        (1 <<  2) /* FIFO >= 3/4 full */
#define IIS_TXDANGER      (1 <<  1) /* FIFO <= 1/4 full */
#define IIS_TXEMPTY       (1 <<  0) /* No samples in FIFO */

#define IIS_RXEMPTY       (1 <<  4) /* FIFO is empty */

#define IDE_BASE         0xc0003000

#define IDE_CFG_STATUS   (*(volatile unsigned long *)(0xc0003024))

#define USB_BASE         0xc0005000

#define I2C_BASE         0xc0008000

/* Processor ID */
#define PROCESSOR_ID     (*(volatile unsigned long *)(0xc4000000))

#define PROC_ID_CPU      0x55
#define PROC_ID_COP      0xaa

#define GPIOA_ENABLE     (*(volatile unsigned char *)(0xcf000000))
#define GPIOB_ENABLE     (*(volatile unsigned char *)(0xcf000004))
#define GPIOC_ENABLE     (*(volatile unsigned char *)(0xcf000008))
#define GPIOD_ENABLE     (*(volatile unsigned char *)(0xcf00000c))
#define GPIOA_OUTPUT_EN  (*(volatile unsigned char *)(0xcf000010))
#define GPIOB_OUTPUT_EN  (*(volatile unsigned char *)(0xcf000014))
#define GPIOC_OUTPUT_EN  (*(volatile unsigned char *)(0xcf000018))
#define GPIOD_OUTPUT_EN  (*(volatile unsigned char *)(0xcf00001c))
#define GPIOA_OUTPUT_VAL (*(volatile unsigned char *)(0xcf000020))
#define GPIOB_OUTPUT_VAL (*(volatile unsigned char *)(0xcf000024))
#define GPIOC_OUTPUT_VAL (*(volatile unsigned char *)(0xcf000028))
#define GPIOD_OUTPUT_VAL (*(volatile unsigned char *)(0xcf00002c))
#define GPIOA_INPUT_VAL  (*(volatile unsigned char *)(0xcf000030))
#define GPIOB_INPUT_VAL  (*(volatile unsigned char *)(0xcf000034))
#define GPIOC_INPUT_VAL  (*(volatile unsigned char *)(0xcf000038))
#define GPIOD_INPUT_VAL  (*(volatile unsigned char *)(0xcf00003c))
#define GPIOA_INT_STAT   (*(volatile unsigned char *)(0xcf000040))
#define GPIOB_INT_STAT   (*(volatile unsigned char *)(0xcf000044))
#define GPIOC_INT_STAT   (*(volatile unsigned char *)(0xcf000048))
#define GPIOD_INT_STAT   (*(volatile unsigned char *)(0xcf00004c))
#define GPIOA_INT_EN     (*(volatile unsigned char *)(0xcf000050))
#define GPIOB_INT_EN     (*(volatile unsigned char *)(0xcf000054))
#define GPIOC_INT_EN     (*(volatile unsigned char *)(0xcf000058))
#define GPIOD_INT_EN     (*(volatile unsigned char *)(0xcf00005c))
#define GPIOA_INT_LEV    (*(volatile unsigned char *)(0xcf000060))
#define GPIOB_INT_LEV    (*(volatile unsigned char *)(0xcf000064))
#define GPIOC_INT_LEV    (*(volatile unsigned char *)(0xcf000068))
#define GPIOD_INT_LEV    (*(volatile unsigned char *)(0xcf00006c))
#define GPIOA_INT_CLR    (*(volatile unsigned char *)(0xcf000070))
#define GPIOB_INT_CLR    (*(volatile unsigned char *)(0xcf000074))
#define GPIOC_INT_CLR    (*(volatile unsigned char *)(0xcf000078))
#define GPIOD_INT_CLR    (*(volatile unsigned char *)(0xcf00007c))

#define CPU_INT_STAT     (*(volatile unsigned long *)(0xcf001000))
#define COP_INT_STAT     (*(volatile unsigned long *)(0xcf001004))
#define CPU_FIQ_STAT     (*(volatile unsigned long *)(0xcf001008))
#define COP_FIQ_STAT     (*(volatile unsigned long *)(0xcf00100c))

#define INT_STAT         (*(volatile unsigned long *)(0xcf001010))
#define INT_FORCED_STAT  (*(volatile unsigned long *)(0xcf001014))
#define INT_FORCED_SET   (*(volatile unsigned long *)(0xcf001018))
#define INT_FORCED_CLR   (*(volatile unsigned long *)(0xcf00101c))

#define CPU_INT_EN_STAT  (*(volatile unsigned long *)(0xcf001020))
#define CPU_INT_EN       (*(volatile unsigned long *)(0xcf001024))
#define CPU_INT_DIS      (*(volatile unsigned long *)(0xcf001028))
#define CPU_INT_PRIORITY (*(volatile unsigned long *)(0xcf00102c))

#define COP_INT_EN_STAT  (*(volatile unsigned long *)(0xcf001030))
#define COP_INT_EN       (*(volatile unsigned long *)(0xcf001034))
#define COP_INT_DIS      (*(volatile unsigned long *)(0xcf001038))
#define COP_INT_PRIORITY (*(volatile unsigned long *)(0xcf00103c))

#define IDE_IRQ          1
#define SER0_IRQ         4
#define I2S_IRQ          5
#define SER1_IRQ         7
#define TIMER1_IRQ      11
#define TIMER2_IRQ      12
#define GPIO_IRQ        14
#define DMA_OUT_IRQ     30
#define DMA_IN_IRQ      31

#define IDE_MASK         (1 << IDE_IRQ)
#define SER0_MASK        (1 << SER0_IRQ)
#define I2S_MASK         (1 << I2S_IRQ)
#define SER1_MASK        (1 << SER1_IRQ)
#define TIMER1_MASK      (1 << TIMER1_IRQ)
#define TIMER2_MASK      (1 << TIMER2_IRQ)
#define GPIO_MASK        (1 << GPIO_IRQ)
#define DMA_OUT_MASK     (1 << DMA_OUT_IRQ)
#define DMA_IN_MASK      (1 << DMA_IN_IRQ)

/* Yes, there is I2S_MASK but this cleans up the pcm code */
#define IIS_MASK         DMA_OUT_MASK

#define TIMER1_CFG       (*(volatile unsigned long *)(0xcf001100))
#define TIMER1_VAL       (*(volatile unsigned long *)(0xcf001104))
#define TIMER2_CFG       (*(volatile unsigned long *)(0xcf001108))
#define TIMER2_VAL       (*(volatile unsigned long *)(0xcf00110c))

#define USEC_TIMER       (*(volatile unsigned long *)(0xcf001110))

#define TIMING1_CTL      (*(volatile unsigned long *)(0xcf004000))
#define TIMING2_CTL      (*(volatile unsigned long *)(0xcf004008))

#define PP_VER1          (*(volatile unsigned long *)(0xcf004030))
#define PP_VER2          (*(volatile unsigned long *)(0xcf004034))
#define PP_VER3          (*(volatile unsigned long *)(0xcf004038))
#define PP_VER4          (*(volatile unsigned long *)(0xcf00403c))

/* Processors Control */
#define PROC_STAT        (*(volatile unsigned long *)(0xcf004050))
#define CPU_CTL          (*(volatile unsigned char *)(0xcf004054))
#define COP_CTL          (*(volatile unsigned char *)(0xcf004058))

#define CPU_SLEEPING     0x8000
#define COP_SLEEPING     0x4000
#define PROC_SLEEPING(core) (0x8000 >> (core))

#define PROC_CTL(core)   ((&CPU_CTL)[(core)*4])

#define PROC_SLEEP       0xca
#define PROC_WAKE        0xce

/* Cache Control */
#define CACHE_CTL        (*(volatile unsigned long *)(0xcf004024))
#define CACHE_RUN        0x1
#define CACHE_INIT       0x2

#define CACHE_MASK       (*(volatile unsigned long *)(0xf000f020))
#define CACHE_OPERATION  (*(volatile unsigned long *)(0xf000f024))
#define CACHE_FLUSH_BASE (*(volatile unsigned long *)(0xf000c000))
#define CACHE_INVALIDATE_BASE (*(volatile unsigned long *)(0xf0004000))
#define CACHE_SIZE       0x2000  /* PP5002 has 8KB cache */

#define CACHE_OP_UNKNOWN1 (1<<11) /* 0x800 */

#define DEV_EN           (*(volatile unsigned long *)(0xcf005000))
#define DEV_RS           (*(volatile unsigned long *)(0xcf005030))

#define DEV_I2C          (1<<8)
#define DEV_I2S          (1<<7)
#define DEV_USB          0x400000

#define CLOCK_ENABLE     (*(volatile unsigned long *)(0xcf005008))
#define CLOCK_SOURCE     (*(volatile unsigned long *)(0xcf00500c))
#define PLL_CONTROL      (*(volatile unsigned long *)(0xcf005010))
#define PLL_DIV          (*(volatile unsigned long *)(0xcf005018))
#define PLL_MULT         (*(volatile unsigned long *)(0xcf00501c))
#define PLL_UNLOCK       (*(volatile unsigned long *)(0xcf005038))

#define MMAP0_LOGICAL    (*(volatile unsigned long *)(0xf000f000))
#define MMAP0_PHYSICAL   (*(volatile unsigned long *)(0xf000f004))
#define MMAP1_LOGICAL    (*(volatile unsigned long *)(0xf000f008))
#define MMAP1_PHYSICAL   (*(volatile unsigned long *)(0xf000f00c))
#define MMAP2_LOGICAL    (*(volatile unsigned long *)(0xf000f010))
#define MMAP2_PHYSICAL   (*(volatile unsigned long *)(0xf000f014))
#define MMAP3_LOGICAL    (*(volatile unsigned long *)(0xf000f018))
#define MMAP3_PHYSICAL   (*(volatile unsigned long *)(0xf000f01c))

#endif
