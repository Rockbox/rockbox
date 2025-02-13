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
#include "stm32h7/spi.h"

/*
 * Align the max transfer size to ensure it will always be a multiple
 * of 32 bits. This is necessary because an unaligned TSIZE will cause
 * some data written to the FIFO to be ignored -- this is OK and the
 * intended behavior when it happens at the end of the transfer. But
 * we don't want this to happen when TSER > 0 when we're still in the
 * middle of the transfer, as it will throw away valid data.
 */
#define TSIZE_MAX \
    ALIGN_DOWN(st_vreadf(BM_SPI_CR2_TSIZE, SPI_CR2, TSIZE), sizeof(uint32_t))

static struct stm_spi *spi_map[STM_SPI_COUNT];
static const uint32_t spi_addr[STM_SPI_COUNT] INITDATA_ATTR = {
    [STM_SPI1] = STA_SPI1,
    [STM_SPI2] = STA_SPI2,
    [STM_SPI3] = STA_SPI3,
    [STM_SPI4] = STA_SPI4,
    [STM_SPI5] = STA_SPI5,
    [STM_SPI6] = STA_SPI6,
};

static void stm_spi_set_cs(struct stm_spi *spi, bool enable)
{
    if (spi->set_cs)
        spi->set_cs(spi, enable);

    st_writelf(spi->regs, SPI_CR1, SSI(1));
}

static void stm_spi_enable(struct stm_spi *spi, bool hd_tx, size_t size)
{
    size_t tsize = size / spi->frame_size;
    size_t tser = 0;
    size_t left = 0;

    if (tsize > TSIZE_MAX)
    {
        tser = tsize - TSIZE_MAX;
        tsize = TSIZE_MAX;

        if (tser > TSIZE_MAX)
        {
            left = tser - TSIZE_MAX;
            tser = TSIZE_MAX;
        }
    }

    /*
     * Save number of bytes left for next TSER load, tracked
     * separately from the overall transfer size because the
     * timing of the SPI_SR.TSERF interrupt isn't clear. We'll
     * decrement this by TSIZE_MAX whenever we load TSER in the
     * middle of a transfer.
     */
    spi->tser_left = left;

    /* TSIZE must be programmed before setting SPE. */
    st_overwritelf(spi->regs, SPI_CR2, TSIZE(tsize), TSER(tser));
    st_writelf(spi->regs, SPI_CR1, HDDIR(hd_tx), SPE(1));
}

static void stm_spi_disable(struct stm_spi *spi)
{
    st_overwritelf(spi->regs, SPI_IFCR, TSERFC(1), TXTFC(1), EOTC(1));
    st_writelf(spi->regs, SPI_CR1, SPE(0));
}

static void stm_spi_start(struct stm_spi *spi)
{
    st_writelf(spi->regs, SPI_CR1, CSTART(1));
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

void stm_spi_init(struct stm_spi *spi,
                  const struct stm_spi_config *config)
{
    uint32_t ftlevel;

    spi->regs = spi_addr[config->num];
    spi->mode = config->mode;
    spi->set_cs = config->set_cs;

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
    if (config->num <= STM_SPI3)
        ftlevel *= 2;

    /* TODO: allow setting MBR here */
    st_writelf(spi->regs, SPI_CFG1,
               MBR(0),
               CRCEN(0),
               CRCSIZE(7),
               TXDMAEN(0),
               RXDMAEN(0),
               UDRDET(0),
               UDRCFG(0),
               FTHLV(ftlevel - 1),
               DSIZE(config->frame_bits - 1));
    st_writelf(spi->regs, SPI_CFG2,
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

    spi_map[config->num] = spi;
}

int stm_spi_xfer(struct stm_spi *spi, size_t size,
                 const void *tx_buf, void *rx_buf)
{
    bool hd_tx = false;
    size_t size_tx = tx_buf ? size : 0;
    size_t size_rx = rx_buf ? size : 0;

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

    stm_spi_set_cs(spi, true);
    stm_spi_enable(spi, hd_tx, size);
    stm_spi_start(spi);

    while (size_tx > 0 || size_rx > 0)
    {
        uint32_t sr = st_readl(spi->regs, SPI_SR);

        /*
         * Handle continuation of large transfers
         *
         * TODO - something is not right with this code
         */
        if (spi->tser_left > 0 && st_vreadf(sr, SPI_SR, TSERF))
        {
            if (spi->tser_left < TSIZE_MAX)
            {
                st_writelf(spi->regs, SPI_CR2, TSER(spi->tser_left));
                spi->tser_left = 0;
            }
            else
            {
                st_writelf(spi->regs, SPI_CR2, TSER(TSIZE_MAX));
                spi->tser_left -= TSIZE_MAX;
            }
        }

        /* Handle FIFO write */
        if (size_tx > 0 && st_vreadf(sr, SPI_SR, TXP))
        {
            uint32_t data = stm_spi_pack(&tx_buf, &size_tx);

            st_writel(spi->regs, SPI_TXDR32, data);
        }

        /*
         * Handle FIFO read. Since RXP is set only if the FIFO level
         * exceeds the threshold we can't rely on it at the end of a
         * transfer, and must check EOT as well.
         */
        if (size_rx > 0 &&
            (st_vreadf(sr, SPI_SR, RXP) || st_vreadf(sr, SPI_SR, EOT)))
        {
            uint32_t data = st_readl(spi->regs, SPI_RXDR32);

            stm_spi_unpack(&rx_buf, &size_rx, data);
        }
    }

    /*
     * Errata 2.22.2: Master data transfer stall at system clock much faster than SCK
     * Errata 2.22.6: Truncation of SPI output signals after EOT event
     *
     * For 2.22.2 we need to wait at least 1 SCK cycle before starting the
     * next transaction. For 2.22.6, waiting 1/2 SCK cycle should be enough
     * since the EOT event is raised on a clock edge.
     *
     * TODO: calculate this delay time. doesn't seem to match assumptions above
     */
    udelay(5);

    stm_spi_disable(spi);
    stm_spi_set_cs(spi, false);
    return 0;
}

void spi_irq_handler(void)
{
    while (1);
}
