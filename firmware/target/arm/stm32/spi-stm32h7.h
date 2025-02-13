/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#ifndef __SPI_STM32H743_H__
#define __SPI_STM32H743_H__

#include "system.h"
#include <stddef.h>

struct stm_spi;

enum stm_spi_num
{
    STM_SPI1,
    STM_SPI2,
    STM_SPI3,
    STM_SPI4,
    STM_SPI5,
    STM_SPI6,
    STM_SPI_COUNT,
};

/* Must match the SPI_CFG2.COMM register */
enum stm_spi_mode
{
    STM_SPIMODE_DUPLEX,
    STM_SPIMODE_TXONLY,
    STM_SPIMODE_RXONLY,
    STM_SPIMODE_HALF_DUPLEX,
};

/* Must match the SPI_CFG2.SP register */
enum stm_spi_protocol
{
    STM_SPIPROTO_MOTOROLA,
    STM_SPIPROTO_TI,
};

typedef void (*stm_spi_set_cs_t) (struct stm_spi *spi, bool enable);

struct stm_spi_config
{
    enum stm_spi_num num;
    enum stm_spi_mode mode;
    enum stm_spi_protocol proto;
    stm_spi_set_cs_t set_cs;
    uint32_t frame_bits: 6;
    uint32_t cpha: 1;
    uint32_t cpol: 1;
    uint32_t hw_cs_input: 1;
    uint32_t hw_cs_output: 1;
    uint32_t hw_cs_polarity: 1;
    uint32_t send_lsb_first: 1;
    uint32_t swap_mosi_miso: 1;
};

struct stm_spi
{
    uint32_t regs;
    enum stm_spi_mode mode;
    stm_spi_set_cs_t set_cs;
    uint32_t frame_size;
    size_t tser_left;
};

void stm_spi_init(struct stm_spi *spi,
                  const struct stm_spi_config *config) INIT_ATTR;

int stm_spi_xfer(struct stm_spi *spi, size_t size,
                 const void *tx_buf, void *rx_buf);

static inline int stm_spi_transmit(struct stm_spi *spi,
                                   const void *buf, size_t size)
{
    return stm_spi_xfer(spi, size, buf, NULL);
}

static inline int stm_spi_receive(struct stm_spi *spi,
                                  void *buf, size_t size)
{
    return stm_spi_xfer(spi, size, NULL, buf);
}

#endif /* __SPI_STM32H743_H__ */
