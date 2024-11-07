/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
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
#include "storage.h"
#include "timer.h"
#include "kernel.h"
#include "string.h"
#include "power.h"
#include "panic.h"
#include "mmu-arm.h"
#include "mmcdefs-target.h"
#include "s5l8702.h"
#include "led.h"
#include "debug.h"
#include "panic.h"
#include "fs_defines.h"

#ifndef ATA_RETRIES
#define ATA_RETRIES 3
#endif

#define CMD_READ_SECTORS           0x20
#define CMD_READ_DMA_EXT           0x25
#define CMD_READ_MULTIPLE_EXT      0x29
#define CMD_WRITE_SECTORS          0x30
#define CMD_WRITE_DMA_EXT          0x35
#define CMD_WRITE_MULTIPLE_EXT     0x39
#ifdef HAVE_ATA_SMART
#define CMD_SMART                  0xB0
#endif
#define CMD_READ_MULTIPLE          0xC4
#define CMD_WRITE_MULTIPLE         0xC5
#define CMD_READ_DMA               0xC8
#define CMD_WRITE_DMA              0xCA
#define CMD_STANDBY_IMMEDIATE      0xE0
#define CMD_FLUSH_CACHE            0xE7
#define CMD_FLUSH_CACHE_EXT        0xEA
#define CMD_IDENTIFY               0xEC
#define CMD_SET_FEATURES           0xEF

#define CEATA_POWERUP_TIMEOUT 20000000
#define CEATA_COMMAND_TIMEOUT 1000000
#define CEATA_DAT_NONBUSY_TIMEOUT 5000000
#define CEATA_MMC_RCA 1

/** static, private data **/
static uint8_t ceata_taskfile[16] STORAGE_ALIGN_ATTR;
static uint16_t identify_info[ATA_IDENTIFY_WORDS] STORAGE_ALIGN_ATTR;
static bool ceata;
static bool ata_lba48;
static bool ata_dma;
static uint64_t ata_total_sectors;
static uint32_t log_sector_size;
static struct mutex ata_mutex;
static struct semaphore ata_wakeup;
static uint32_t ata_dma_flags;
static long ata_last_activity_value = -1;
static long ata_sleep_timeout = 7 * HZ;
static bool ata_powered;
static bool canflush = true;
static struct semaphore mmc_wakeup;
static struct semaphore mmc_comp_wakeup;
static int spinup_time = 0;
static int dma_mode = 0;

static const int ata_retries = ATA_RETRIES;
static const bool ata_error_srst = true;

static int ata_reset(void);

static uint16_t ata_read_cbr(uint32_t volatile* reg)
{
    while (!(ATA_PIO_READY & 2));
    volatile uint32_t dummy __attribute__((unused)) = *reg;
    while (!(ATA_PIO_READY & 1));
    return ATA_PIO_RDATA;
}

static void ata_write_cbr(uint32_t volatile* reg, uint16_t data)
{
    while (!(ATA_PIO_READY & 2));
    *reg = data;
}

static int ata_wait_for_not_bsy(long timeout)
{
    long startusec = USEC_TIMER;
    while (true)
    {
        uint8_t csd = ata_read_cbr(&ATA_PIO_CSD);
        if (!(csd & BIT(7)))
            return 0;
        if (TIMEOUT_EXPIRED(startusec, timeout))
            RET_ERR(0);
        yield();
    }
}

static int ata_wait_for_rdy(long timeout)
{
    long startusec = USEC_TIMER;
    PASS_RC(ata_wait_for_not_bsy(timeout), 1, 0);
    while (true)
    {
        uint8_t dad = ata_read_cbr(&ATA_PIO_DAD);
        if (dad & BIT(6))
            return 0;
        if (TIMEOUT_EXPIRED(startusec, timeout))
            RET_ERR(1);
        yield();
    }
}

static int ata_wait_for_start_of_transfer(long timeout)
{
    long startusec = USEC_TIMER;
    PASS_RC(ata_wait_for_not_bsy(timeout), 2, 0);
    while (true)
    {
        uint8_t dad = ata_read_cbr(&ATA_PIO_DAD);
        if (dad & BIT(0))
            RET_ERR(1);
        if ((dad & (BIT(7) | BIT(3))) == BIT(3))
            return 0;
        if (TIMEOUT_EXPIRED(startusec, timeout))
            RET_ERR(2);
        yield();
    }
}

static int ata_wait_for_end_of_transfer(long timeout)
{
    PASS_RC(ata_wait_for_not_bsy(timeout), 2, 0);
    uint8_t dad = ata_read_cbr(&ATA_PIO_DAD);
    if (dad & BIT(0))
        RET_ERR(1);
    if ((dad & (BIT(3) | BITRANGE(5, 7))) == BIT(6))
        return 0;
    RET_ERR(2);
}

static int mmc_dsta_check_command_success(bool disable_crc)
{
    int rc = 0;
    uint32_t dsta = SDCI_DSTA;
    if (dsta & SDCI_DSTA_RESTOUTE)
        rc |= 1;
    if (dsta & SDCI_DSTA_RESENDE)
        rc |= 2;
    if (dsta & SDCI_DSTA_RESINDE)
        rc |= 4;
    if (!disable_crc)
        if (dsta & SDCI_DSTA_RESCRCE)
            rc |= 8;
    if (rc)
        RET_ERR(rc);
    return 0;
}

static bool mmc_send_command(uint32_t cmd, uint32_t arg, uint32_t* result, int timeout)
{
    long starttime = USEC_TIMER;
    while ((SDCI_STATE & SDCI_STATE_CMD_STATE_MASK) != SDCI_STATE_CMD_STATE_CMD_IDLE)
    {
        if (TIMEOUT_EXPIRED(starttime, timeout))
            RET_ERR(0);
        yield();
    }
    SDCI_STAC = SDCI_STAC_CLR_CMDEND | SDCI_STAC_CLR_BIT_3
              | SDCI_STAC_CLR_RESEND | SDCI_STAC_CLR_DATEND
              | SDCI_STAC_CLR_DAT_CRCEND | SDCI_STAC_CLR_CRC_STAEND
              | SDCI_STAC_CLR_RESTOUTE | SDCI_STAC_CLR_RESENDE
              | SDCI_STAC_CLR_RESINDE | SDCI_STAC_CLR_RESCRCE
              | SDCI_STAC_CLR_WR_DATCRCE | SDCI_STAC_CLR_RD_DATCRCE
              | SDCI_STAC_CLR_RD_DATENDE0 | SDCI_STAC_CLR_RD_DATENDE1
              | SDCI_STAC_CLR_RD_DATENDE2 | SDCI_STAC_CLR_RD_DATENDE3
              | SDCI_STAC_CLR_RD_DATENDE4 | SDCI_STAC_CLR_RD_DATENDE5
              | SDCI_STAC_CLR_RD_DATENDE6 | SDCI_STAC_CLR_RD_DATENDE7;
    SDCI_ARGU = arg;
    SDCI_CMD = cmd;
    if (!(SDCI_DSTA & SDCI_DSTA_CMDRDY))
        RET_ERR(1);
    SDCI_CMD = cmd | SDCI_CMD_CMDSTR;
    long sleepbase = USEC_TIMER;
    while (TIMEOUT_EXPIRED(sleepbase, 1000))
        yield();
    while (!(SDCI_DSTA & SDCI_DSTA_CMDEND))
    {
        if (TIMEOUT_EXPIRED(starttime, timeout))
            RET_ERR(2);
        yield();
    }
    if ((cmd & SDCI_CMD_RES_TYPE_MASK) != SDCI_CMD_RES_TYPE_NONE)
    {
        while (!(SDCI_DSTA & SDCI_DSTA_RESEND))
        {
            if (TIMEOUT_EXPIRED(starttime, timeout))
                RET_ERR(3);
            yield();
        }
        if (cmd & SDCI_CMD_RES_BUSY)
            while (SDCI_DSTA & SDCI_DSTA_DAT_BUSY)
            {
                if (TIMEOUT_EXPIRED(starttime, CEATA_DAT_NONBUSY_TIMEOUT))
                    RET_ERR(4);
                yield();
            }
    }
    bool nocrc = (cmd & SDCI_CMD_RES_SIZE_MASK) == SDCI_CMD_RES_SIZE_136;
    PASS_RC(mmc_dsta_check_command_success(nocrc), 3, 5);
    if (result)
        *result = SDCI_RESP0;
    return 0;
}

static int mmc_get_card_status(uint32_t* result)
{
    return mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_SEND_STATUS)
                          | SDCI_CMD_CMD_TYPE_AC | SDCI_CMD_RES_TYPE_R1
                          | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                            MMC_CMD_SEND_STATUS_RCA(CEATA_MMC_RCA), result, CEATA_COMMAND_TIMEOUT);
}

static int mmc_init(void)
{
    sleep(HZ / 10);
    PASS_RC(mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_GO_IDLE_STATE)
                           | SDCI_CMD_CMD_TYPE_BC | SDCI_CMD_RES_TYPE_NONE
                           | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NID,
                             0, NULL, CEATA_COMMAND_TIMEOUT), 3, 0);
    long startusec = USEC_TIMER;
    uint32_t result;
    do
    {
        if (TIMEOUT_EXPIRED(startusec, CEATA_POWERUP_TIMEOUT))
            RET_ERR(1);
        sleep(HZ / 100);
        PASS_RC(mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_SEND_OP_COND)
                               | SDCI_CMD_CMD_TYPE_BCR | SDCI_CMD_RES_TYPE_R3
                               | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NID,
                                 MMC_CMD_SEND_OP_COND_OCR(MMC_OCR_270_360),
                                 NULL, CEATA_COMMAND_TIMEOUT), 3, 2);
        result = SDCI_RESP0;
    }
    while (!(result & MMC_OCR_POWER_UP_DONE));
    PASS_RC(mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_ALL_SEND_CID)
                           | SDCI_CMD_CMD_TYPE_BCR | SDCI_CMD_RES_TYPE_R2
                           | SDCI_CMD_RES_SIZE_136 | SDCI_CMD_NCR_NID_NID,
                             0, NULL, CEATA_COMMAND_TIMEOUT), 3, 3);
    PASS_RC(mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_SET_RELATIVE_ADDR)
                           | SDCI_CMD_CMD_TYPE_BCR | SDCI_CMD_RES_TYPE_R1
                           | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                             MMC_CMD_SET_RELATIVE_ADDR_RCA(CEATA_MMC_RCA),
                             NULL, CEATA_COMMAND_TIMEOUT), 3, 4);
    PASS_RC(mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_SELECT_CARD)
                           | SDCI_CMD_CMD_TYPE_AC | SDCI_CMD_RES_TYPE_R1
                           | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                             MMC_CMD_SELECT_CARD_RCA(CEATA_MMC_RCA),
                             NULL, CEATA_COMMAND_TIMEOUT), 3, 5);
    PASS_RC(mmc_get_card_status(&result), 3, 6);
    if ((result & MMC_STATUS_CURRENT_STATE_MASK) != MMC_STATUS_CURRENT_STATE_TRAN)
        RET_ERR(7);
    return 0;
}

static int mmc_fastio_write(uint32_t addr, uint32_t data)
{
    return mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_FAST_IO)
                          | SDCI_CMD_CMD_TYPE_AC | SDCI_CMD_RES_TYPE_R4
                          | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                            MMC_CMD_FAST_IO_RCA(CEATA_MMC_RCA) | MMC_CMD_FAST_IO_DIRECTION_WRITE
                          | MMC_CMD_FAST_IO_ADDRESS(addr) | MMC_CMD_FAST_IO_DATA(data),
                            NULL, CEATA_COMMAND_TIMEOUT);
}

static int mmc_fastio_read(uint32_t addr, uint32_t* data)
{
    return mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_FAST_IO)
                          | SDCI_CMD_CMD_TYPE_AC | SDCI_CMD_RES_TYPE_R4
                          | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                            MMC_CMD_FAST_IO_RCA(CEATA_MMC_RCA) | MMC_CMD_FAST_IO_DIRECTION_READ
                          | MMC_CMD_FAST_IO_ADDRESS(addr), data, CEATA_COMMAND_TIMEOUT);
}

static int ceata_soft_reset(void)
{
    PASS_RC(mmc_fastio_write(6, 4), 2, 0);
    sleep(HZ / 100);
    PASS_RC(mmc_fastio_write(6, 0), 2, 1);
    sleep(HZ / 100);
    long startusec = USEC_TIMER;
    uint32_t status;
    do
    {
        PASS_RC(mmc_fastio_read(0xf, &status), 2, 2);
        if (TIMEOUT_EXPIRED(startusec, CEATA_POWERUP_TIMEOUT))
            RET_ERR(3);
        sleep(HZ / 100);
    }
    while (status & 0x80);
    return 0;
}

static int mmc_dsta_check_data_success(void)
{
    int rc = 0;
    uint32_t dsta = SDCI_DSTA;
    if (dsta & (SDCI_DSTA_WR_DATCRCE | SDCI_DSTA_RD_DATCRCE))
    {
        if (dsta & SDCI_DSTA_WR_DATCRCE)
            rc |= 1;
        if (dsta & SDCI_DSTA_RD_DATCRCE)
            rc |= 2;
        if ((dsta & SDCI_DSTA_WR_CRC_STATUS_MASK) == SDCI_DSTA_WR_CRC_STATUS_TXERR)
            rc |= 4;
        else if ((dsta & SDCI_DSTA_WR_CRC_STATUS_MASK) == SDCI_DSTA_WR_CRC_STATUS_CARDERR)
            rc |= 8;
    }
    if (dsta & (SDCI_DSTA_RD_DATENDE0 | SDCI_DSTA_RD_DATENDE1 | SDCI_DSTA_RD_DATENDE2
              | SDCI_DSTA_RD_DATENDE3 | SDCI_DSTA_RD_DATENDE4 | SDCI_DSTA_RD_DATENDE5
              | SDCI_DSTA_RD_DATENDE6 | SDCI_DSTA_RD_DATENDE7))
        rc |= 16;
    if (rc)
        RET_ERR(rc);
    return 0;
}

static void mmc_discard_irq(void)
{
    SDCI_IRQ = SDCI_IRQ_DAT_DONE_INT | SDCI_IRQ_MASK_MASK_IOCARD_IRQ_INT
             | SDCI_IRQ_MASK_MASK_READ_WAIT_INT;
    semaphore_wait(&mmc_wakeup, 0);
}

static int ceata_read_multiple_register(uint32_t addr, void* dest, uint32_t size)
{
    if (size > 0x10)
        RET_ERR(0);
    mmc_discard_irq();
    SDCI_DMASIZE = size;
    SDCI_DMACOUNT = 1;
    SDCI_DMAADDR = dest;
    SDCI_DCTRL = SDCI_DCTRL_TXFIFORST | SDCI_DCTRL_RXFIFORST;
    commit_discard_dcache();
    PASS_RC(mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_CEATA_RW_MULTIPLE_REG)
                           | SDCI_CMD_CMD_TYPE_ADTC | SDCI_CMD_RES_TYPE_R1
                           | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                             MMC_CMD_CEATA_RW_MULTIPLE_REG_DIRECTION_READ
                           | MMC_CMD_CEATA_RW_MULTIPLE_REG_ADDRESS(addr & 0xfc)
                           | MMC_CMD_CEATA_RW_MULTIPLE_REG_COUNT(size & 0xfc),
                             NULL, CEATA_COMMAND_TIMEOUT), 2, 1);
    if (semaphore_wait(&mmc_wakeup, CEATA_COMMAND_TIMEOUT * HZ / 1000000) == OBJ_WAIT_TIMEDOUT)
        RET_ERR(2);
    PASS_RC(mmc_dsta_check_data_success(), 2, 3);
    return 0;
}

static int ceata_write_multiple_register(uint32_t addr, void* dest, uint32_t size)
{
    uint32_t i;
    if (size > 0x10)
        RET_ERR(0);
    mmc_discard_irq();
    SDCI_DMASIZE = size;
    SDCI_DMACOUNT = 0;
    SDCI_DCTRL = SDCI_DCTRL_TXFIFORST | SDCI_DCTRL_RXFIFORST;
    PASS_RC(mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_CEATA_RW_MULTIPLE_REG)
                           | SDCI_CMD_CMD_TYPE_ADTC | SDCI_CMD_CMD_RD_WR
                           | SDCI_CMD_RES_BUSY | SDCI_CMD_RES_TYPE_R1
                           | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                             MMC_CMD_CEATA_RW_MULTIPLE_REG_DIRECTION_WRITE
                           | MMC_CMD_CEATA_RW_MULTIPLE_REG_ADDRESS(addr & 0xfc)
                           | MMC_CMD_CEATA_RW_MULTIPLE_REG_COUNT(size & 0xfc),
                             NULL, CEATA_COMMAND_TIMEOUT), 3, 1);
    SDCI_DCTRL = SDCI_DCTRL_TRCONT_TX;
    for (i = 0; i < size / 4; i++)
        SDCI_DATA = ((uint32_t*)dest)[i];
    long startusec = USEC_TIMER;
    if (semaphore_wait(&mmc_wakeup, CEATA_COMMAND_TIMEOUT * HZ / 1000000) == OBJ_WAIT_TIMEDOUT)
        RET_ERR(2);
    while ((SDCI_STATE & SDCI_STATE_DAT_STATE_MASK) != SDCI_STATE_DAT_STATE_IDLE)
    {
        if (TIMEOUT_EXPIRED(startusec, CEATA_COMMAND_TIMEOUT))
            RET_ERR(3);
        yield();
    }
    PASS_RC(mmc_dsta_check_data_success(), 3, 4);
    return 0;
}

static int ceata_init(int buswidth)
{
    uint32_t result;
    PASS_RC(mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_SWITCH) | SDCI_CMD_RES_BUSY
                           | SDCI_CMD_CMD_TYPE_AC | SDCI_CMD_RES_TYPE_R1
                           | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                             MMC_CMD_SWITCH_ACCESS_WRITE_BYTE
                           | MMC_CMD_SWITCH_INDEX(MMC_CMD_SWITCH_FIELD_HS_TIMING)
                           | MMC_CMD_SWITCH_VALUE(MMC_CMD_SWITCH_FIELD_HS_TIMING_HIGH_SPEED),
                             &result, CEATA_COMMAND_TIMEOUT), 3, 0);
    if (result & MMC_STATUS_SWITCH_ERROR)
        RET_ERR(1);
    if (buswidth > 1)
    {
        int setting;
        if (buswidth == 4)
            setting = MMC_CMD_SWITCH_FIELD_BUS_WIDTH_4BIT;
        else if (buswidth == 8)
            setting = MMC_CMD_SWITCH_FIELD_BUS_WIDTH_8BIT;
        else
            setting = MMC_CMD_SWITCH_FIELD_BUS_WIDTH_1BIT;
        PASS_RC(mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_SWITCH) | SDCI_CMD_RES_BUSY
                               | SDCI_CMD_CMD_TYPE_AC | SDCI_CMD_RES_TYPE_R1
                               | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                                 MMC_CMD_SWITCH_ACCESS_WRITE_BYTE
                               | MMC_CMD_SWITCH_INDEX(MMC_CMD_SWITCH_FIELD_BUS_WIDTH)
                               | MMC_CMD_SWITCH_VALUE(setting),
                                 &result, CEATA_COMMAND_TIMEOUT), 3, 2);
        if (result & MMC_STATUS_SWITCH_ERROR)
            RET_ERR(3);
        if (buswidth == 4)
            SDCI_CTRL = (SDCI_CTRL & ~SDCI_CTRL_BUS_WIDTH_MASK) | SDCI_CTRL_BUS_WIDTH_4BIT;
        else if (buswidth == 8)
            SDCI_CTRL = (SDCI_CTRL & ~SDCI_CTRL_BUS_WIDTH_MASK) | SDCI_CTRL_BUS_WIDTH_8BIT;
    }
    PASS_RC(ceata_soft_reset(), 3, 4);
    PASS_RC(ceata_read_multiple_register(0, ceata_taskfile, 0x10), 3, 5);
    if (ceata_taskfile[0xc] != 0xce || ceata_taskfile[0xd] != 0xaa)
        RET_ERR(6);
    PASS_RC(mmc_fastio_write(6, 0), 3, 7);
    return 0;
}

static int ceata_check_error(void)
{
    uint32_t status, error;
    PASS_RC(mmc_fastio_read(0xf, &status), 2, 0);
    if (status & 1)
    {
        PASS_RC(mmc_fastio_read(0x9, &error), 2, 1);
        RET_ERR((error << 2) | 2);
    }
    return 0;
}

static int ceata_wait_idle(void)
{
    long startusec = USEC_TIMER;
    while (true)
    {
        uint32_t status;
        PASS_RC(mmc_fastio_read(0xf, &status), 1, 0);
        if (!(status & 0x88))
            return 0;
        if (TIMEOUT_EXPIRED(startusec, CEATA_DAT_NONBUSY_TIMEOUT))
            RET_ERR(1);
        sleep(HZ / 20);
    }
}

static int ceata_cancel_command(void)
{
    *((uint32_t volatile*)0x3cf00200) = 0x9000e;
    udelay(1);
    *((uint32_t volatile*)0x3cf00200) = 0x9000f;
    udelay(1);
    *((uint32_t volatile*)0x3cf00200) = 0x90003;
    udelay(1);
    PASS_RC(mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_STOP_TRANSMISSION)
                           | SDCI_CMD_CMD_TYPE_AC | SDCI_CMD_RES_TYPE_R1 | SDCI_CMD_RES_BUSY
                           | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                             0, NULL, CEATA_COMMAND_TIMEOUT), 1, 0);
    PASS_RC(ceata_wait_idle(), 1, 1);
    return 0;
}

static int ceata_rw_multiple_block(bool write, void* buf, uint32_t count, long timeout)
{
    mmc_discard_irq();
    uint32_t responsetype;
    uint32_t cmdtype;
    uint32_t direction;
    if (write)
    {
        cmdtype = SDCI_CMD_CMD_TYPE_ADTC | SDCI_CMD_CMD_RD_WR;
        responsetype = SDCI_CMD_RES_TYPE_R1 | SDCI_CMD_RES_BUSY;
        direction = MMC_CMD_CEATA_RW_MULTIPLE_BLOCK_DIRECTION_WRITE;
    }
    else
    {
        cmdtype = SDCI_CMD_CMD_TYPE_ADTC;
        responsetype = SDCI_CMD_RES_TYPE_R1;
        direction = MMC_CMD_CEATA_RW_MULTIPLE_BLOCK_DIRECTION_READ;
    }
    SDCI_DMASIZE = 0x200;
    SDCI_DMAADDR = buf;
    SDCI_DMACOUNT = count;
    SDCI_DCTRL = SDCI_DCTRL_TXFIFORST | SDCI_DCTRL_RXFIFORST;
    commit_discard_dcache();
    PASS_RC(mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_CEATA_RW_MULTIPLE_BLOCK)
                           | SDCI_CMD_CMD_TYPE_ADTC | cmdtype | responsetype
                           | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                             direction | MMC_CMD_CEATA_RW_MULTIPLE_BLOCK_COUNT(count),
                             NULL, CEATA_COMMAND_TIMEOUT), 3, 0);
    if (write)
        SDCI_DCTRL = SDCI_DCTRL_TRCONT_TX;
    if (semaphore_wait(&mmc_wakeup, timeout) == OBJ_WAIT_TIMEDOUT)
    {
        PASS_RC(ceata_cancel_command(), 3, 1);
        RET_ERR(2);
    }
    PASS_RC(mmc_dsta_check_data_success(), 3, 3);
    if (semaphore_wait(&mmc_comp_wakeup, timeout) == OBJ_WAIT_TIMEDOUT)
    {
        PASS_RC(ceata_cancel_command(), 3, 4);
        RET_ERR(5);
    }
    PASS_RC(ceata_check_error(), 3, 6);
    return 0;
}

static int ata_identify(uint16_t* buf)
{
    int i;
    if (ceata)
    {
        memset(ceata_taskfile, 0, 16);
        ceata_taskfile[0xf] = CMD_IDENTIFY;
        PASS_RC(ceata_wait_idle(), 2, 0);
        PASS_RC(ceata_write_multiple_register(0, ceata_taskfile, 16), 2, 1);
        PASS_RC(ceata_rw_multiple_block(false, buf, 1, CEATA_COMMAND_TIMEOUT * HZ / 1000000), 2, 2);
    }
    else
    {
        PASS_RC(ata_wait_for_not_bsy(10000000), 1, 0);
        ata_write_cbr(&ATA_PIO_DVR, 0);
        ata_write_cbr(&ATA_PIO_CSD, CMD_IDENTIFY);
        PASS_RC(ata_wait_for_start_of_transfer(10000000), 1, 1);
        for (i = 0; i < ATA_IDENTIFY_WORDS; i++)
            buf[i] = ata_read_cbr(&ATA_PIO_DTR);
    }
    return 0;
}

static void ata_set_active(void)
{
    ata_last_activity_value = current_tick;
}

bool ata_disk_is_active(void)
{
    return ata_powered;
}

static int ata_set_feature(uint32_t feature, uint32_t param)
{
    if (ceata)
    {
        memset(ceata_taskfile, 0, 16);
        ceata_taskfile[0x1] = feature;
        ceata_taskfile[0x2] = param;
        ceata_taskfile[0xf] = CMD_SET_FEATURES;
        PASS_RC(ceata_wait_idle(), 2, 0);
        PASS_RC(ceata_write_multiple_register(0, ceata_taskfile, 16), 2, 1);
        PASS_RC(ceata_wait_idle(), 2, 2);
    }
    else
    {
        PASS_RC(ata_wait_for_rdy(2000000), 2, 0);
        ata_write_cbr(&ATA_PIO_DVR, 0);
        ata_write_cbr(&ATA_PIO_FED, feature);
        ata_write_cbr(&ATA_PIO_SCR, param);
        ata_write_cbr(&ATA_PIO_CSD, CMD_SET_FEATURES);
        PASS_RC(ata_wait_for_rdy(2000000), 2, 1);
    }
    return 0;
}

#ifdef HAVE_ATA_DMA
static int udmatimes[ATA_MAX_UDMA + 1] = {
        0x4071152,
        0x2050d52,
#if ATA_MAX_UDMA >= 2
        0x2030a52,
#endif
#if ATA_MAX_UDMA >= 3
        0x1020a52,
#endif
#if ATA_MAX_UDMA >= 4
        0x1010a52,
#endif
};
static int mwdmatimes[ATA_MAX_MWDMA + 1] = {
        0x1c175,
        0x7083,
        0x5072,
};

static int ata_get_best_mode(unsigned short identword, int max, int modetype)
{
    unsigned short testbit = BIT_N(max);

    while (1) {
        if (identword & testbit)
            return max | modetype;
        testbit >>= 1;
        if (!testbit)
            return 0;
        max--;
    }
}
#endif

/*
 * ATA_UDMA_TIME register is documented on s3c6400 datasheet, information
 * included in s5l8700 datasheet is wrong or not valid for s5l8702.
 *
 * On Classic (boosted):
 *  HClk = 108 MHz. -> T = ~9.26 ns.
 *
 * Configured values (in nanoseconds):
 *
 *  UDMA  ATA_UDMA_TIME  tACK  tRP    tSS   tDVS  tDVH  Tcyc   WR(MB/s)
 *  0     0x4071152      27.8  166.7  55.6  74.1  46.3  120.4  16.6
 *  1     0x2050d52      27.8  129.6  55.6  55.6  27.8   83.4  24
 *  2     0x2030a52      27.8  101.8  55.6  37    27.8   64.8  30.9
 *  3     0x1020a52      27.8  101.8  55.6  27.8  18.5   46.3  43.2
 *  4     0x1010a52      27.8  101.8  55.6  18.5  18.5   37    54
 *
 *  Tcyc = tDVS+tDVH
 *  WR[bytes/s] = 1/Tcyc[s] * 2[bytes]
 */
static int ata_power_up(void)
{
    ata_set_active();
    ide_power_enable(true);
    long spinup_start = current_tick;
    if (ceata) {
        ata_lba48 = true;
        ata_dma = true;
        dma_mode = 0xff; /* Canary */
        PCON(8) = 0x33333333;
        PCON(9) = 0x00000033;
        PCON(11) |= 0xf;
        *((uint32_t volatile*)0x38a00000) = 0;
        *((uint32_t volatile*)0x38700000) = 0;
        PWRCON(0) &= ~(1 << 9);
        SDCI_RESET = 0xa5;
        sleep(HZ / 100);
        *((uint32_t volatile*)0x3cf00380) = 0;
        *((uint32_t volatile*)0x3cf0010c) = 0xff;
        SDCI_CTRL = SDCI_CTRL_SDCIEN | SDCI_CTRL_CLK_SEL_SDCLK
                  | SDCI_CTRL_BIT_8 | SDCI_CTRL_BIT_14;
        SDCI_CDIV = SDCI_CDIV_CLKDIV(260);
        *((uint32_t volatile*)0x3cf00200) = 0xb000f;
        SDCI_IRQ_MASK = SDCI_IRQ_MASK_MASK_DAT_DONE_INT | SDCI_IRQ_MASK_MASK_IOCARD_IRQ_INT;
        PASS_RC(mmc_init(), 3, 0);
        SDCI_CDIV = SDCI_CDIV_CLKDIV(4);
        sleep(HZ / 100);
        PASS_RC(ceata_init(8), 3, 1);
        PASS_RC(ata_identify(identify_info), 3, 2);
    } else {
        PCON(7) = 0x44444444;
        PCON(8) = 0x44444444;
        PCON(9) = 0x44444444;
        PCON(10) = (PCON(10) & ~0xffff) | 0x4444;
        PWRCON(0) &= ~(1 << 5);
        ATA_CFG = BIT(0);
        sleep(HZ / 100);
        ATA_CFG = 0;
        sleep(HZ / 100);
        ATA_SWRST = BIT(0);
        sleep(HZ / 100);
        ATA_SWRST = 0;
        sleep(HZ / 10);
        ATA_CONTROL = BIT(0);
        sleep(HZ / 5);
        ATA_PIO_TIME = 0x191f7;
        ATA_PIO_LHR = 0;
        ATA_CFG = BIT(6);
        while (!(ATA_PIO_READY & BIT(1))) yield();

        PASS_RC(ata_identify(identify_info), 3, 3);

        uint32_t piotime = 0x11f3; /* PIO0-2? */
        if (identify_info[53] & BIT(1)) /* Word 64..70 valid */
        {
            if (identify_info[64] & BIT(1))
                piotime = 0x2072; /* PIO mode 4 */
            else if (identify_info[64] & BIT(0))
                piotime = 0x7083; /* PIO mode 3 */
        }
        ATA_PIO_TIME = piotime;

        uint32_t param = 0;
        ata_dma_flags = 0;
#ifdef HAVE_ATA_DMA
        if ((identify_info[53] & BIT(2)) && (identify_info[88] & BITRANGE(0, 4))) /* Any UDMA */
        {
            int max_udma = ATA_MAX_UDMA;
#if ATA_MAX_UDMA > 2
            if (!(identify_info[93] & BIT(13)))
                max_udma = 2;
#endif
            param = ata_get_best_mode(identify_info[88], max_udma, 0x40);
            ATA_UDMA_TIME = udmatimes[param & 0xf];
            ata_dma_flags = BIT(2) | BIT(3) | BIT(9) | BIT(10);
        }
        if (!param && identify_info[63] & BITRANGE(0, 2)) /* Fall back to any MWDMA */
        {
            param = ata_get_best_mode(identify_info[63], ATA_MAX_MWDMA, 0x20);
            ATA_MDMA_TIME = mwdmatimes[param & 0xf];
            ata_dma_flags = BIT(3) | BIT(10);
        }
#endif /* HAVE_ATA_DMA */
        ata_dma = param ? true : false;
        dma_mode = param;
        PASS_RC(ata_set_feature(0x03, param), 3, 4); /* Transfer mode */

        /* SET_FEATURE only supported on PATA, not CE-ATA */
        if (identify_info[82] & BIT(5))
            PASS_RC(ata_set_feature(0x02, 0), 3, 5); /* Enable volatile write cache */
        if (identify_info[82] & BIT(6))
            PASS_RC(ata_set_feature(0xaa, 0), 3, 6); /* Enable read lookahead */
        if (identify_info[83] & BIT(3))
            PASS_RC(ata_set_feature(0x05, 0x80), 3, 7); /* Enable lowest power mode w/o standby */
        if (identify_info[83] & BIT(9))
            PASS_RC(ata_set_feature(0x42, 0x80), 3, 8); /* Enable lowest noise mode */

        PASS_RC(ata_identify(identify_info), 3, 9); /* Finally, re-read identify info */
    }

    spinup_time = current_tick - spinup_start;

    ata_total_sectors = (identify_info[61] << 16) | identify_info[60];
    if ( identify_info[83] & BIT(10) && ata_total_sectors == 0x0FFFFFFF)
    {
        ata_total_sectors = ((uint64_t)identify_info[103] << 48) |
                ((uint64_t)identify_info[102] << 32) |
                ((uint64_t)identify_info[101] << 16) |
                identify_info[100];
        ata_lba48 = true;
    } else {
        ata_lba48 = false;
    }

    ata_powered = true;
    ata_set_active();
    return 0;
}

static void ata_power_down(void)
{
    if (!ata_powered)
        return;
    PCON(7) = 0;
    PCON(8) = 0;
    PCON(9) = 0;
    PCON(10) &= ~0xffff;
    PCON(11) &= ~0xf;
    ide_power_enable(false);
    ata_powered = false;
}

static int ata_rw_chunk_internal(uint64_t sector, uint32_t cnt, void* buffer, bool write)
{
    if (ceata)
    {
        memset(ceata_taskfile, 0, 16);
        ceata_taskfile[0x2] = cnt >> 8;
        ceata_taskfile[0x3] = sector >> 24;
        ceata_taskfile[0x4] = sector >> 32;
        ceata_taskfile[0x5] = sector >> 40;
        ceata_taskfile[0xa] = cnt;
        ceata_taskfile[0xb] = sector;
        ceata_taskfile[0xc] = sector >> 8;
        ceata_taskfile[0xd] = sector >> 16;
        ceata_taskfile[0xf] = write ? CMD_WRITE_DMA_EXT : CMD_READ_DMA_EXT;
        PASS_RC(ceata_wait_idle(), 2, 0);
        PASS_RC(ceata_write_multiple_register(0, ceata_taskfile, 16), 2, 1);
        PASS_RC(ceata_rw_multiple_block(write, buffer, cnt, CEATA_COMMAND_TIMEOUT * HZ / 1000000), 2, 2);
    }
    else
    {
        PASS_RC(ata_wait_for_rdy(100000), 2, 0);
        ata_write_cbr(&ATA_PIO_DVR, 0);
        if (ata_lba48)
        {
            ata_write_cbr(&ATA_PIO_SCR, (cnt >> 8) & 0xff);
            ata_write_cbr(&ATA_PIO_SCR, (cnt) & 0xff);
            ata_write_cbr(&ATA_PIO_LHR, (sector >> 40) & 0xff);
            ata_write_cbr(&ATA_PIO_LMR, (sector >> 32) & 0xff);
            ata_write_cbr(&ATA_PIO_LLR, (sector >> 24) & 0xff);
            ata_write_cbr(&ATA_PIO_LHR, (sector >> 16) & 0xff);
            ata_write_cbr(&ATA_PIO_LMR, (sector >> 8) & 0xff);
            ata_write_cbr(&ATA_PIO_LLR, (sector) & 0xff);
            ata_write_cbr(&ATA_PIO_DVR, BIT(6));
            if (write)
                ata_write_cbr(&ATA_PIO_CSD, ata_dma ? CMD_WRITE_DMA_EXT : CMD_WRITE_MULTIPLE_EXT);
            else
                ata_write_cbr(&ATA_PIO_CSD, ata_dma ? CMD_READ_DMA_EXT : CMD_READ_MULTIPLE_EXT);
        }
        else
        {
            ata_write_cbr(&ATA_PIO_SCR, (cnt) & 0xff);
            ata_write_cbr(&ATA_PIO_LHR, (sector >> 16) & 0xff);
            ata_write_cbr(&ATA_PIO_LMR, (sector >> 8) & 0xff);
            ata_write_cbr(&ATA_PIO_LLR, (sector) & 0xff);
            ata_write_cbr(&ATA_PIO_DVR, BIT(6) | ((sector >> 24) & 0xf)); /* LBA28, mask off upper 4 bits of 32-bit sector address */
            if (write)
                ata_write_cbr(&ATA_PIO_CSD, ata_dma ? CMD_WRITE_DMA : CMD_WRITE_SECTORS);
            else
                ata_write_cbr(&ATA_PIO_CSD, ata_dma ? CMD_READ_DMA : CMD_READ_MULTIPLE);
        }
#ifdef HAVE_ATA_DMA
        if (ata_dma)
        {
            PASS_RC(ata_wait_for_start_of_transfer(500000), 2, 1);
            if (write)
            {
                ATA_SBUF_START = buffer;
                ATA_SBUF_SIZE = log_sector_size * cnt;
                ATA_CFG |= BIT(4);
            }
            else
            {
                ATA_TBUF_START = buffer;
                ATA_TBUF_SIZE = log_sector_size * cnt;
                ATA_CFG &= ~BIT(4);
            }
            ATA_XFR_NUM = log_sector_size * cnt - 1;
            ATA_CFG |= ata_dma_flags;
            ATA_CFG &= ~(BIT(7) | BIT(8));
            semaphore_wait(&ata_wakeup, 0);
            ATA_IRQ = BITRANGE(0, 4);
            ATA_IRQ_MASK = BIT(0);
            ATA_COMMAND = BIT(0);
            if (semaphore_wait(&ata_wakeup, 500000 * HZ / 1000000) == OBJ_WAIT_TIMEDOUT)
            {
                ATA_COMMAND = BIT(1);
                ATA_CFG &= ~(BITRANGE(2, 3) | BIT(12));
                RET_ERR(2);
            }
            ATA_COMMAND = BIT(1);
            ATA_CFG &= ~(BITRANGE(2, 3) | BIT(12));
        }
        else
#endif // HAVE_ATA_DMA
        {
            while (cnt--)
            {
                uint16_t i;
                PASS_RC(ata_wait_for_start_of_transfer(500000), 2, 1);
                if (write)
                    for (i = 0; i < log_sector_size/2; i++)
                        ata_write_cbr(&ATA_PIO_DTR, ((uint16_t*)buffer)[i]);
                else
                    for (i = 0; i < log_sector_size/2; i++)
                        ((uint16_t*)buffer)[i] = ata_read_cbr(&ATA_PIO_DTR);
                buffer += log_sector_size;
            }
        }
        PASS_RC(ata_wait_for_end_of_transfer(100000), 2, 3);
    }
    return 0;
}

static int ata_rw_chunk(uint64_t sector, uint32_t cnt, void* buffer, bool write)
{
    led(true);
    int rc = ata_rw_chunk_internal(sector, cnt, buffer, write);
    led(false);
    return rc;
}

static int ata_transfer_sectors(uint64_t sector, uint32_t count, void* buffer, bool write)
{
    if (!ata_powered)
        ata_power_up();
    if (sector + count > ata_total_sectors)
        RET_ERR(0);
    ata_set_active();
    if (ata_dma && write)
        commit_dcache();
    else if (ata_dma)
        commit_discard_dcache();
    if (!ceata)
        ATA_COMMAND = BIT(1);

    while (count)
    {
        uint32_t cnt = MIN(ata_lba48 ? 65536 : 256, count);
        int rc = -1;
        rc = ata_rw_chunk(sector, cnt, buffer, write);
        if (rc && ata_error_srst)
            ata_reset();
        if (rc && ata_retries)
        {
            void* buf = buffer;
            uint64_t sect;
            for (sect = sector; sect < sector + cnt; sect++)
            {
                rc = -1;
                int tries = ata_retries;
                while (tries-- && rc)
                {
                    rc = ata_rw_chunk(sect, 1, buf, write);
                    if (rc && ata_error_srst)
                        ata_reset();
                }
                if (rc)
                    break;
                buf += log_sector_size;
            }
        }
        PASS_RC(rc, 1, 1);
        buffer += log_sector_size * cnt;
        sector += cnt;
        count -= cnt;
    }
    ata_set_active();
    return 0;
}

/* API Functions */
int ata_soft_reset(void)
{
    int rc;
    mutex_lock(&ata_mutex);
    if (!ata_powered)
        PASS_RC(ata_power_up(), 1, 0);
    ata_set_active();
    if (ceata)
    {
        rc = ceata_soft_reset();
    }
    else
    {
        ata_write_cbr(&ATA_PIO_DAD, BIT(1) | BIT(2));
        udelay(10);
        ata_write_cbr(&ATA_PIO_DAD, 0);
        rc = ata_wait_for_rdy(3000000);
    }
    ata_set_active();
    mutex_unlock(&ata_mutex);
    PASS_RC(rc, 1, 1);
    return 0;
}

int ata_hard_reset(void)
{
    mutex_lock(&ata_mutex);
    PASS_RC(ata_power_up(), 0, 0);
    ata_set_active();
    mutex_unlock(&ata_mutex);
    return 0;
}

static int ata_reset(void)
{
    int rc;
    mutex_lock(&ata_mutex);
    if (!ata_powered)
        PASS_RC(ata_power_up(), 2, 0);
    ata_set_active();
    rc = ata_soft_reset();
    if (IS_ERR(rc))
    {
        rc = ata_hard_reset();
        if (IS_ERR(rc))
        {
            rc = ERR_RC((rc << 2) | 1);
            ata_power_down();
            sleep(HZ * 3);
            int rc2 = ata_power_up();
            if (IS_ERR(rc2))
                rc = ERR_RC((rc << 2) | 2);
        }
        else rc = 1;
    }
    ata_set_active();
    mutex_unlock(&ata_mutex);
    return rc;
}

#include "ata-common.c"

#ifndef MAX_PHYS_SECTOR_SIZE
int ata_read_sectors(IF_MD(int drive,) sector_t start, int incount,
                     void* inbuf)
{
#ifdef HAVE_MULTIDRIVE
    (void)drive; /* unused for now */
#endif

    mutex_lock(&ata_mutex);
    int rc = ata_transfer_sectors(start, incount, inbuf, false);
    mutex_unlock(&ata_mutex);
    return rc;
}

int ata_write_sectors(IF_MD(int drive,) sector_t start, int count,
                      const void* outbuf)
{
#ifdef HAVE_MULTIDRIVE
    (void)drive; /* unused for now */
#endif

    mutex_lock(&ata_mutex);
    int rc = ata_transfer_sectors(start, count, (void*)((uint32_t)outbuf), true);
    mutex_unlock(&ata_mutex);
    return rc;
}
#endif /* ndef MAX_PHYS_SECTOR_SIZE */

void ata_spindown(int seconds)
{
    ata_sleep_timeout = seconds * HZ;
}

static void ata_flush_cache(void)
{
    uint8_t cmd;

    if (ceata) {
        memset(ceata_taskfile, 0, 16);
        ceata_taskfile[0xf] = CMD_FLUSH_CACHE_EXT;  /* CE-ATA only supports EXT */
        ceata_wait_idle();
        ceata_write_multiple_register(0, ceata_taskfile, 16);
        ceata_wait_idle();
    } else {
        if (!canflush) {
            return;
        } else if (ata_lba48 && identify_info[83] & BIT(13)) {
            cmd = CMD_FLUSH_CACHE_EXT; /* Flag, optional, ATA-6 and up, for use with LBA48 devices. Mandatory for CE-ATA */
        } else if (identify_info[83] & BIT(12)) {
            cmd = CMD_FLUSH_CACHE; /* Flag, mandatory, ATA-6 and up */
        } else if (identify_info[80] >= BIT(5)) {  /* Use >= instead of '&' because bits lower than the latest standard we support don't have to be set */
            cmd = CMD_FLUSH_CACHE; /* No flag, mandatory, ATA-5  (Optional for ATA-4) */
        } else {
            /* If neither command is supported then don't issue it. */
            canflush = 0;
            return;
        }

        ata_wait_for_rdy(1000000);
        ata_write_cbr(&ATA_PIO_DVR, 0);
        ata_write_cbr(&ATA_PIO_CSD, cmd);
        ata_wait_for_rdy(1000000);
    }
}

int ata_flush(void)
{
    if (ata_powered) {
        mutex_lock(&ata_mutex);
        ata_flush_cache();
        mutex_unlock(&ata_mutex);
    }
    return 0;
}

void ata_sleepnow(void)
{
    mutex_lock(&ata_mutex);

    ata_flush_cache();

    if (ata_disk_can_sleep()) {
        if (ceata) {
            memset(ceata_taskfile, 0, 16);
            ceata_taskfile[0xf] = CMD_STANDBY_IMMEDIATE;
            ceata_wait_idle();
            ceata_write_multiple_register(0, ceata_taskfile, 16);
            ceata_wait_idle();
            sleep(HZ);
            PWRCON(0) |= (1 << 9);
        } else {
            ata_wait_for_rdy(1000000);
            ata_write_cbr(&ATA_PIO_DVR, 0);
            ata_write_cbr(&ATA_PIO_CSD, CMD_STANDBY_IMMEDIATE);
            ata_wait_for_rdy(1000000);
            sleep(HZ / 30);
            ATA_CONTROL = 0;
            while (!(ATA_CONTROL & BIT(1)))
                yield();
            PWRCON(0) |= (1 << 5);
        }
    }

    if (ata_disk_can_sleep() || canflush)
        ata_power_down(); // XXX add a powerdown delay similar to main ATA driver?

    mutex_unlock(&ata_mutex);
}

void ata_spin(void)
{
    ata_set_active();
}

#ifdef STORAGE_GET_INFO
void ata_get_info(IF_MD(int drive,) struct storage_info *info)
{
    info->sector_size = log_sector_size;
    info->num_sectors = ata_total_sectors;
    info->vendor = "Apple";
    info->product = "iPod Classic";
    info->revision = "1.0";
}
#endif

long ata_last_disk_activity(void)
{
    return ata_last_activity_value;
}

int ata_init(void)
{
    mutex_init(&ata_mutex);
    semaphore_init(&ata_wakeup, 1, 0);
    semaphore_init(&mmc_wakeup, 1, 0);
    semaphore_init(&mmc_comp_wakeup, 1, 0);
    ceata = PDAT(11) & BIT(1);
    ata_powered = false;
    ata_total_sectors = 0;

    /* get identify_info */
    mutex_lock(&ata_mutex);
    int rc = ata_power_up(); /* Include identify() call */
    mutex_unlock(&ata_mutex);
    if (IS_ERR(rc))
        return rc;

    /* Logical sector size */
    if ((identify_info[106] & 0xd000) == 0x5000) /* B14, B12 */
        log_sector_size = (identify_info[117] | (identify_info[118] << 16)) * 2;
    else
        log_sector_size = 512;

#ifdef MAX_PHYS_SECTOR_SIZE
    rc = ata_get_phys_sector_mult();
    if (IS_ERR(rc))
        return rc;
#endif

    return 0;
}

#ifdef HAVE_ATA_SMART
static int ata_smart(uint16_t* buf)
{
    if (!ata_powered) PASS_RC(ata_power_up(), 3, 0);
    if (ceata)
    {
        memset(ceata_taskfile, 0, 16);
        ceata_taskfile[0xc] = 0x4f;
        ceata_taskfile[0xd] = 0xc2;
        ceata_taskfile[0xe] = BIT(6);
        ceata_taskfile[0xf] = CMD_SMART;
        PASS_RC(ceata_wait_idle(), 3, 1);
        if (((uint8_t*)identify_info)[54] != 'A')  /* Model != aAmsung */
        {
            ceata_taskfile[0x9] = 0xd8; /* SMART enable operations */
            PASS_RC(ceata_write_multiple_register(0, ceata_taskfile, 16), 3, 2);
            PASS_RC(ceata_check_error(), 3, 3);
        }
        ceata_taskfile[0x9] = 0xd0; /* SMART read data */
        PASS_RC(ceata_write_multiple_register(0, ceata_taskfile, 16), 3, 4);
        PASS_RC(ceata_rw_multiple_block(false, buf, 1, CEATA_COMMAND_TIMEOUT * HZ / 1000000), 3, 5);
    }
    else
    {
        int i;
        PASS_RC(ata_wait_for_not_bsy(10000000), 3, 6);
        ata_write_cbr(&ATA_PIO_FED, 0xd0);
        ata_write_cbr(&ATA_PIO_LMR, 0x4f);
        ata_write_cbr(&ATA_PIO_LHR, 0xc2);
        ata_write_cbr(&ATA_PIO_DVR, BIT(6));
        ata_write_cbr(&ATA_PIO_CSD, CMD_SMART);
        PASS_RC(ata_wait_for_start_of_transfer(10000000), 3, 7);
        for (i = 0; i < 0x100; i++)
            buf[i] = ata_read_cbr(&ATA_PIO_DTR);
    }
    ata_set_active();
    return 0;
}

int ata_read_smart(struct ata_smart_values* smart_data)
{
    mutex_lock(&ata_mutex);
    int rc = ata_smart((uint16_t*)smart_data);
    mutex_unlock(&ata_mutex);
    return rc;
}
#endif /* HAVE_ATA_SMART */

#ifdef CONFIG_STORAGE_MULTI
static int ata_num_drives(int first_drive)
{
    /* We don't care which logical drive number(s) we have been assigned */
    (void)first_drive;

    return 1;
}
#endif

unsigned short* ata_get_identify(void)
{
    return identify_info;
}

int ata_spinup_time(void)
{
    return spinup_time;
}

#ifdef HAVE_ATA_DMA
int ata_get_dma_mode(void)
{
    return dma_mode;
}
#endif

void INT_ATA(void)
{
    uint32_t ata_irq = ATA_IRQ;
    ATA_IRQ = ata_irq;
    if (ata_irq & ATA_IRQ_MASK)
        semaphore_release(&ata_wakeup);
    ATA_IRQ_MASK = 0;
}

void INT_MMC(void)
{
    uint32_t irq = SDCI_IRQ;
    if (irq & SDCI_IRQ_DAT_DONE_INT)
        semaphore_release(&mmc_wakeup);
    if (irq & SDCI_IRQ_IOCARD_IRQ_INT)
        semaphore_release(&mmc_comp_wakeup);
    SDCI_IRQ = irq;
}

int ata_event(long id, intptr_t data)
{
    int rc = 0;

    /* GCC does a lousy job culling unreachable cases in the default handler
       if statements are in a switch statement, so we'll do it this way. Only
       the first case is frequently hit anyway. */
    if (LIKELY(id == Q_STORAGE_TICK))
    {
        if (!ata_powered ||
            TIME_BEFORE(current_tick, ata_last_activity_value + ata_sleep_timeout))
        {
            STG_EVENT_ASSERT_ACTIVE(STORAGE_ATA);
        }
    }
    else if (id == Q_STORAGE_SLEEPNOW)
    {
        ata_sleepnow();
    }
    else if (id == Q_STORAGE_SLEEP)
    {
        ata_last_activity_value = current_tick - ata_sleep_timeout + HZ / 5;
    }
    else if (id == SYS_USB_CONNECTED)
    {
        STG_EVENT_ASSERT_ACTIVE(STORAGE_ATA);
    }
    else
    {
        rc = storage_event_default_handler(id, data, ata_last_activity_value,
                                           STORAGE_ATA);
    }

    return rc;
}
