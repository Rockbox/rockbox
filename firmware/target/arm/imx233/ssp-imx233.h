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
#ifndef __SSP_IMX233_H__
#define __SSP_IMX233_H__

#include "cpu.h"
#include "system.h"
#include "system-target.h"

/* ssp can value 1 or 2 */
#define __SSP_SELECT(ssp, ssp1, ssp2) ((ssp) == 1 ? (ssp1) : (ssp2))

#define INT_SRC_SSP_DMA(ssp)    __SSP_SELECT(ssp, INT_SRC_SSP1_DMA, INT_SRC_SSP2_DMA)
#define INT_SRC_SSP_ERROR(ssp)  __SSP_SELECT(ssp, INT_SRC_SSP1_ERROR, INT_SRC_SSP2_ERROR)

#define HW_SSP1_BASE        0x80010000
#define HW_SSP2_BASE        0x80034000

#define HW_SSP_BASE(ssp)    __SSP_SELECT(ssp, HW_SSP1_BASE, HW_SSP2_BASE)

#define HW_SSP_CTRL0(ssp)   (*(volatile uint32_t *)(HW_SSP_BASE(ssp) + 0x0))
#define HW_SSP_CTRL0__RUN           (1 << 29)
#define HW_SSP_CTRL0__SDIO_IRQ_CHECK    (1 << 28)
#define HW_SSP_CTRL0__LOCK_CS       (1 << 27)
#define HW_SSP_CTRL0__IGNORE_CRC    (1 << 26)
#define HW_SSP_CTRL0__READ          (1 << 25)
#define HW_SSP_CTRL0__DATA_XFER     (1 << 24)
#define HW_SSP_CTRL0__BUS_WIDTH_BM  (3 << 22)
#define HW_SSP_CTRL0__BUS_WIDTH_BP  22
#define HW_SSP_CTRL0__BUS_WIDTH__ONE_BIT    0
#define HW_SSP_CTRL0__BUS_WIDTH__FOUR_BIT   1
#define HW_SSP_CTRL0__BUS_WIDTH__EIGHT_BIT  2
#define HW_SSP_CTRL0__WAIT_FOR_IRQ  (1 << 21)
#define HW_SSP_CTRL0__WAIT_FOR_CMD  (1 << 20)
#define HW_SSP_CTRL0__LONG_RESP     (1 << 19)
#define HW_SSP_CTRL0__CHECK_RESP    (1 << 18)
#define HW_SSP_CTRL0__GET_RESP      (1 << 17)
#define HW_SSP_CTRL0__ENABLE        (1 << 16)
#define HW_SSP_CTRL0__XFER_COUNT_BM 0xffff


#define HW_SSP_CMD0(ssp)    (*(volatile uint32_t *)(HW_SSP_BASE(ssp) + 0x10))
#define HW_SSP_CMD0__SLOW_CLKING_EN     (1 << 22)
#define HW_SSP_CMD0__CONT_CLKING_EN     (1 << 21)
#define HW_SSP_CMD0__APPEND_8CYC        (1 << 20)
#define HW_SSP_CMD0__BLOCK_SIZE_BM      (0xf << 16)
#define HW_SSP_CMD0__BLOCK_SIZE_BP      16
#define HW_SSP_CMD0__BLOCK_COUNT_BM     (0xff << 8)
#define HW_SSP_CMD0__BLOCK_COUNT_BP     16
#define HW_SSP_CMD0__CMD_BM             0xff

#define HW_SSP_CMD1(ssp)    (*(volatile uint32_t *)(HW_SSP_BASE(ssp) + 0x20))

#define HW_SSP_TIMING(ssp)  (*(volatile uint32_t *)(HW_SSP_BASE(ssp) + 0x50))
#define HW_SSP_TIMING__CLOCK_TIMEOUT_BM 0xffff0000
#define HW_SSP_TIMING__CLOCK_TIMEOUT_BP 16
#define HW_SSP_TIMING__CLOCK_DIVIDE_BM  0xff00
#define HW_SSP_TIMING__CLOCK_DIVIDE_BP  8
#define HW_SSP_TIMING__CLOCK_RATE_BM    0xff

#define HW_SSP_CTRL1(ssp)   (*(volatile uint32_t *)(HW_SSP_BASE(ssp) + 0x60))
#define HW_SSP_CTRL1__SDIO_IRQ          (1 << 31)
#define HW_SSP_CTRL1__SDIO_IRQ_EN       (1 << 30)
#define HW_SSP_CTRL1__RESP_ERR_IRQ      (1 << 29)
#define HW_SSP_CTRL1__RESP_ERR_IRQ_EN   (1 << 28)
#define HW_SSP_CTRL1__RESP_TIMEOUT_IRQ  (1 << 27)
#define HW_SSP_CTRL1__RESP_TIMEOUT_IRQ_EN  (1 << 26)
#define HW_SSP_CTRL1__DATA_TIMEOUT_IRQ  (1 << 25)
#define HW_SSP_CTRL1__DATA_TIMEOUT_IRQ_EN  (1 << 24)
#define HW_SSP_CTRL1__DATA_CRC_IRQ      (1 << 23)
#define HW_SSP_CTRL1__DATA_CRC_IRQ_EN   (1 << 22)
#define HW_SSP_CTRL1__FIFO_UNDERRUN_IRQ (1 << 21)
#define HW_SSP_CTRL1__FIFO_UNDERRUN_IRQ_EN (1 << 20)
#define HW_SSP_CTRL1__RECV_TIMEOUT_IRQ  (1 << 17)
#define HW_SSP_CTRL1__RECV_TIMEOUT_IRQ_EN   (1 << 16)
#define HW_SSP_CTRL1__FIFO_OVERRUN_IRQ  (1 << 15)
#define HW_SSP_CTRL1__FIFO_OVERRUN_IRQ_EN   (1 << 14)
#define HW_SSP_CTRL1__DMA_ENABLE        (1 << 13)
#define HW_SSP_CTRL1__SLAVE_OUT_DISABLE (1 << 11)
#define HW_SSP_CTRL1__PHASE             (1 << 10)
#define HW_SSP_CTRL1__POLARITY          (1 << 9)
#define HW_SSP_CTRL1__SLAVE_MODE        (1 << 8)
#define HW_SSP_CTRL1__WORD_LENGTH_BM    (0xf << 4)
#define HW_SSP_CTRL1__WORD_LENGTH_BP    4
#define HW_SSP_CTRL1__WORD_LENGTH__EIGHT_BITS   0x7
#define HW_SSP_CTRL1__SSP_MODE_BM       0xf
#define HW_SSP_CTRL1__SSP_MODE__SD_MMC  0x3
#define HW_SSP_CTRL1__ALL_IRQ           0xaaa28000

#define HW_SSP_DATA(ssp)   (*(volatile uint32_t *)(HW_SSP_BASE(ssp) + 0x70))

#define HW_SSP_SDRESP0(ssp)   (*(volatile uint32_t *)(HW_SSP_BASE(ssp) + 0x80))
#define HW_SSP_SDRESP1(ssp)   (*(volatile uint32_t *)(HW_SSP_BASE(ssp) + 0x90))
#define HW_SSP_SDRESP2(ssp)   (*(volatile uint32_t *)(HW_SSP_BASE(ssp) + 0xA0))
#define HW_SSP_SDRESP3(ssp)   (*(volatile uint32_t *)(HW_SSP_BASE(ssp) + 0xB0))

#define HW_SSP_STATUS(ssp)   (*(volatile uint32_t *)(HW_SSP_BASE(ssp) + 0xC0))
#define HW_SSP_STATUS__RECV_TIMEOUT_STAT    (1 << 11)
#define HW_SSP_STATUS__TIMEOUT              (1 << 12)
#define HW_SSP_STATUS__DATA_CRC_ERR         (1 << 13)
#define HW_SSP_STATUS__RESP_TIMEOUT         (1 << 14)
#define HW_SSP_STATUS__RESP_ERR             (1 << 15)
#define HW_SSP_STATUS__RESP_CRC_ERR         (1 << 16)
#define HW_SSP_STATUS__ALL_ERRORS           0x1f800

#define HW_SSP_DEBUG(ssp)   (*(volatile uint32_t *)(HW_SSP_BASE(ssp) + 0x100))

#define HW_SSP_VERSION(ssp) (*(volatile uint32_t *)(HW_SSP_BASE(ssp) + 0x110))

enum imx233_ssp_error_t
{
    SSP_SUCCESS = 0,
    SSP_ERROR = -1,
    SSP_TIMEOUT = -2,
};

enum imx233_ssp_resp_t
{
    SSP_NO_RESP = 0,
    SSP_SHORT_RESP,
    SSP_LONG_RESP
};

void imx233_ssp_init(void);
void imx233_ssp_start(int ssp);
void imx233_ssp_stop(int ssp);
/* only softreset between start and stop or it might hang ! */
void imx233_ssp_softreset(int ssp);
void imx233_ssp_set_timings(int ssp, int divide, int rate);
void imx233_ssp_set_timeout(int ssp, int timeout);
void imx233_ssp_set_mode(int ssp, unsigned mode);
/* SD/MMC facilities */
enum imx233_ssp_error_t imx233_ssp_sd_mmc_transfer(int ssp, uint8_t cmd, uint32_t cmd_arg,
    enum imx233_ssp_resp_t resp, void *buffer, int xfer_size, bool read, uint32_t *resp_ptr);
void imx233_ssp_setup_ssp2_sd_mmc_pins(bool enable_pullups, unsigned bus_width,
    unsigned drive_strength);
/* SD/MMC requires that the card be provided the clock during an init sequence of
 * at least 1msec (or 74 clocks). Does NOT touch the clock so it has to be correct. */
void imx233_ssp_sd_mmc_power_up_sequence(int ssp);

#endif /* __SSP_IMX233_H__ */
