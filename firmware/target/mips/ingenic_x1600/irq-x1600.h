/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

#ifndef __IRQ_X1600_H__
#define __IRQ_X1600_H__

/* Extend the INTC source numbers with per-pin GPIO IRQs and dispatcher
 * helpers. X1600 interrupt assignments differ from X1000. */
#include "x1600/intc.h"

/* Map the two supported I2C controllers to their INTC sources. */
#define IRQ_I2C(c)  ((c) == 0 ? IRQ_I2C0 : IRQ_I2C1)

/* GPIO IRQs start at 64 and reserve 32 entries per port. PA, PB and PC
 * have 32 pins; PD has six (PM tables 21-2 through 21-5). */
#define IRQ_GPIO(port, pin) (64 + 32*(port) + (pin))

#define IRQ_GPIO_A_BASE     IRQ_GPIO(0, 0)
#define IRQ_GPIO_D_LAST     IRQ_GPIO(3, 5)
#define IRQ_COUNT           (IRQ_GPIO(3, 6))

/* Group tests/conversions used by system_enable_irq()/system_disable_irq()
 * and by the vectoring code.  Group 0 is INTC sources 0..31, group 1 is
 * 32..63, and anything >= 64 is a GPIO pin interrupt. */
#define IRQ_IS_GROUP0(irq)      (((irq) & 0xffffffe0) == 0x00)
#define IRQ_IS_GROUP1(irq)      (((irq) & 0xffffffe0) == 0x20)
#define IRQ_IS_GPIO(irq)        ((irq) >= 64)

#define IRQ_TO_GROUP0(irq)      (irq)
#define IRQ_TO_GROUP1(irq)      ((irq) - 32)
#define IRQ_TO_GPIO_PORT(irq)   (((irq) - 64) >> 5)
#define IRQ_TO_GPIO_PIN(irq)    (((irq) - 64) & 0x1f)

/* INTC interrupt numbers and bit-mask helpers. */
#define IRQ_AUDIO       0   /* AIC */
#define IRQ_USB_OTG     1   /* live */
#define IRQ_PDMA        3   /* live */
#define IRQ_PDMAD       4   /* live: PDMA descriptor */
#define IRQ_PDMAM       5   /* live: PDMA MCU ("xburst-mcu") */
#define IRQ_PWM         6
#define IRQ_SFC         7   /* live */
#define IRQ_SSI0        9
#define IRQ_SADC        11  /* live */
#define IRQ_GPIO3       14
#define IRQ_GPIO2       15
#define IRQ_GPIO1       16
#define IRQ_GPIO0       17
#define IRQ_HASH        22
#define IRQ_AES         23
#define IRQ_TCU2        25
#define IRQ_TCU1        26  /* live */
#define IRQ_TCU0        27  /* live */
#define IRQ_MIPI_CSI    28
#define IRQ_CIM         30
#define IRQ_LCD         31  /* live -- this is the DPU on X1600 */
#define IRQ_RTC         32  /* live */
#define IRQ_SOFT        33  /* CPM software interrupt, see CPM_SFTINT */
#define IRQ_DTRNG       34
#define IRQ_MSC1        36  /* live -- microSD on the HiBy R1 */
#define IRQ_MSC0        37  /* live -- SDIO wifi on the HiBy R1 */
#define IRQ_CAN0        40
#define IRQ_CAN1        41
#define IRQ_CDBUS       42
#define IRQ_UART3       44
#define IRQ_UART2       45  /* live */
#define IRQ_UART1       46
#define IRQ_UART0       47  /* live */
#define IRQ_HARB2       49
#define IRQ_HARB0       50
#define IRQ_CPM         51
#define IRQ_DDR         52
#define IRQ_EFUSE       54
#define IRQ_GMAC        55
#define IRQ_I2C1        60  /* live -- touch panel on the HiBy R1 */
#define IRQ_I2C0        61  /* live -- AXP2101 PMU + CW2015 gauge */
#define IRQ_SSI_SLV     62
#define IRQ_INTC_COUNT  64

#define IRQ_TO_BIT(n)   (1u << ((n) & 31))

#endif /* __IRQ_X1600_H__ */
