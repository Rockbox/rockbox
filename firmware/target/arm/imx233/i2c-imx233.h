/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by amaury Pouly
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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
#ifndef __I2C_IMX233_H__
#define __I2C_IMX233_H__

#include "cpu.h"
#include "system.h"
#include "system-target.h"
#include "i2c.h"

#define HW_I2C_BASE         0x80058000 

#define HW_I2C_CTRL0        (*(volatile uint32_t *)(HW_I2C_BASE + 0x0))
#define HW_I2C_CTRL0__XFER_COUNT_BM     0xffff
#define HW_I2C_CTRL0__TRANSMIT          (1 << 16)
#define HW_I2C_CTRL0__MASTER_MODE       (1 << 17)
#define HW_I2C_CTRL0__SLAVE_ADDRESS_ENABLE  (1 << 18)
#define HW_I2C_CTRL0__PRE_SEND_START    (1 << 19)
#define HW_I2C_CTRL0__POST_SEND_STOP    (1 << 20)
#define HW_I2C_CTRL0__RETAIN_CLOCK      (1 << 21)
#define HW_I2C_CTRL0__CLOCK_HELD        (1 << 22)
#define HW_I2C_CTRL0__PIO_MODE          (1 << 24)
#define HW_I2C_CTRL0__SEND_NAK_ON_LAST  (1 << 25)
#define HW_I2C_CTRL0__ACKNOWLEDGE       (1 << 26)
#define HW_I2C_CTRL0__RUN               (1 << 29)

#define HW_I2C_TIMING0      (*(volatile uint32_t *)(HW_I2C_BASE + 0x10))
#define HW_I2C_TIMING0__RECV_COUNT_BM   0x3ff
#define HW_I2C_TIMING0__HIGH_COUNT_BM   (0x3ff << 16)
#define HW_I2C_TIMING0__HIGH_COUNT_BP   16

#define HW_I2C_TIMING1      (*(volatile uint32_t *)(HW_I2C_BASE + 0x20))
#define HW_I2C_TIMING1__XMIT_COUNT_BM   0x3ff
#define HW_I2C_TIMING1__LOW_COUNT_BM    (0x3ff << 16)
#define HW_I2C_TIMING1__LOW_COUNT_BP    16

#define HW_I2C_TIMING2      (*(volatile uint32_t *)(HW_I2C_BASE + 0x30))
#define HW_I2C_TIMING2__LEADIN_COUNT_BM 0x3ff
#define HW_I2C_TIMING2__BUS_FREE_BM     (0x3ff << 16)
#define HW_I2C_TIMING2__BUS_FREE_BP     16

#define HW_I2C_CTRL1        (*(volatile uint32_t *)(HW_I2C_BASE + 0x40))
#define HW_I2C_CTRL1__SLAVE_IRQ         (1 << 0)
#define HW_I2C_CTRL1__SLAVE_STOP_IRQ    (1 << 1)
#define HW_I2C_CTRL1__MASTER_LOSS_IRQ   (1 << 2)
#define HW_I2C_CTRL1__EARLY_TERM_IRQ    (1 << 3)
#define HW_I2C_CTRL1__OVERSIZE_XFER_TERM_IRQ    (1 << 4)
#define HW_I2C_CTRL1__NO_SLAVE_ACK_IRQ  (1 << 5)
#define HW_I2C_CTRL1__DATA_ENGINE_COMPLT_IRQ    (1 << 6)
#define HW_I2C_CTRL1__BUS_FREE_IRQ      (1 << 7)
#define HW_I2C_CTRL1__SLAVE_IRQ_EN      (1 << 8)
#define HW_I2C_CTRL1__SLAVE_STOP_IRQ_EN (1 << 9)
#define HW_I2C_CTRL1__MASTER_LOSS_IRQ_EN    (1 << 10)
#define HW_I2C_CTRL1__EARLY_TERM_IRQ_EN (1 << 11)
#define HW_I2C_CTRL1__OVERSIZE_XFER_TERM_IRQ_EN (1 << 12)
#define HW_I2C_CTRL1__NO_SLAVE_ACK_IRQ_EN   (1 << 13)
#define HW_I2C_CTRL1__DATA_ENGINE_COMPLT_IRQ_EN (1 << 14)
#define HW_I2C_CTRL1__BUS_FREE_IRQ_EN   (1 << 15)
#define HW_I2C_CTRL1__BCAST_SLAVE_EN    (1 << 24)
#define HW_I2C_CTRL1__FORCE_CLK_IDLE    (1 << 25)
#define HW_I2C_CTRL1__FORCE_DATA_IDLE   (1 << 26)
#define HW_I2C_CTRL1__ACK_MODE          (1 << 27)
#define HW_I2C_CTRL1__CLR_GOT_A_NAK     (1 << 28)
#define HW_I2C_CTRL1__ALL_IRQ           0xff
#define HW_I2C_CTRL1__ALL_IRQ_EN        0xff00

#define HW_I2C_STAT         (*(volatile uint32_t *)(HW_I2C_BASE + 0x50))
#define HW_I2C_STAT__SLAVE_IRQ_SUMMARY          (1 << 0)
#define HW_I2C_STAT__SLAVE_STOP_IRQ_SUMMARY     (1 << 1)
#define HW_I2C_STAT__MASTER_LOSS_IRQ_SUMMARY    (1 << 2)
#define HW_I2C_STAT__EARLY_TERM_IRQ_SUMMARY     (1 << 3)
#define HW_I2C_STAT__OVERSIZE_XFER_TERM_IRQ_SUMMARY     (1 << 4)
#define HW_I2C_STAT__NO_SLAVE_ACK_IRQ_SUMMARY   (1 << 5)
#define HW_I2C_STAT__DATA_ENGINE_COMPLT_IRQ_SUMMARY     (1 << 6)
#define HW_I2C_STAT__BUS_FREE_IRQ_SUMMARY       (1 << 7)
#define HW_I2C_STAT__SLAVE_BUSY                 (1 << 8)
#define HW_I2C_STAT__DATA_ENGINE_BUSY           (1 << 9)
#define HW_I2C_STAT__CLK_GEN_BUSY               (1 << 10)
#define HW_I2C_STAT__BUS_BUSY                   (1 << 11)
#define HW_I2C_STAT__DATA_ENGINE_DMA_WAIT       (1 << 12)
#define HW_I2C_STAT__SLAVE_SEARCHING            (1 << 13)
#define HW_I2C_STAT__SLAVE_FOUND                (1 << 14)
#define HW_I2C_STAT__SLAVE_ADDR_EQ_ZERO         (1 << 15)
#define HW_I2C_STAT__RCVD_SLAVE_ADDR_BM         (0xff << 16)
#define HW_I2C_STAT__RCVD_SLAVE_ADDR_BP         16
#define HW_I2C_STAT__GOT_A_NAK                  (1 << 28)
#define HW_I2C_STAT__ANY_ENABLED_IRQ            (1 << 29)
#define HW_I2C_STAT__MASTER_PRESENT             (1 << 31)

#define HW_I2C_DATA         (*(volatile uint32_t *)(HW_I2C_BASE + 0x60))

#define HW_I2C_DEBUG0       (*(volatile uint32_t *)(HW_I2C_BASE + 0x70))

#define HW_I2C_DEBUG1       (*(volatile uint32_t *)(HW_I2C_BASE + 0x80))

#define HW_I2C_VERSION      (*(volatile uint32_t *)(HW_I2C_BASE + 0x90))

enum imx233_i2c_error_t
{
    I2C_SUCCESS = 0,
    I2C_ERROR = -1,
    I2C_TIMEOUT = -2,
    I2C_MASTER_LOSS = -3,
    I2C_NO_SLAVE_ACK = -4,
    I2C_SLAVE_NAK = -5
};

void imx233_i2c_init(void);
/* start building a transfer, will acquire an exclusive lock */
void imx233_i2c_begin(void);
/* add stage */
enum imx233_i2c_error_t imx233_i2c_add(bool start, bool transmit, void *buffer, unsigned size, bool stop);
/* end building a transfer and start the transfer */
enum imx233_i2c_error_t imx233_i2c_end(unsigned timeout);

#endif // __DMA_IMX233_H__
