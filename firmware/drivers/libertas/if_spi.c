/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 by Tomasz Moń
 * Ported from Linux libertas driver
 *   Copyright 2008 Analog Devices Inc.
 *   Authors:
 *   Andrey Yurovsky <andrey@cozybit.com>
 *   Colin McCabe <colin@cozybit.com>
 *   Inspired by if_sdio.c, Copyright 2007-2008 Pierre Ossman
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

#include "config.h"
/*#define LOGF_ENABLE*/
#include "logf.h"
#include "errno.h"
#include "file.h"
#include "panic.h"
#include "system.h"
#include "tick.h"
#include <stddef.h>
#include "if_spi.h"
#include "if_spi_drv.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1)/(d))

struct if_spi_card
{
    /* The card ID and card revision, as reported by the hardware. */
    uint16_t card_id;
    uint8_t  card_rev;

    unsigned long spu_port_delay;
    unsigned long spu_reg_delay;

    uint8_t cmd_buffer[IF_SPI_CMD_BUF_SIZE];
};

#define MODEL_8686    0x0b

static const struct
{
    uint16_t model;
    const char *helper;
    const char *main;
}
fw_table[] =
{
    { MODEL_8686, ROCKBOX_DIR"/libertas/gspi8686_v9_helper.bin", ROCKBOX_DIR"/libertas/gspi8686_v9.bin" },
    { 0, NULL, NULL }
};


/*
 * SPI Interface Unit Routines
 *
 * The SPU sits between the host and the WLAN module.
 * All communication with the firmware is through SPU transactions.
 *
 * First we have to put a SPU register name on the bus. Then we can
 * either read from or write to that register.
 *
 */

static void spu_transaction_init(struct if_spi_card *card)
{
    (void)card;
    /* Linux delays 400 ns if spu_transaction_finish() was called
     * within the same jiffy. As we don't have jiffy counter nor
     * nanosecond delays, simply delay for 1 us. This currently
     * does not really matter as this driver simply loads firmware.
     */
    udelay(1);
    libertas_spi_cs(0); /* assert CS */
}

static void spu_transaction_finish(struct if_spi_card *card)
{
    (void)card;
    libertas_spi_cs(1); /* drop CS */
}

/*
 * Write out a byte buffer to an SPI register,
 * using a series of 16-bit transfers.
 */
static int spu_write(struct if_spi_card *card, uint16_t reg, const uint8_t *buf, int len)
{
    int err = 0;
    uint8_t reg_out[2];

    /* You must give an even number of bytes to the SPU, even if it
     * doesn't care about the last one.  */
    if (len & 0x1)
        panicf("Odd length in spu_write()");

    reg |= IF_SPI_WRITE_OPERATION_MASK;
    reg_out[0] = (reg & 0x00FF);
    reg_out[1] = (reg & 0xFF00) >> 8;

    spu_transaction_init(card);
    libertas_spi_tx(reg_out, sizeof(reg_out));
    libertas_spi_tx(buf, len);
    spu_transaction_finish(card);
    return err;
}

static inline int spu_write_u16(struct if_spi_card *card, uint16_t reg, uint16_t val)
{
    uint8_t buf[2];
    buf[0] = (val & 0x00FF);
    buf[1] = (val & 0xFF00) >> 8;
    return spu_write(card, reg, buf, sizeof(buf));
}

static inline int spu_reg_is_port_reg(uint16_t reg)
{
    switch (reg)
    {
    case IF_SPI_IO_RDWRPORT_REG:
    case IF_SPI_CMD_RDWRPORT_REG:
    case IF_SPI_DATA_RDWRPORT_REG:
        return 1;
    default:
        return 0;
    }
}

static int spu_read(struct if_spi_card *card, uint16_t reg, uint8_t *buf, int len)
{
    unsigned int delay;
    int err = 0;
    uint8_t reg_out[2];

    /*
     * You must take an even number of bytes from the SPU, even if you
     * don't care about the last one.
     */
    if (len & 0x1)
        panicf("Odd length in spu_read()");

    reg |= IF_SPI_READ_OPERATION_MASK;
    reg_out[0] = (reg & 0x00FF);
    reg_out[1] = (reg & 0xFF00) >> 8;

    spu_transaction_init(card);
    libertas_spi_tx(reg_out, sizeof(reg_out));

    delay = spu_reg_is_port_reg(reg) ? card->spu_port_delay : card->spu_reg_delay;
    /* Busy-wait while the SPU fills the FIFO */
    delay = DIV_ROUND_UP((100 + (delay * 10)), 1000);
    if (delay < 1000)
        udelay(delay);
    else
        mdelay(DIV_ROUND_UP(delay, 1000));

    libertas_spi_rx(buf, len);
    spu_transaction_finish(card);
    return err;
}

/* Read 16 bits from an SPI register */
static inline int spu_read_u16(struct if_spi_card *card, uint16_t reg, uint16_t *val)
{
    uint8_t buf[2];
    int ret;

    ret = spu_read(card, reg, buf, sizeof(buf));
    if (ret == 0)
        *val = buf[0] | (buf[1] << 8);

    return ret;
}

/*
 * Read 32 bits from an SPI register.
 * The low 16 bits are read first.
 */
static int spu_read_u32(struct if_spi_card *card, uint16_t reg, uint32_t *val)
{
    uint8_t buf[4];
    int err;

    err = spu_read(card, reg, buf, sizeof(buf));
    if (!err)
        *val = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
    return err;
}

/*
 * Keep reading 16 bits from an SPI register until you get the correct result.
 *
 * If mask = 0, the correct result is any non-zero number.
 * If mask != 0, the correct result is any number where
 * number & target_mask == target
 *
 * Returns -ETIMEDOUT if a five seconds passes without the correct result.
 */
static int spu_wait_for_u16(struct if_spi_card *card, uint16_t reg,
                            uint16_t target_mask, uint16_t target)
{
    int err;
    unsigned long timeout = current_tick + 5*HZ;
    while (1)
    {
        uint16_t val;
        err = spu_read_u16(card, reg, &val);
        if (err)
            return err;
        if (target_mask)
        {
            if ((val & target_mask) == target)
                return 0;
        }
        else
        {
            if (val)
                return 0;
        }
        udelay(100);
        if (TIME_AFTER(current_tick, timeout))
        {
            logf("%s: timeout with val=%02x, target_mask=%02x, target=%02x",
                 __func__, val, target_mask, target);
            return -ETIMEDOUT;
        }
    }
}

/*
 * Read 16 bits from an SPI register until you receive a specific value.
 * Returns -ETIMEDOUT if a 4 tries pass without success.
 */
static int spu_wait_for_u32(struct if_spi_card *card, uint32_t reg, uint32_t target)
{
    int err, try;
    for (try = 0; try < 4; ++try)
    {
        uint32_t val = 0;
        err = spu_read_u32(card, reg, &val);
        if (err)
            return err;
        if (val == target)
            return 0;
        mdelay(100);
    }
    return -ETIMEDOUT;
}

static int spu_set_interrupt_mode(struct if_spi_card *card,
                                  int suppress_host_int,
                                  int auto_int)
{
    int err = 0;

    /*
     * We can suppress a host interrupt by clearing the appropriate
     * bit in the "host interrupt status mask" register
     */
    if (suppress_host_int) {
        err = spu_write_u16(card, IF_SPI_HOST_INT_STATUS_MASK_REG, 0);
        if (err)
            return err;
    } else {
        err = spu_write_u16(card, IF_SPI_HOST_INT_STATUS_MASK_REG,
                            IF_SPI_HISM_TX_DOWNLOAD_RDY |
                            IF_SPI_HISM_RX_UPLOAD_RDY |
                            IF_SPI_HISM_CMD_DOWNLOAD_RDY |
                            IF_SPI_HISM_CARDEVENT |
                            IF_SPI_HISM_CMD_UPLOAD_RDY);
        if (err)
            return err;
    }

    /*
     * If auto-interrupts are on, the completion of certain transactions
     * will trigger an interrupt automatically. If auto-interrupts
     * are off, we need to set the "Card Interrupt Cause" register to
     * trigger a card interrupt.
     */
    if (auto_int) {
        err = spu_write_u16(card, IF_SPI_HOST_INT_CTRL_REG,
                            IF_SPI_HICT_TX_DOWNLOAD_OVER_AUTO |
                            IF_SPI_HICT_RX_UPLOAD_OVER_AUTO |
                            IF_SPI_HICT_CMD_DOWNLOAD_OVER_AUTO |
                            IF_SPI_HICT_CMD_UPLOAD_OVER_AUTO);
        if (err)
            return err;
    } else {
        err = spu_write_u16(card, IF_SPI_HOST_INT_STATUS_MASK_REG, 0);
        if (err)
            return err;
    }
    return err;
}

static int spu_get_chip_revision(struct if_spi_card *card,
                                 uint16_t *card_id, uint8_t *card_rev)
{
    int err = 0;
    uint32_t dev_ctrl;
    err = spu_read_u32(card, IF_SPI_DEVICEID_CTRL_REG, &dev_ctrl);
    if (err)
        return err;
    *card_id = IF_SPI_DEVICEID_CTRL_REG_TO_CARD_ID(dev_ctrl);
    *card_rev = IF_SPI_DEVICEID_CTRL_REG_TO_CARD_REV(dev_ctrl);
    return err;
}

static int spu_set_bus_mode(struct if_spi_card *card, uint16_t mode)
{
    int err = 0;
    uint16_t rval;
    /* set bus mode */
    err = spu_write_u16(card, IF_SPI_SPU_BUS_MODE_REG, mode);
    if (err)
        return err;
    /* Check that we were able to read back what we just wrote. */
    err = spu_read_u16(card, IF_SPI_SPU_BUS_MODE_REG, &rval);
    if (err)
        return err;
    if ((rval & 0xF) != mode)
    {
        logf("Can't read bus mode register");
        return -EIO;
    }
    return 0;
}

static int spu_init(struct if_spi_card *card)
{
    int err = 0;
    uint32_t delay;

    err = spu_set_bus_mode(card,
                           IF_SPI_BUS_MODE_SPI_CLOCK_PHASE_RISING |
                           IF_SPI_BUS_MODE_DELAY_METHOD_TIMED |
                           IF_SPI_BUS_MODE_16_BIT_ADDRESS_16_BIT_DATA);
    if (err)
        return err;
    card->spu_port_delay = 1000;
    card->spu_reg_delay = 1000;
    err = spu_read_u32(card, IF_SPI_DELAY_READ_REG, &delay);
    if (err)
        return err;
    card->spu_port_delay = delay & 0x0000ffff;
    card->spu_reg_delay = (delay & 0xffff0000) >> 16;

    logf("Initialized SPU unit. "
         "spu_port_delay=0x%04lx, spu_reg_delay=0x%04lx",
         card->spu_port_delay, card->spu_reg_delay);
    return err;
}


/*
 * Firmware Loading
 */

static int if_spi_prog_helper_firmware(struct if_spi_card *card, int fd)
{
    int err = 0;
    int bytes_read;
    uint8_t *temp = card->cmd_buffer;

    err = spu_set_interrupt_mode(card, 1, 0);
    if (err)
        goto out;

    /* Load helper firmware image */
    while ((bytes_read = read(fd, temp, HELPER_FW_LOAD_CHUNK_SZ)) > 0)
    {
        /*
         * Scratch pad 1 should contain the number of bytes we
         * want to download to the firmware
         */
        err = spu_write_u16(card, IF_SPI_SCRATCH_1_REG,
                            HELPER_FW_LOAD_CHUNK_SZ);
        if (err)
            goto out;

        err = spu_wait_for_u16(card, IF_SPI_HOST_INT_STATUS_REG,
                               IF_SPI_HIST_CMD_DOWNLOAD_RDY,
                               IF_SPI_HIST_CMD_DOWNLOAD_RDY);
        if (err)
            goto out;

        /*
         * Feed the data into the command read/write port reg
         * in chunks of 64 bytes
         */
        memset(temp + bytes_read, 0, HELPER_FW_LOAD_CHUNK_SZ - bytes_read);
        mdelay(10);
        err = spu_write(card, IF_SPI_CMD_RDWRPORT_REG,
                        temp, HELPER_FW_LOAD_CHUNK_SZ);
        if (err)
            goto out;

        /* Interrupt the boot code */
        err = spu_write_u16(card, IF_SPI_HOST_INT_STATUS_REG, 0);
        if (err)
            goto out;
        err = spu_write_u16(card, IF_SPI_CARD_INT_CAUSE_REG,
                            IF_SPI_CIC_CMD_DOWNLOAD_OVER);
        if (err)
            goto out;
    }

    /*
     * Once the helper / single stage firmware download is complete,
     * write 0 to scratch pad 1 and interrupt the
     * bootloader. This completes the helper download.
     */
    err = spu_write_u16(card, IF_SPI_SCRATCH_1_REG, FIRMWARE_DNLD_OK);
    if (err)
        goto out;
    err = spu_write_u16(card, IF_SPI_HOST_INT_STATUS_REG, 0);
    if (err)
        goto out;
    err = spu_write_u16(card, IF_SPI_CARD_INT_CAUSE_REG,
                        IF_SPI_CIC_CMD_DOWNLOAD_OVER);
out:
    if (err)
        logf("failed to load helper firmware (err=%d)", err);

    return err;
}

/*
 * Returns the length of the next packet the firmware expects us to send.
 * Sets crc_err if the previous transfer had a CRC error.
 */
static int if_spi_prog_main_firmware_check_len(struct if_spi_card *card,
                                               int *crc_err)
{
    uint16_t len;
    int err = 0;

    /*
     * wait until the host interrupt status register indicates
     * that we are ready to download
     */
    err = spu_wait_for_u16(card, IF_SPI_HOST_INT_STATUS_REG,
                           IF_SPI_HIST_CMD_DOWNLOAD_RDY,
                           IF_SPI_HIST_CMD_DOWNLOAD_RDY);
    if (err)
    {
        logf("timed out waiting for host_int_status");
        return err;
    }

    /* Ask the device how many bytes of firmware it wants. */
    err = spu_read_u16(card, IF_SPI_SCRATCH_1_REG, &len);
    if (err)
        return err;

    if (len > IF_SPI_CMD_BUF_SIZE)
    {
        logf("firmware load device requested a larger transfer than we are prepared to handle (len = %d)",
             len);
        return -EIO;
    }
    if (len & 0x1) {
        logf("%s: crc error", __func__);
        len &= ~0x1;
        *crc_err = 1;
    } else
        *crc_err = 0;

    return len;
}

static int if_spi_prog_main_firmware(struct if_spi_card *card, int fd)
{
    int len;
    int bytes_read = 0, crc_err = 0, err = 0;
    uint16_t num_crc_errs;

    err = spu_set_interrupt_mode(card, 1, 0);
    if (err)
        goto out;

    err = spu_wait_for_u16(card, IF_SPI_SCRATCH_1_REG, 0, 0);
    if (err)
    {
        logf("%s: timed out waiting for initial scratch reg = 0", __func__);
        goto out;
    }

    num_crc_errs = 0;
    while ((len = if_spi_prog_main_firmware_check_len(card, &crc_err)))
    {
        if (len < 0)
        {
            err = len;
            goto out;
        }
        if (crc_err)
        {
            /* Previous transfer failed. */
            if (++num_crc_errs > MAX_MAIN_FW_LOAD_CRC_ERR)
            {
                logf("Too many CRC errors encountered in firmware load.");
                err = -EIO;
                goto out;
            }

            /* Rewind so we read back the data from previous transfer */
            lseek(fd, -bytes_read, SEEK_CUR);
        }

        bytes_read = read(fd, card->cmd_buffer, len);
        if (bytes_read < 0)
        {
            /*
             * If there are no more bytes left, we would normally
             * expect to have terminated with len = 0
             */
            logf("Firmware load wants more bytes than we have to offer.");
            break;
        }
        else if (bytes_read < len)
        {
            memset(card->cmd_buffer + bytes_read, 0, len - bytes_read);
        }

        err = spu_write_u16(card, IF_SPI_HOST_INT_STATUS_REG, 0);
        if (err)
            goto out;
        err = spu_write(card, IF_SPI_CMD_RDWRPORT_REG, card->cmd_buffer, len);
        if (err)
            goto out;
        err = spu_write_u16(card, IF_SPI_CARD_INT_CAUSE_REG,
                            IF_SPI_CIC_CMD_DOWNLOAD_OVER);
        if (err)
            goto out;
    }
    if (read(fd, card->cmd_buffer, IF_SPI_CMD_BUF_SIZE) > 0)
    {
        logf("firmware load wants fewer bytes than we have to offer");
    }

    /* Confirm firmware download */
    err = spu_wait_for_u32(card, IF_SPI_SCRATCH_4_REG,
                           SUCCESSFUL_FW_DOWNLOAD_MAGIC);
    if (err)
    {
        logf("failed to confirm the firmware download");
        goto out;
    }

out:
    if (err)
        logf("failed to load firmware (err=%d)", err);

    return err;
}

static int if_spi_init_card(struct if_spi_card *card)
{
    int err;
    size_t i;
    uint32_t scratch;
    int fd;

    err = spu_init(card);
    if (err)
        goto out;
    err = spu_get_chip_revision(card, &card->card_id, &card->card_rev);
    if (err)
        goto out;

    err = spu_read_u32(card, IF_SPI_SCRATCH_4_REG, &scratch);
    if (err)
        goto out;
    if (scratch == SUCCESSFUL_FW_DOWNLOAD_MAGIC)
        logf("Firmware is already loaded for Marvell WLAN 802.11 adapter");
    else {
        /* Check if we support this card */
        for (i = 0; i < ARRAY_SIZE(fw_table); i++) {
            if (card->card_id == fw_table[i].model)
                break;
        }
        if (i == ARRAY_SIZE(fw_table)) {
            logf("Unsupported chip_id: 0x%02x", card->card_id);
            err = -ENODEV;
            goto out;
        }

        logf("Initializing FW for Marvell WLAN 802.11 adapter "
             "(chip_id = 0x%04x, chip_rev = 0x%02x)",
             card->card_id, card->card_rev);

        fd = open(fw_table[i].helper, O_RDONLY);
        if (fd >= 0)
        {
            err = if_spi_prog_helper_firmware(card, fd);
            close(fd);
            if (err)
                goto out;
        }
        else
        {
            logf("failed to find firmware helper (%s)", fw_table[i].helper);
            err = -ENOENT;
            goto out;
        }

        fd = open(fw_table[i].main, O_RDONLY);
        if (fd >= 0)
        {
            err = if_spi_prog_main_firmware(card, fd);
            close(fd);
            if (err)
                goto out;
        }
        else
        {
            logf("failed to find firmware (%s)", fw_table[i].main);
            err = -ENOENT;
            goto out;
        }

        logf("loaded FW for Marvell WLAN 802.11 adapter");
    }

    err = spu_set_interrupt_mode(card, 0, 1);
    if (err)
        goto out;

out:
    return err;
}

void wifi_init(void) INIT_ATTR
{
#if 0
    static struct if_spi_card card;
    libertas_spi_init();
    libertas_spi_pd(1);
    libertas_spi_reset(1);
    mdelay(100);
    if (!if_spi_init_card(&card))
    {
        /* TODO: Configure card and enter deep sleep */
    }
    else
#else
    libertas_spi_init();
    (void)if_spi_init_card;
#endif
    {
        /* Keep the lines in lowest power configuration */
        libertas_spi_pd(0);
        libertas_spi_reset(1);
        libertas_spi_cs(1);
    }
}
