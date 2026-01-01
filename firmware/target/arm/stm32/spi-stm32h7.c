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
#include "spi-stm32h7.h"
#include "panic.h"
#include "kernel.h"
#include "regs/stm32h743/spi.h"

/*
 * Align the max transfer size to ensure it will always be a multiple
 * of 32 bits. This is necessary because an unaligned TSIZE will cause
 * some data written to the FIFO to be ignored.
 */
#define TSIZE_MAX \
    ALIGN_DOWN(BM_SPI_CR2_TSIZE >> BP_SPI_CR2_TSIZE, sizeof(uint32_t))

static void stm_spi_enable(struct stm_spi *spi, bool hd_tx, size_t size)
{
    size_t tsize = size / spi->frame_size;
    if (tsize > TSIZE_MAX)
        panicf("%s: tsize > TSIZE_MAX", __func__);

    stm32_clock_enable(spi->clock);

    if (spi->set_cs)
        spi->set_cs(spi, true);

    /* TSIZE must be programmed before setting SPE. */
    reg_assignlf(spi->regs, SPI_CR2, TSIZE(tsize), TSER(0));
    reg_writelf(spi->regs, SPI_CR1, HDDIR(hd_tx), SSI(1), SPE(1));
    reg_assignlf(spi->regs, SPI_IER, RXPIE(1), TXPIE(1), EOTIE(1));

    /* Set CSTART to kick off the transfer */
    reg_writelf(spi->regs, SPI_CR1, CSTART(1));
}

static void stm_spi_disable(struct stm_spi *spi)
{
    reg_assignlf(spi->regs, SPI_IFCR, TSERFC(1), TXTFC(1), EOTC(1));
    reg_writelf(spi->regs, SPI_CR1, SPE(0));

    if (spi->set_cs)
        spi->set_cs(spi, false);

    stm32_clock_disable(spi->clock);
}

static uint32_t stm_spi_pack(const void **bufp, size_t *sizep)
{
    const uint8_t *buf = *bufp;
    uint32_t data = 0;

    if (LIKELY(*sizep >= sizeof(uint32_t)))
    {
        data = load_le32(buf);
        *bufp += sizeof(uint32_t);
        *sizep -= sizeof(uint32_t);
        return data;
    }

    switch (*sizep) {
    case 3: data |= buf[2] << 16; /* fallthrough */
    case 2: data |= buf[1] << 8;  /* fallthrough */
    case 1: data |= buf[0];       /* fallthrough */
    }

    *bufp += *sizep;
    *sizep = 0;
    return data;
}

static void stm_spi_unpack(void **bufp, size_t *sizep, uint32_t data)
{
    uint8_t *buf = *bufp;

    if (LIKELY(*sizep >= sizeof(uint32_t)))
    {
        store_le32(buf, data);
        *bufp += sizeof(uint32_t);
        *sizep -= sizeof(uint32_t);
        return;
    }

    switch (*sizep) {
    case 3: buf[2] = (data >> 16) & 0xff; /* fallthrough */
    case 2: buf[1] = (data >> 8) & 0xff;  /* fallthrough */
    case 1: buf[0] = data & 0xff;         /* fallthrough */
    }

    *bufp += *sizep;
    *sizep = 0;
}

static uint32_t stm_spi_calc_mbr(const struct stm_spi_config *config)
{
    size_t ker_freq = stm32_clock_get_frequency(config->clock);
    for (uint32_t mbr = 0; mbr <= 7; mbr++)
    {
        if (ker_freq / (2 << mbr) <= config->freq)
            return mbr;
    }

    panicf("%s: impossible frequency", __func__);
}

void stm_spi_init(struct stm_spi *spi,
                  const struct stm_spi_config *config)
{
    uint32_t ftlevel;
    uint32_t mbr = stm_spi_calc_mbr(config);

    spi->regs = config->instance;
    spi->clock = config->clock;
    spi->mode = config->mode;
    spi->set_cs = config->set_cs;
    spi->eot_delay_us = 1 + (1000000 / config->freq);

    semaphore_init(&spi->sem, 1, 0);

    /* Set FIFO level based on 32-bit packed writes */
    if (config->frame_bits > 16)
    {
        spi->frame_size = 4;
        ftlevel = 1;
    }
    else if (config->frame_bits > 8)
    {
        spi->frame_size = 2;
        ftlevel = 2;
    }
    else
    {
        spi->frame_size = 1;
        ftlevel = 4;
    }

    /*
     * SPI1-3 have a 16-byte FIFO, SPI4-6 have only 8 bytes.
     * So we can double the threshold setting for SPI1-3.
     * (Maximum allowable threshold is 1/2 the FIFO size.)
     */
    if (config->instance == ITA_SPI1 ||
        config->instance == ITA_SPI2 ||
        config->instance == ITA_SPI3)
    {
        ftlevel *= 2;
    }

    stm32_clock_enable(spi->clock);

    reg_writelf(spi->regs, SPI_CFG1,
                MBR(mbr),
                CRCEN(0),
                CRCSIZE(7),
                TXDMAEN(0),
                RXDMAEN(0),
                UDRDET(0),
                UDRCFG(0),
                FTHLV(ftlevel - 1),
                DSIZE(config->frame_bits - 1));
    reg_writelf(spi->regs, SPI_CFG2,
                AFCNTR(1),
                SSM(config->hw_cs_input ? 0 : 1),
                SSOE(config->hw_cs_output),
                SSIOP(config->hw_cs_polarity),
                CPOL(config->cpol),
                CPHA(config->cpha),
                LSBFIRST(config->send_lsb_first),
                COMM(config->mode),
                SP(config->proto),
                MASTER(1),
                SSOM(0),
                IOSWP(config->swap_mosi_miso),
                MIDI(0),
                MSSI(0));

    stm32_clock_disable(spi->clock);
}

int stm_spi_xfer(struct stm_spi *spi, size_t size,
                 const void *tx_buf, void *rx_buf)
{
    bool hd_tx = false;

    /* Ignore zero-length transfers. */
    if (size == 0)
        return 0;

    /* Reject nonsensical transfers (no data or less than 1 frame) */
    if (!tx_buf && !rx_buf)
        return -1;
    if (size < spi->frame_size)
        return -1;

    if (spi->mode == STM_SPIMODE_HALF_DUPLEX)
    {
        /* Half duplex mode can't support duplex transfers. */
        if (tx_buf && rx_buf)
            return -1;

        if (tx_buf)
            hd_tx = true;
    }

    spi->tx_buf = tx_buf;
    spi->tx_size = tx_buf ? size : 0;

    spi->rx_buf = rx_buf;
    spi->rx_size = rx_buf ? size : 0;

    membarrier();

    stm_spi_enable(spi, hd_tx, size);

    semaphore_wait(&spi->sem, TIMEOUT_BLOCK);

    /*
     * Errata 2.22.2: Master data transfer stall at system clock much faster than SCK
     * Errata 2.22.6: Truncation of SPI output signals after EOT event
     *
     * For 2.22.2 we need to wait at least 1 SCK cycle before starting the
     * next transaction. For 2.22.6, waiting 1/2 SCK cycle should be enough
     * since the EOT event is raised on a clock edge.
     */
    udelay(spi->eot_delay_us);

    stm_spi_disable(spi);
    return 0;
}

void stm_spi_irq_handler(struct stm_spi *spi)
{
    while (true)
    {
        uint32_t sr = reg_readl(spi->regs, SPI_SR);

        if (spi->tx_size == 0 &&
            spi->rx_size == 0 &&
            reg_vreadf(sr, SPI_SR, EOT))
        {
            semaphore_release(&spi->sem);

            reg_writelf(spi->regs, SPI_IFCR, EOTC(1));
            reg_varl(spi->regs, SPI_IER) = 0;
            return;
        }

        if (spi->tx_size > 0 && reg_vreadf(sr, SPI_SR, TXP))
        {
            uint32_t data = stm_spi_pack(&spi->tx_buf, &spi->tx_size);
            if (spi->tx_size == 0)
                reg_writelf(spi->regs, SPI_IER, TXPIE(0));

            reg_varl(spi->regs, SPI_DR) = data;
            continue;
        }

        if (spi->rx_size > 0 &&
            (reg_vreadf(sr, SPI_SR, RXP) || reg_vreadf(sr, SPI_SR, EOT)))
        {
            uint32_t data = reg_readl(spi->regs, SPI_DR);

            stm_spi_unpack(&spi->rx_buf, &spi->rx_size, data);
            if (spi->rx_size == 0)
                reg_writelf(spi->regs, SPI_IER, RXPIE(0));

            continue;
        }

        break;
    }
}
