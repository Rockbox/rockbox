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
#ifndef __PP5020_H__
#define __PP5020_H__

/* All info gleaned and/or copied from the iPodLinux project. */

#define GPIOA_ENABLE     (*(volatile unsigned long *)(0x6000d000))
#define GPIOB_ENABLE     (*(volatile unsigned long *)(0x6000d004))
#define GPIOC_ENABLE     (*(volatile unsigned long *)(0x6000d008))
#define GPIOD_ENABLE     (*(volatile unsigned long *)(0x6000d00c))
#define GPIOA_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d010))
#define GPIOA_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d020))
#define GPIOA_INPUT_VAL  (*(volatile unsigned long *)(0x6000d030))
#define GPIOA_INT_STAT   (*(volatile unsigned long *)(0x6000d040))
#define GPIOA_INT_EN     (*(volatile unsigned long *)(0x6000d050))
#define GPIOA_INT_LEV    (*(volatile unsigned long *)(0x6000d060))
#define GPIOA_INT_CLR    (*(volatile unsigned long *)(0x6000d070))

#define PP5020_TIMER1       (*(volatile unsigned long *)(0x60005000))
#define PP5020_TIMER1_ACK   (*(volatile unsigned long *)(0x60005004))
#define PP5020_TIMER2       (*(volatile unsigned long *)(0x60005008))
#define PP5020_TIMER2_ACK   (*(volatile unsigned long *)(0x6000500c))
#define PP5020_TIMER_STATUS (*(volatile unsigned long *)(0x60005010))

#define PP5020_CPU_INT_STAT (*(volatile unsigned long*)(0x64004000))
#define PP5020_CPU_INT_EN   (*(volatile unsigned long*)(0x60004024))

#define PP5020_TIMER1_IRQ   0
#define PP5020_TIMER2_IRQ   1
#define PP5020_I2S_IRQ      10
#define PP5020_IDE_IRQ      23
#define PP5020_GPIO_IRQ     (32+0)
#define PP5020_SER0_IRQ     (32+4)
#define PP5020_SER1_IRQ     (32+5)
#define PP5020_I2C_IRQ      (32+8)

#define PP5020_TIMER1_MASK  (1 << PP5020_TIMER1_IRQ)
#define PP5020_I2S_MASK     (1 << PP5020_I2S_IRQ)
#define PP5020_IDE_MASK     (1 << PP5020_IDE_IRQ)
#define PP5020_GPIO_MASK    (1 << (PP5020_GPIO_IRQ-32))
#define PP5020_SER0_MASK    (1 << (PP5020_SER0_IRQ-32))
#define PP5020_SER1_MASK    (1 << (PP5020_SER1_IRQ-32))
#define PP5020_I2C_MASK     (1 << (PP5020_I2C_IRQ-32))

#endif
