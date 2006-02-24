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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __PP5002_H__
#define __PP5002_H__

/* All info gleaned and/or copied from the iPodLinux project. */

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

#define DEV_RS (*(volatile unsigned long *)( 0xcf005030))
#define DEV_EN (*(volatile unsigned long *)( 0xcf005000))


#define CPU_INT_STAT     (*(volatile unsigned long*)(0xcf001000))
#define CPU_INT_EN       (*(volatile unsigned long*)(0xcf001024))
#define CPU_INT_CLR      (*(volatile unsigned long*)(0xcf001028))

#define USB2D_IDENT         (*(volatile unsigned long*)(0xc5000000))
#define USB_STATUS          (*(volatile unsigned long*)(0xc50001a4))

#define IISCONFIG           (*(volatile unsigned long*)(0xc0002500))

#define IISFIFO_CFG         (*(volatile unsigned long*)(0xc000251c))
#define IISFIFO_WR          (*(volatile unsigned long*)(0xc0002540))
#define IISFIFO_RD          (*(volatile unsigned long*)(0xc0002580))
/* PP5002 registers */
#define PP5002_TIMER1       0xcf001100
#define PP5002_TIMER1_ACK   0xcf001104
#define PP5002_TIMER2       0xcf001108
#define PP5002_TIMER2_ACK   0xcf00110c

#define PP5002_TIMER_STATUS 0xcf001110

#define IDE_IRQ          1
#define SER0_IRQ         4
#define I2S_IRQ          5
#define SER1_IRQ         7
#define TIMER1_IRQ      11
#define GPIO_IRQ        14
#define DMA_OUT_IRQ     30
#define DMA_IN_IRQ      31



#define TIMER1_MASK  (1 << TIMER1_IRQ)
#define I2S_MASK     (1 << I2S_IRQ)
#define IDE_MASK     (1 << IDE_IRQ)
#define GPIO_MASK    (1 << GPIO_IRQ)
#define SER0_MASK    (1 << SER0_IRQ)
#define SER1_MASK    (1 << SER1_IRQ)

#define TIMER1_VAL   (*(volatile unsigned long *)(0xcf001104))
#define TIMER1_CFG   (*(volatile unsigned long *)(0xcf001100))
#define USEC_TIMER   (*(volatile unsigned long *)(0xcf001110))

#endif
