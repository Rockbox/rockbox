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
#include "thread.h"
#include "disk.h"
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
#include "ata_idle_notify.h"


#ifndef ATA_RETRIES
#define ATA_RETRIES 3
#endif


#define CEATA_POWERUP_TIMEOUT 20000000
#define CEATA_COMMAND_TIMEOUT 1000000
#define CEATA_DAT_NONBUSY_TIMEOUT 5000000
#define CEATA_MMC_RCA 1


/** static, private data **/ 
static uint8_t ceata_taskfile[16] __attribute__((aligned(16)));
uint16_t ata_identify_data[0x100] __attribute__((aligned(16)));
bool ceata;
bool ata_lba48;
bool ata_dma;
uint64_t ata_total_sectors;
struct mutex ata_mutex;
static struct semaphore ata_wakeup;
static uint32_t ata_dma_flags;
static long ata_last_activity_value = -1;
static long ata_sleep_timeout = 20 * HZ;
static uint32_t ata_stack[(DEFAULT_STACK_SIZE + 0x400) / 4];
static bool ata_powered;
static const int ata_retries = ATA_RETRIES;
static const bool ata_error_srst = true;
static struct semaphore mmc_wakeup;
static struct semaphore mmc_comp_wakeup;
static int spinup_time = 0;
static int dma_mode = 0;


#ifdef ATA_HAVE_BBT
char ata_bbt_buf[ATA_BBT_PAGES * 64];
uint16_t (*ata_bbt)[0x20];
uint64_t ata_virtual_sectors;
uint32_t ata_last_offset;
uint64_t ata_last_phys;

int ata_rw_sectors_internal(uint64_t sector, uint32_t count,
                            void* buffer, bool write);

int ata_bbt_read_sectors(uint32_t sector, uint32_t count, void* buffer)
{
    if (ata_last_phys != sector - 1 && ata_last_phys > sector - 64) ata_soft_reset();
    int rc = ata_rw_sectors_internal(sector, count, buffer, false);
    if (rc) rc = ata_rw_sectors_internal(sector, count, buffer, false);
    ata_last_phys = sector + count - 1;
    ata_last_offset = 0;
    if (IS_ERR(rc))
        panicf("ATA: Error %08X while reading BBT (sector %d, count %d)\n",
               (unsigned int)rc, (unsigned int)sector, (unsigned int)count);
    return rc;
}
#endif


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
        if (!(csd & BIT(7))) return 0;
        if (TIMEOUT_EXPIRED(startusec, timeout)) RET_ERR(0);
    }
}

static int ata_wait_for_rdy(long timeout)
{
    long startusec = USEC_TIMER;
    PASS_RC(ata_wait_for_not_bsy(timeout), 1, 0);
    while (true)
    {
        uint8_t dad = ata_read_cbr(&ATA_PIO_DAD);
        if (dad & BIT(6)) return 0;
        if (TIMEOUT_EXPIRED(startusec, timeout)) RET_ERR(1);
    }
}

static int ata_wait_for_start_of_transfer(long timeout)
{
    long startusec = USEC_TIMER;
    PASS_RC(ata_wait_for_not_bsy(timeout), 2, 0);
    while (true)
    {
        uint8_t dad = ata_read_cbr(&ATA_PIO_DAD);
        if (dad & BIT(0)) RET_ERR(1);
        if ((dad & (BIT(7) | BIT(3))) == BIT(3)) return 0;
        if (TIMEOUT_EXPIRED(startusec, timeout)) RET_ERR(2);
    }
}

static int ata_wait_for_end_of_transfer(long timeout)
{
    PASS_RC(ata_wait_for_not_bsy(timeout), 2, 0);
    uint8_t dad = ata_read_cbr(&ATA_PIO_DAD);
    if (dad & BIT(0)) RET_ERR(1);
    if ((dad & (BIT(3) | BITRANGE(5, 7))) == BIT(6)) return 0;
    RET_ERR(2);
}    

int mmc_dsta_check_command_success(bool disable_crc)
{
    int rc = 0;
    uint32_t dsta = SDCI_DSTA;
    if (dsta & SDCI_DSTA_RESTOUTE) rc |= 1; 
    if (dsta & SDCI_DSTA_RESENDE) rc |= 2;
    if (dsta & SDCI_DSTA_RESINDE) rc |= 4;
    if (!disable_crc)
        if (dsta & SDCI_DSTA_RESCRCE)
            rc |= 8;
    if (rc) RET_ERR(rc);
    return 0;
}

bool mmc_send_command(uint32_t cmd, uint32_t arg, uint32_t* result, int timeout)
{
    long starttime = USEC_TIMER;
    while ((SDCI_STATE & SDCI_STATE_CMD_STATE_MASK) != SDCI_STATE_CMD_STATE_CMD_IDLE)
    {
        if (TIMEOUT_EXPIRED(starttime, timeout)) RET_ERR(0);
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
    if (!(SDCI_DSTA & SDCI_DSTA_CMDRDY)) RET_ERR(1);
    SDCI_CMD = cmd | SDCI_CMD_CMDSTR;
    long sleepbase = USEC_TIMER;
    while (TIMEOUT_EXPIRED(sleepbase, 1000)) yield();
    while (!(SDCI_DSTA & SDCI_DSTA_CMDEND))
    {
        if (TIMEOUT_EXPIRED(starttime, timeout)) RET_ERR(2);
        yield();
    }
    if ((cmd & SDCI_CMD_RES_TYPE_MASK) != SDCI_CMD_RES_TYPE_NONE)
    {
        while (!(SDCI_DSTA & SDCI_DSTA_RESEND))
        {
            if (TIMEOUT_EXPIRED(starttime, timeout)) RET_ERR(3);
            yield();
        }
        if (cmd & SDCI_CMD_RES_BUSY)
            while (SDCI_DSTA & SDCI_DSTA_DAT_BUSY)
            {
                if (TIMEOUT_EXPIRED(starttime, CEATA_DAT_NONBUSY_TIMEOUT)) RET_ERR(4);
                yield();
            }
    }
    bool nocrc = (cmd & SDCI_CMD_RES_SIZE_MASK) == SDCI_CMD_RES_SIZE_136;
    PASS_RC(mmc_dsta_check_command_success(nocrc), 3, 5);
    if (result) *result = SDCI_RESP0;
    return 0;
}

int mmc_get_card_status(uint32_t* result)
{
    return mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_SEND_STATUS)
                          | SDCI_CMD_CMD_TYPE_AC | SDCI_CMD_RES_TYPE_R1
                          | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                            MMC_CMD_SEND_STATUS_RCA(CEATA_MMC_RCA), result, CEATA_COMMAND_TIMEOUT);
}

int mmc_init(void)
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
        if (TIMEOUT_EXPIRED(startusec, CEATA_POWERUP_TIMEOUT)) RET_ERR(1);
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
    if ((result & MMC_STATUS_CURRENT_STATE_MASK) != MMC_STATUS_CURRENT_STATE_TRAN) RET_ERR(7);
    return 0;
}

int mmc_fastio_write(uint32_t addr, uint32_t data)
{
    return mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_FAST_IO)
                          | SDCI_CMD_CMD_TYPE_AC | SDCI_CMD_RES_TYPE_R4
                          | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                            MMC_CMD_FAST_IO_RCA(CEATA_MMC_RCA) | MMC_CMD_FAST_IO_DIRECTION_WRITE
                          | MMC_CMD_FAST_IO_ADDRESS(addr) | MMC_CMD_FAST_IO_DATA(data),
                            NULL, CEATA_COMMAND_TIMEOUT);
}

int mmc_fastio_read(uint32_t addr, uint32_t* data)
{
    return mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_FAST_IO)
                          | SDCI_CMD_CMD_TYPE_AC | SDCI_CMD_RES_TYPE_R4
                          | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                            MMC_CMD_FAST_IO_RCA(CEATA_MMC_RCA) | MMC_CMD_FAST_IO_DIRECTION_READ
                          | MMC_CMD_FAST_IO_ADDRESS(addr), data, CEATA_COMMAND_TIMEOUT);
}

int ceata_soft_reset(void)
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
        if (TIMEOUT_EXPIRED(startusec, CEATA_POWERUP_TIMEOUT)) RET_ERR(3);
        sleep(HZ / 100);
    }
    while (status & 0x80);
    return 0;
}

int mmc_dsta_check_data_success(void)
{
    int rc = 0;
    uint32_t dsta = SDCI_DSTA;
    if (dsta & (SDCI_DSTA_WR_DATCRCE | SDCI_DSTA_RD_DATCRCE))
    {
        if (dsta & SDCI_DSTA_WR_DATCRCE) rc |= 1;
        if (dsta & SDCI_DSTA_RD_DATCRCE) rc |= 2;
        if ((dsta & SDCI_DSTA_WR_CRC_STATUS_MASK) == SDCI_DSTA_WR_CRC_STATUS_TXERR) rc |= 4;
        else if ((dsta & SDCI_DSTA_WR_CRC_STATUS_MASK) == SDCI_DSTA_WR_CRC_STATUS_CARDERR) rc |= 8;
    }
    if (dsta & (SDCI_DSTA_RD_DATENDE0 | SDCI_DSTA_RD_DATENDE1 | SDCI_DSTA_RD_DATENDE2
              | SDCI_DSTA_RD_DATENDE3 | SDCI_DSTA_RD_DATENDE4 | SDCI_DSTA_RD_DATENDE5
              | SDCI_DSTA_RD_DATENDE6 | SDCI_DSTA_RD_DATENDE7))
        rc |= 16;
    if (rc) RET_ERR(rc);
    return 0;
}

void mmc_discard_irq(void)
{
    SDCI_IRQ = SDCI_IRQ_DAT_DONE_INT | SDCI_IRQ_MASK_MASK_IOCARD_IRQ_INT
             | SDCI_IRQ_MASK_MASK_READ_WAIT_INT;
    semaphore_wait(&mmc_wakeup, 0);
}

int ceata_read_multiple_register(uint32_t addr, void* dest, uint32_t size)
{
    if (size > 0x10) RET_ERR(0);
    mmc_discard_irq();
    SDCI_DMASIZE = size;
    SDCI_DMACOUNT = 1;
    SDCI_DMAADDR = dest;
    SDCI_DCTRL = SDCI_DCTRL_TXFIFORST | SDCI_DCTRL_RXFIFORST;
    invalidate_dcache();
    PASS_RC(mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_CEATA_RW_MULTIPLE_REG)
                           | SDCI_CMD_CMD_TYPE_ADTC | SDCI_CMD_RES_TYPE_R1
                           | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                             MMC_CMD_CEATA_RW_MULTIPLE_REG_DIRECTION_READ
                           | MMC_CMD_CEATA_RW_MULTIPLE_REG_ADDRESS(addr & 0xfc)
                           | MMC_CMD_CEATA_RW_MULTIPLE_REG_COUNT(size & 0xfc),
                             NULL, CEATA_COMMAND_TIMEOUT), 2, 1);
    if (semaphore_wait(&mmc_wakeup, CEATA_COMMAND_TIMEOUT * HZ / 1000000)
        == OBJ_WAIT_TIMEDOUT) RET_ERR(2);
    PASS_RC(mmc_dsta_check_data_success(), 2, 3);
    return 0;
}

int ceata_write_multiple_register(uint32_t addr, void* dest, uint32_t size)
{
    uint32_t i;
    if (size > 0x10) RET_ERR(0);
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
    for (i = 0; i < size / 4; i++) SDCI_DATA = ((uint32_t*)dest)[i];
    long startusec = USEC_TIMER;
    if (semaphore_wait(&mmc_wakeup, CEATA_COMMAND_TIMEOUT * HZ / 1000000)
        == OBJ_WAIT_TIMEDOUT) RET_ERR(2);
    while ((SDCI_STATE & SDCI_STATE_DAT_STATE_MASK) != SDCI_STATE_DAT_STATE_IDLE)
    {
        if (TIMEOUT_EXPIRED(startusec, CEATA_COMMAND_TIMEOUT)) RET_ERR(3);
        yield();
    }
    PASS_RC(mmc_dsta_check_data_success(), 3, 4);
    return 0;
}

int ceata_init(int buswidth)
{
    uint32_t result;
    PASS_RC(mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_SWITCH) | SDCI_CMD_RES_BUSY
                           | SDCI_CMD_CMD_TYPE_AC | SDCI_CMD_RES_TYPE_R1 
                           | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                             MMC_CMD_SWITCH_ACCESS_WRITE_BYTE
                           | MMC_CMD_SWITCH_INDEX(MMC_CMD_SWITCH_FIELD_HS_TIMING)
                           | MMC_CMD_SWITCH_VALUE(MMC_CMD_SWITCH_FIELD_HS_TIMING_HIGH_SPEED),
                             &result, CEATA_COMMAND_TIMEOUT), 3, 0);
    if (result & MMC_STATUS_SWITCH_ERROR) RET_ERR(1);
    if (buswidth > 1)
    {
        int setting;
        if (buswidth == 4) setting = MMC_CMD_SWITCH_FIELD_BUS_WIDTH_4BIT;
        else if (buswidth == 8) setting = MMC_CMD_SWITCH_FIELD_BUS_WIDTH_8BIT;
        else setting = MMC_CMD_SWITCH_FIELD_BUS_WIDTH_1BIT;
        PASS_RC(mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_SWITCH) | SDCI_CMD_RES_BUSY
                               | SDCI_CMD_CMD_TYPE_AC | SDCI_CMD_RES_TYPE_R1
                               | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                                 MMC_CMD_SWITCH_ACCESS_WRITE_BYTE
                               | MMC_CMD_SWITCH_INDEX(MMC_CMD_SWITCH_FIELD_BUS_WIDTH)
                               | MMC_CMD_SWITCH_VALUE(setting),
                                 &result, CEATA_COMMAND_TIMEOUT), 3, 2);
        if (result & MMC_STATUS_SWITCH_ERROR) RET_ERR(3);
        if (buswidth == 4)
            SDCI_CTRL = (SDCI_CTRL & ~SDCI_CTRL_BUS_WIDTH_MASK) | SDCI_CTRL_BUS_WIDTH_4BIT;
        else if (buswidth == 8)
            SDCI_CTRL = (SDCI_CTRL & ~SDCI_CTRL_BUS_WIDTH_MASK) | SDCI_CTRL_BUS_WIDTH_8BIT;
    }
    PASS_RC(ceata_soft_reset(), 3, 4);
    PASS_RC(ceata_read_multiple_register(0, ceata_taskfile, 0x10), 3, 5);
    if (ceata_taskfile[0xc] != 0xce || ceata_taskfile[0xd] != 0xaa) RET_ERR(6);
    PASS_RC(mmc_fastio_write(6, 0), 3, 7);
    return 0;
}

int ceata_check_error(void)
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

int ceata_wait_idle(void)
{
    long startusec = USEC_TIMER;
    while (true)
    {
        uint32_t status;
        PASS_RC(mmc_fastio_read(0xf, &status), 1, 0);
        if (!(status & 0x88)) return 0;
        if (TIMEOUT_EXPIRED(startusec, CEATA_DAT_NONBUSY_TIMEOUT)) RET_ERR(1);
        sleep(HZ / 20);
    }
}

int ceata_cancel_command(void)
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

int ceata_rw_multiple_block(bool write, void* buf, uint32_t count, long timeout)
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
    invalidate_dcache();
    PASS_RC(mmc_send_command(SDCI_CMD_CMD_NUM(MMC_CMD_CEATA_RW_MULTIPLE_BLOCK)
                           | SDCI_CMD_CMD_TYPE_ADTC | cmdtype | responsetype
                           | SDCI_CMD_RES_SIZE_48 | SDCI_CMD_NCR_NID_NCR,
                             direction | MMC_CMD_CEATA_RW_MULTIPLE_BLOCK_COUNT(count),
                             NULL, CEATA_COMMAND_TIMEOUT), 4, 0);
    if (write) SDCI_DCTRL = SDCI_DCTRL_TRCONT_TX;
    if (semaphore_wait(&mmc_wakeup, timeout) == OBJ_WAIT_TIMEDOUT)
    {
        PASS_RC(ceata_cancel_command(), 4, 1);
        RET_ERR(2);
    }
    PASS_RC(mmc_dsta_check_data_success(), 4, 3);
    if (semaphore_wait(&mmc_comp_wakeup, timeout) == OBJ_WAIT_TIMEDOUT)
    {
        PASS_RC(ceata_cancel_command(), 4, 4);
        RET_ERR(4);
    }
    PASS_RC(ceata_check_error(), 4, 5);
    return 0;
}

int ata_identify(uint16_t* buf)
{
    int i;
    if (ceata)
    {
        memset(ceata_taskfile, 0, 16);
        ceata_taskfile[0xf] = 0xec;
        PASS_RC(ceata_wait_idle(), 2, 0);
        PASS_RC(ceata_write_multiple_register(0, ceata_taskfile, 16), 2, 1);
        PASS_RC(ceata_rw_multiple_block(false, buf, 1, CEATA_COMMAND_TIMEOUT * HZ / 1000000), 2, 2);
    }
    else
    {
        PASS_RC(ata_wait_for_not_bsy(10000000), 1, 0);
        ata_write_cbr(&ATA_PIO_DVR, 0);
        ata_write_cbr(&ATA_PIO_CSD, 0xec);
        PASS_RC(ata_wait_for_start_of_transfer(10000000), 1, 1);
        for (i = 0; i < 0x100; i++)
        {
            uint16_t word = ata_read_cbr(&ATA_PIO_DTR);
            buf[i] = (word >> 8) | (word << 8);
        }
    }
    return 0;
}

void ata_set_active(void)
{
    ata_last_activity_value = current_tick;
}

bool ata_disk_is_active(void)
{
    return ata_powered;
}

int ata_set_feature(uint32_t feature, uint32_t param)
{
    PASS_RC(ata_wait_for_rdy(500000), 1, 0);
    ata_write_cbr(&ATA_PIO_DVR, 0);
    ata_write_cbr(&ATA_PIO_FED, 3);
    ata_write_cbr(&ATA_PIO_SCR, param);
    ata_write_cbr(&ATA_PIO_CSD, feature);
    PASS_RC(ata_wait_for_rdy(500000), 1, 1);
    return 0;
}

int ata_power_up(void)
{
    ata_set_active();
    if (ata_powered) return 0;
    ide_power_enable(true);
    long spinup_start = current_tick;
    if (ceata)
    {
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
        PASS_RC(mmc_init(), 2, 0);
        SDCI_CDIV = SDCI_CDIV_CLKDIV(4);
        sleep(HZ / 100);
        PASS_RC(ceata_init(8), 2, 1);
        PASS_RC(ata_identify(ata_identify_data), 2, 2);
        dma_mode = 0x44;
    }
    else
    {
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
        while (!(ATA_PIO_READY & BIT(1))) yield();
        PASS_RC(ata_identify(ata_identify_data), 2, 0);
        uint32_t piotime = 0x11f3;
        uint32_t mdmatime = 0x1c175;
        uint32_t udmatime = 0x5071152;
        uint32_t param = 0;
        ata_dma_flags = 0;
        ata_lba48 = ata_identify_data[83] & BIT(10) ? true : false;
        if (ata_identify_data[53] & BIT(1))
        {
            if (ata_identify_data[64] & BIT(1)) piotime = 0x2072;
            else if (ata_identify_data[64] & BIT(0)) piotime = 0x7083;
        }
        if (ata_identify_data[63] & BIT(2))
        {
            mdmatime = 0x5072;
            param = 0x22;
        }
        else if (ata_identify_data[63] & BIT(1))
        {
            mdmatime = 0x7083;
            param = 0x21;
        }
        if (ata_identify_data[63] & BITRANGE(0, 2))
        {
            ata_dma_flags = BIT(3) | BIT(10);
            param |= 0x20;
        }
        if (ata_identify_data[53] & BIT(2))
        {
            if (ata_identify_data[88] & BIT(4))
            {
                udmatime = 0x2010a52;
                param = 0x44;
            }
            else if (ata_identify_data[88] & BIT(3))
            {
                udmatime = 0x2020a52;
                param = 0x43;
            }
            else if (ata_identify_data[88] & BIT(2))
            {
                udmatime = 0x3030a52;
                param = 0x42;
            }
            else if (ata_identify_data[88] & BIT(1))
            {
                udmatime = 0x3050a52;
                param = 0x41;
            }
            if (ata_identify_data[88] & BITRANGE(0, 4))
            {
                ata_dma_flags = BIT(2) | BIT(3) | BIT(9) | BIT(10);
                param |= 0x40;
            }
        }
        ata_dma = param ? true : false;
        dma_mode = param;
        PASS_RC(ata_set_feature(0xef, param), 2, 1);
        if (ata_identify_data[82] & BIT(5)) PASS_RC(ata_set_feature(0x02, 0), 2, 2);
        if (ata_identify_data[82] & BIT(6)) PASS_RC(ata_set_feature(0x55, 0), 2, 3);
        ATA_PIO_TIME = piotime;
        ATA_MDMA_TIME = mdmatime;
        ATA_UDMA_TIME = udmatime;
    }
    spinup_time = current_tick - spinup_start;
    if (ata_lba48)
        ata_total_sectors = ata_identify_data[100]
                            | (((uint64_t)ata_identify_data[101]) << 16)
                            | (((uint64_t)ata_identify_data[102]) << 32)
                            | (((uint64_t)ata_identify_data[103]) << 48);
    else ata_total_sectors = ata_identify_data[60] | (((uint32_t)ata_identify_data[61]) << 16);
    ata_total_sectors >>= 3;
    ata_powered = true;
    ata_set_active();
    return 0;
}

void ata_power_down(void)
{
    if (!ata_powered) return;
    ata_powered = false;
    if (ceata)
    {
        memset(ceata_taskfile, 0, 16);
        ceata_taskfile[0xf] = 0xe0;
        ceata_wait_idle();
        ceata_write_multiple_register(0, ceata_taskfile, 16);
        sleep(HZ);
        PWRCON(0) |= (1 << 9);
    }
    else
    {
        ata_wait_for_rdy(1000000);
        ata_write_cbr(&ATA_PIO_DVR, 0);
        ata_write_cbr(&ATA_PIO_CSD, 0xe0);
        ata_wait_for_rdy(1000000);
        sleep(HZ / 30);
        ATA_CONTROL = 0;
        while (!(ATA_CONTROL & BIT(1))) yield();
        PWRCON(0) |= (1 << 5);
    }
    ide_power_enable(false);
}

int ata_rw_chunk_internal(uint64_t sector, uint32_t cnt, void* buffer, bool write)
{
    if (ceata)
    {
        memset(ceata_taskfile, 0, 16);
        ceata_taskfile[0x2] = cnt >> 5;
        ceata_taskfile[0x3] = sector >> 21;
        ceata_taskfile[0x4] = sector >> 29;
        ceata_taskfile[0x5] = sector >> 37;
        ceata_taskfile[0xa] = cnt << 3;
        ceata_taskfile[0xb] = sector << 3;
        ceata_taskfile[0xc] = sector >> 5;
        ceata_taskfile[0xd] = sector >> 13;
        ceata_taskfile[0xf] = write ? 0x35 : 0x25;
        PASS_RC(ceata_wait_idle(), 2, 0);
        PASS_RC(ceata_write_multiple_register(0, ceata_taskfile, 16), 2, 1);
        PASS_RC(ceata_rw_multiple_block(write, buffer, cnt << 3, CEATA_COMMAND_TIMEOUT * HZ / 1000000), 2, 2);
    }
    else
    {
        PASS_RC(ata_wait_for_rdy(100000), 2, 0);
        ata_write_cbr(&ATA_PIO_DVR, 0);
        if (ata_lba48)
        {
            ata_write_cbr(&ATA_PIO_SCR, cnt >> 5);
            ata_write_cbr(&ATA_PIO_SCR, (cnt << 3) & 0xff);
            ata_write_cbr(&ATA_PIO_LHR, (sector >> 37) & 0xff);
            ata_write_cbr(&ATA_PIO_LMR, (sector >> 29) & 0xff);
            ata_write_cbr(&ATA_PIO_LLR, (sector >> 21) & 0xff);
            ata_write_cbr(&ATA_PIO_LHR, (sector >> 13) & 0xff);
            ata_write_cbr(&ATA_PIO_LMR, (sector >> 5) & 0xff);
            ata_write_cbr(&ATA_PIO_LLR, (sector << 3) & 0xff);
            ata_write_cbr(&ATA_PIO_DVR, BIT(6));
            if (write) ata_write_cbr(&ATA_PIO_CSD, ata_dma ? 0x35 : 0x39);
            else ata_write_cbr(&ATA_PIO_CSD, ata_dma ? 0x25 : 0x29);
        }
        else
        {
            ata_write_cbr(&ATA_PIO_SCR, (cnt << 3) & 0xff);
            ata_write_cbr(&ATA_PIO_LHR, (sector >> 13) & 0xff);
            ata_write_cbr(&ATA_PIO_LMR, (sector >> 5) & 0xff);
            ata_write_cbr(&ATA_PIO_LLR, (sector << 3) & 0xff);
            ata_write_cbr(&ATA_PIO_DVR, BIT(6) | ((sector >> 21) & 0xf));
            if (write) ata_write_cbr(&ATA_PIO_CSD, ata_dma ? 0xca : 0x30);
            else ata_write_cbr(&ATA_PIO_CSD, ata_dma ? 0xc8 : 0xc4);
        }
        if (ata_dma)
        {
            PASS_RC(ata_wait_for_start_of_transfer(500000), 2, 1);
            if (write)
            {
                ATA_SBUF_START = buffer;
                ATA_SBUF_SIZE = SECTOR_SIZE * cnt;
                ATA_CFG |= BIT(4);
            }
            else
            {
                ATA_TBUF_START = buffer;
                ATA_TBUF_SIZE = SECTOR_SIZE * cnt;
                ATA_CFG &= ~BIT(4);
            }
            ATA_XFR_NUM = SECTOR_SIZE * cnt - 1;
            ATA_CFG |= ata_dma_flags;
            ATA_CFG &= ~(BIT(7) | BIT(8));
            semaphore_wait(&ata_wakeup, 0);
            ATA_IRQ = BITRANGE(0, 4);
            ATA_IRQ_MASK = BIT(0);
            ATA_COMMAND = BIT(0);
            if (semaphore_wait(&ata_wakeup, 500000 * HZ / 1000000)
                == OBJ_WAIT_TIMEDOUT)
            {
                ATA_COMMAND = BIT(1);
                ATA_CFG &= ~(BITRANGE(2, 3) | BIT(12));
                RET_ERR(2);
            }
            ATA_COMMAND = BIT(1);
            ATA_CFG &= ~(BITRANGE(2, 3) | BIT(12));
        }
        else
        {
            cnt *= SECTOR_SIZE / 512;
            while (cnt--)
            {
                int i;
                PASS_RC(ata_wait_for_start_of_transfer(500000), 2, 1);
                if (write)
                    for (i = 0; i < 256; i++)
                        ata_write_cbr(&ATA_PIO_DTR, ((uint16_t*)buffer)[i]);
                else
                    for (i = 0; i < 256; i++)
                        ((uint16_t*)buffer)[i] = ata_read_cbr(&ATA_PIO_DTR);
                buffer += 512;
            }
        }
        PASS_RC(ata_wait_for_end_of_transfer(100000), 2, 3);
    }
    return 0;
}

int ata_rw_chunk(uint64_t sector, uint32_t cnt, void* buffer, bool write)
{
    led(true);
    int rc = ata_rw_chunk_internal(sector, cnt, buffer, write);
    led(false);
    return rc;
}

#ifdef ATA_HAVE_BBT
int ata_bbt_translate(uint64_t sector, uint32_t count, uint64_t* phys, uint32_t* physcount)
{
    if (sector + count > ata_virtual_sectors) RET_ERR(0);
    if (!ata_bbt)
    {
        *phys = sector;
        *physcount = count;
        return 0;
    }
    if (!count)
    {
        *phys = 0;
        *physcount = 0;
        return 0;
    }
    uint32_t offset;
    uint32_t l0idx = sector >> 15;
    uint32_t l0offs = sector & 0x7fff;
    *physcount = MIN(count, 0x8000 - l0offs);
    uint32_t l0data = ata_bbt[0][l0idx << 1];
    uint32_t base = ata_bbt[0][(l0idx << 1) | 1] << 12;
    if (l0data < 0x8000) offset = l0data + base;
    else
    {
        uint32_t l1idx = (sector >> 10) & 0x1f;
        uint32_t l1offs = sector & 0x3ff;
        *physcount = MIN(count, 0x400 - l1offs);
        uint32_t l1data = ata_bbt[l0data & 0x7fff][l1idx];
        if (l1data < 0x8000) offset = l1data + base;
        else
        {
            uint32_t l2idx = (sector >> 5) & 0x1f;
            uint32_t l2offs = sector & 0x1f;
            *physcount = MIN(count, 0x20 - l2offs);
            uint32_t l2data = ata_bbt[l1data & 0x7fff][l2idx];
            if (l2data < 0x8000) offset = l2data + base;
            else
            {
                uint32_t l3idx = sector & 0x1f;
                uint32_t l3data = ata_bbt[l2data & 0x7fff][l3idx];
                for (*physcount = 1; *physcount < count && l3idx + *physcount < 0x20; (*physcount)++)
                    if (ata_bbt[l2data & 0x7fff][l3idx + *physcount] != l3data)
                        break;
                offset = l3data + base;
            }
        }
    }
    *phys = sector + offset;
    return 0;
}
#endif

int ata_rw_sectors(uint64_t sector, uint32_t count, void* buffer, bool write)
{
    if (((uint32_t)buffer) & 0xf)
        panicf("ATA: Misaligned data buffer at %08X (sector %lu, count %lu)",
               (unsigned int)buffer, (long unsigned int)sector,  (long unsigned int)count);
#ifdef ATA_HAVE_BBT
    if (sector + count > ata_virtual_sectors) RET_ERR(0);
    if (ata_bbt)
        while (count)
        {
            uint64_t phys;
            uint32_t cnt;
            PASS_RC(ata_bbt_translate(sector, count, &phys, &cnt), 0, 0);
            uint32_t offset = phys - sector;
            if (offset != ata_last_offset && phys - ata_last_phys < 64) ata_soft_reset();
            ata_last_offset = offset;
            ata_last_phys = phys + cnt;
            PASS_RC(ata_rw_sectors_internal(phys, cnt, buffer, write), 0, 0);
            buffer += cnt * SECTOR_SIZE;
            sector += cnt;
            count -= cnt;
        }
    else PASS_RC(ata_rw_sectors_internal(sector, count, buffer, write), 0, 0);
    return 0;
}

int ata_rw_sectors_internal(uint64_t sector, uint32_t count, void* buffer, bool write)
{
#endif
    if (sector + count > ata_total_sectors) RET_ERR(0);
    if (!ata_powered) ata_power_up();
    ata_set_active();
    if (ata_dma && write) clean_dcache();
    else if (ata_dma) invalidate_dcache();
    if (!ceata) ATA_COMMAND = BIT(1);
    while (count)
    {
        uint32_t cnt = MIN(ata_lba48 ? 8192 : 32, count);
        int rc = -1;
        rc = ata_rw_chunk(sector, cnt, buffer, write);
        if (rc && ata_error_srst) ata_soft_reset();
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
                    if (rc && ata_error_srst) ata_soft_reset();
                }
                if (rc) break;
                buf += SECTOR_SIZE;
            }
        }
        PASS_RC(rc, 1, 1);
        buffer += SECTOR_SIZE * cnt;
        sector += cnt;
        count -= cnt;
    }
    ata_set_active();
    return 0;
}

static void ata_thread(void)
{
    while (true)
    {
        mutex_lock(&ata_mutex);
        if (TIME_AFTER(current_tick, ata_last_activity_value + ata_sleep_timeout) && ata_powered)
        {
            call_storage_idle_notifys(false);
            ata_power_down();
        }
        mutex_unlock(&ata_mutex);
        sleep(HZ / 2);
    }
}

/* API Functions */
int ata_soft_reset(void)
{
    int rc;
    mutex_lock(&ata_mutex);
    if (!ata_powered) ata_power_up();
    ata_set_active();
    if (ceata) rc = ceata_soft_reset();
    else
    {
        ata_write_cbr(&ATA_PIO_DAD, BIT(1) | BIT(2));
        udelay(10);
        ata_write_cbr(&ATA_PIO_DAD, 0);
        rc = ata_wait_for_rdy(20000000);
    }
    if (IS_ERR(rc))
    {
        ata_power_down();
        sleep(HZ * 3);
        ata_power_up();
    }
    ata_set_active();
    mutex_unlock(&ata_mutex);
    return rc;
}

int ata_read_sectors(IF_MD2(int drive,) unsigned long start, int incount,
                     void* inbuf)
{
    mutex_lock(&ata_mutex);
    int rc = ata_rw_sectors(start, incount, inbuf, false);
    mutex_unlock(&ata_mutex);
    return rc;
}

int ata_write_sectors(IF_MD2(int drive,) unsigned long start, int count,
                      const void* outbuf)
{
    mutex_lock(&ata_mutex);
    int rc = ata_rw_sectors(start, count, (void*)((uint32_t)outbuf), true);
    mutex_unlock(&ata_mutex);
    return rc;
}

void ata_spindown(int seconds)
{
    ata_sleep_timeout = seconds * HZ;
}

void ata_sleep(void)
{
    ata_last_activity_value = current_tick - ata_sleep_timeout + HZ / 5;
}

void ata_sleepnow(void)
{
    mutex_lock(&ata_mutex);
    ata_power_down();
    mutex_unlock(&ata_mutex);
}

void ata_close(void)
{
    ata_sleepnow();
}

void ata_spin(void)
{
    ata_set_active();
}

void ata_get_info(IF_MD2(int drive,) struct storage_info *info)
{
    (*info).sector_size = SECTOR_SIZE;
#ifdef ATA_HAVE_BBT
    (*info).num_sectors = ata_virtual_sectors;
#else
    (*info).num_sectors = ata_total_sectors;
#endif
    (*info).vendor = "Apple";
    (*info).product = "iPod Classic";
    (*info).revision = "1.0";
}

long ata_last_disk_activity(void)
{
    return ata_last_activity_value;
}

#ifdef ATA_HAVE_BBT
void ata_bbt_disable(void)
{
    mutex_lock(&ata_mutex);
    ata_bbt = NULL;
    ata_virtual_sectors = ata_total_sectors;
    mutex_unlock(&ata_mutex);
}

void ata_bbt_reload(void)
{
    mutex_lock(&ata_mutex);
    ata_bbt_disable();
    ata_power_up();
    uint32_t* buf = (uint32_t*)(ata_bbt_buf + sizeof(ata_bbt_buf) - SECTOR_SIZE);
    if (buf)
    {
        if (IS_ERR(ata_bbt_read_sectors(0, 1, buf)))
            ata_virtual_sectors = ata_total_sectors;
        else if (!memcmp(buf, "emBIbbth", 8))
        {
            ata_virtual_sectors = (((uint64_t)buf[0x1fd]) << 32) | buf[0x1fc];
            uint32_t count = buf[0x1ff];
            if (count > ATA_BBT_PAGES / 64)
                panicf("ATA: BBT too big! (space: %d, size: %d)",
                       ATA_BBT_PAGES, (unsigned int)(count * 64));
            uint32_t i;
            uint32_t cnt;
            ata_bbt = (typeof(ata_bbt))ata_bbt_buf;
            for (i = 0; i < count; i += cnt)
            {
                uint32_t phys = buf[0x200 + i];
                for (cnt = 1; cnt < count; cnt++)
                    if (buf[0x200 + i + cnt] != phys + cnt)
                        break;
                if (IS_ERR(ata_bbt_read_sectors(phys, cnt, ata_bbt[i << 6])))
                {
                    ata_virtual_sectors = ata_total_sectors;
                    break;
                }
            }
        }
        else ata_virtual_sectors = ata_total_sectors;
    }
    else ata_virtual_sectors = ata_total_sectors;
    mutex_unlock(&ata_mutex);
}
#endif

int ata_init(void)
{
    mutex_init(&ata_mutex);
    semaphore_init(&ata_wakeup, 1, 0);
    semaphore_init(&mmc_wakeup, 1, 0);
    semaphore_init(&mmc_comp_wakeup, 1, 0);
    ceata = PDAT(11) & BIT(1);
    if (ceata)
    {
        ata_lba48 = true;
        ata_dma = true;
        PCON(8) = 0x33333333;
        PCON(9) = (PCON(9) & ~0xff) | 0x33;
        PCON(11) |= 0xf;
        *((uint32_t volatile*)0x38a00000) = 0;
        *((uint32_t volatile*)0x38700000) = 0;
    }
    else
    {
        PCON(7) = 0x44444444;
        PCON(8) = 0x44444444;
        PCON(9) = 0x44444444;
        PCON(10) = (PCON(10) & ~0xffff) | 0x4444;
    }
    ata_powered = false;
    ata_total_sectors = 0;
    ata_power_up();
#ifdef ATA_HAVE_BBT
    ata_bbt_reload();
#endif
    create_thread(ata_thread, ata_stack,
                    sizeof(ata_stack), 0, "ATA idle monitor"
                    IF_PRIO(, PRIORITY_USER_INTERFACE)
                    IF_COP(, CPU));
    return 0;
}

int ata_num_drives(int first_drive)
{
    /* We don't care which logical drive number(s) we have been assigned */
    (void)first_drive;
    
    return 1;
}

unsigned short* ata_get_identify(void)
{
    return ata_identify_data;
}

int ata_spinup_time(void)
{
    return spinup_time;
}

int ata_get_dma_mode(void)
{
    return dma_mode;
}

void INT_ATA(void)
{
    uint32_t ata_irq = ATA_IRQ;
    ATA_IRQ = ata_irq;
    if (ata_irq & ATA_IRQ_MASK) semaphore_release(&ata_wakeup);
    ATA_IRQ_MASK = 0;
}

void INT_MMC(void)
{
    uint32_t irq = SDCI_IRQ;
    if (irq & SDCI_IRQ_DAT_DONE_INT) semaphore_release(&mmc_wakeup);
    if (irq & SDCI_IRQ_IOCARD_IRQ_INT) semaphore_release(&mmc_comp_wakeup);
    SDCI_IRQ = irq;
}
