/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __IRQ_X1000_H__
#define __IRQ_X1000_H__

/* INTC(0) interrupts */
#define IRQ0_DMIC   0
#define IRQ0_AIC    1
#define IRQ0_SFC    7
#define IRQ0_SSI    8
#define IRQ0_PDMA   10
#define IRQ0_PDMAD  11
#define IRQ0_GPIO3  14
#define IRQ0_GPIO2  15
#define IRQ0_GPIO1  16
#define IRQ0_GPIO0  17
#define IRQ0_OTG    21
#define IRQ0_AES    23
#define IRQ0_TCU2   25
#define IRQ0_TCU1   26
#define IRQ0_TCU0   27
#define IRQ0_CIM    30
#define IRQ0_LCD    31

/* INTC(1) interrupts */
#define IRQ1_RTC    0
#define IRQ1_MSC1   4
#define IRQ1_MSC0   5
#define IRQ1_SCC    6
#define IRQ1_PCM    8
#define IRQ1_HARB2  12
#define IRQ1_HARB0  14
#define IRQ1_CPM    15
#define IRQ1_UART2  17
#define IRQ1_UART1  18
#define IRQ1_UART0  19
#define IRQ1_DDR    20
#define IRQ1_EFUSE  22
#define IRQ1_MAC    23
#define IRQ1_I2C2   26
#define IRQ1_I2C1   27
#define IRQ1_I2C0   28
#define IRQ1_I2C(c) (28 - (c))
#define IRQ1_PDMAM  29
#define IRQ1_JPEG   30

/* Unified IRQ numbers */
#define IRQ_DMIC    IRQ0_DMIC
#define IRQ_AIC     IRQ0_AIC
#define IRQ_SFC     IRQ0_SFC
#define IRQ_SSI     IRQ0_SSI
#define IRQ_PDMA    IRQ0_PDMA
#define IRQ_PDMAD   IRQ0_PDMAD
#define IRQ_GPIO3   IRQ0_GPIO3
#define IRQ_GPIO2   IRQ0_GPIO2
#define IRQ_GPIO1   IRQ0_GPIO1
#define IRQ_GPIO0   IRQ0_GPIO0
#define IRQ_OTG     IRQ0_OTG
#define IRQ_AES     IRQ0_AES
#define IRQ_TCU2    IRQ0_TCU2
#define IRQ_TCU1    IRQ0_TCU1
#define IRQ_TCU0    IRQ0_TCU0
#define IRQ_CIM     IRQ0_CIM
#define IRQ_LCD     IRQ0_LCD
#define IRQ_RTC     (32+IRQ1_RTC)
#define IRQ_MSC1    (32+IRQ1_MSC1)
#define IRQ_MSC0    (32+IRQ1_MSC0)
#define IRQ_SCC     (32+IRQ1_SCC)
#define IRQ_PCM     (32+IRQ1_PCM)
#define IRQ_HARB2   (32+IRQ1_HARB2)
#define IRQ_HARB0   (32+IRQ1_HARB0)
#define IRQ_CPM     (32+IRQ1_CPM)
#define IRQ_UART2   (32+IRQ1_UART2)
#define IRQ_UART1   (32+IRQ1_UART1)
#define IRQ_UART0   (32+IRQ1_UART0)
#define IRQ_DDR     (32+IRQ1_DDR)
#define IRQ_EFUSE   (32+IRQ1_EFUSE)
#define IRQ_MAC     (32+IRQ1_MAC)
#define IRQ_I2C2    (32+IRQ1_I2C2)
#define IRQ_I2C1    (32+IRQ1_I2C1)
#define IRQ_I2C0    (32+IRQ1_I2C0)
#define IRQ_I2C(c)  (32+IRQ1_I2C(c))
#define IRQ_PDMAM   (32+IRQ1_PDMAM)
#define IRQ_JPEG    (32+IRQ1_JPEG)
#define IRQ_GPIO(port, pin) (64 + 32*(port) + (pin))

#define IRQ_IS_GROUP0(irq)      (((irq) & 0xffffff20) == 0x00)
#define IRQ_IS_GROUP1(irq)      (((irq) & 0xffffff20) == 0x20)
#define IRQ_IS_GPIO(irq)        ((irq) >= 64)

#define IRQ_TO_GROUP0(irq)      (irq)
#define IRQ_TO_GROUP1(irq)      ((irq) - 32)
#define IRQ_TO_GPIO_PORT(irq)   (((irq) - 64) >> 5)
#define IRQ_TO_GPIO_PIN(irq)    (((irq) - 64) & 0x1f)

#endif /* __IRQ_X1000_H__ */
