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

#include "nand-x1000.h"
#include "sfc-x1000.h"
#include "system.h"
#include <stddef.h>

/* NAND command numbers */
#define NAND_CMD_READ_ID            0x9f
#define NAND_CMD_WRITE_ENABLE       0x06
#define NAND_CMD_GET_FEATURE        0x0f
#define NAND_CMD_SET_FEATURE        0x1f
#define NAND_CMD_PAGE_READ_TO_CACHE 0x13
#define NAND_CMD_READ_FROM_CACHE    0x0b
#define NAND_CMD_READ_FROM_CACHEx4  0x6b
#define NAND_CMD_PROGRAM_LOAD       0x02
#define NAND_CMD_PROGRAM_LOADx4     0x32
#define NAND_CMD_PROGRAM_EXECUTE    0x10
#define NAND_CMD_BLOCK_ERASE        0xd8

/* NAND device register addresses for GET_FEATURE / SET_FEATURE */
#define NAND_FREG_PROTECTION        0xa0
#define NAND_FREG_FEATURE           0xb0
#define NAND_FREG_STATUS            0xc0

/* Protection register bits */
#define NAND_FREG_PROTECTION_BRWD   0x80
#define NAND_FREG_PROTECTION_BP2    0x20
#define NAND_FREG_PROTECTION_BP1    0x10
#define NAND_FREG_PROTECTION_BP0    0x80
/* Mask of BP bits 0-2 */
#define NAND_FREG_PROTECTION_ALLBP  (0x38)

/* Feature register bits */
#define NAND_FREG_FEATURE_QE        0x01

/* Status register bits */
#define NAND_FREG_STATUS_OIP        0x01
#define NAND_FREG_STATUS_WEL        0x02
#define NAND_FREG_STATUS_E_FAIL     0x04
#define NAND_FREG_STATUS_P_FAIL     0x08

/* NAND chip config */
const nand_chip_data target_nand_chip_data[] = {
#ifdef FIIO_M3K
    {
        /* ATO25D1GA */
        .mf_id = 0x9b,
        .dev_id = 0x12,
        .dev_conf = jz_orf(SFC_DEV_CONF, CE_DL(1), HOLD_DL(1), WP_DL(1),
                           CPHA(0), CPOL(0), TSH(7), TSETUP(0), THOLD(0),
                           STA_TYPE_V(1BYTE), CMD_TYPE_V(8BITS), SMP_DELAY(1)),
        .clock_freq = 150000000,
        .log2_page_size = 11, /* = 2048 bytes */
        .log2_block_size = 6, /* = 64 pages */
        .rowaddr_width = 3,
        .coladdr_width = 2,
        .flags = NANDCHIP_FLAG_QUAD,
    }
#else
    /* Nobody will use this anyway if the device has no NAND flash */
    { 0 },
#endif
};

const size_t target_nand_chip_count =
    sizeof(target_nand_chip_data) / sizeof(nand_chip_data);

/* NAND ops -- high level primitives used by the driver */
static int nandop_wait_status(int errbit);
static int nandop_read_page(uint32_t row_addr, uint8_t* buf);
static int nandop_write_page(uint32_t row_addr, const uint8_t* buf);
static int nandop_erase_block(uint32_t block_addr);
static int nandop_set_write_protect(bool en);

/* NAND commands -- 1-to-1 mapping between chip commands and functions */
static int nandcmd_read_id(int* mf_id, int* dev_id);
static int nandcmd_write_enable(void);
static int nandcmd_get_feature(uint8_t reg);
static int nandcmd_set_feature(uint8_t reg, uint8_t val);
static int nandcmd_page_read_to_cache(uint32_t row_addr);
static int nandcmd_read_from_cache(uint8_t* buf);
static int nandcmd_program_load(const uint8_t* buf);
static int nandcmd_program_execute(uint32_t row_addr);
static int nandcmd_block_erase(uint32_t block_addr);

struct nand_drv {
    const nand_chip_data* chip_data;
    bool write_enabled;
};

static struct nand_drv nand_drv;
static uint8_t nand_auxbuf[32] CACHEALIGN_ATTR;

static void nand_drv_reset(void)
{
    nand_drv.chip_data = NULL;
    nand_drv.write_enabled = false;
}

int nand_open(void)
{
    sfc_init();
    sfc_lock();

    nand_drv_reset();
    sfc_open();

    const nand_chip_data* chip_data = &target_nand_chip_data[0];
    sfc_set_dev_conf(chip_data->dev_conf);
    sfc_set_clock(chip_data->clock_freq);

    return NAND_SUCCESS;
}

void nand_close(void)
{
    sfc_lock();
    sfc_close();
    nand_drv_reset();
    sfc_unlock();
}

int nand_identify(int* mf_id, int* dev_id)
{
    sfc_lock();

    int status = nandcmd_read_id(mf_id, dev_id);
    if(status < 0)
        goto error;

    for(size_t i = 0; i < target_nand_chip_count; ++i) {
        const nand_chip_data* data = &target_nand_chip_data[i];
        if(data->mf_id == *mf_id && data->dev_id == *dev_id) {
            nand_drv.chip_data = data;
            break;
        }
    }

    if(!nand_drv.chip_data) {
        status = NAND_ERR_UNKNOWN_CHIP;
        goto error;
    }

    /* Set parameters according to new chip data */
    sfc_set_dev_conf(nand_drv.chip_data->dev_conf);
    sfc_set_clock(nand_drv.chip_data->clock_freq);
    status = NAND_SUCCESS;

  error:
    sfc_unlock();
    return status;
}

const nand_chip_data* nand_get_chip_data(void)
{
    return nand_drv.chip_data;
}

extern int nand_enable_writes(bool en)
{
    if(en == nand_drv.write_enabled)
        return NAND_SUCCESS;

    int rc = nandop_set_write_protect(!en);
    if(rc == NAND_SUCCESS)
        nand_drv.write_enabled = en;

    return rc;
}

static int nand_rdwr(bool write, uint32_t addr, uint32_t size, uint8_t* buf)
{
    const uint32_t page_size = (1 << nand_drv.chip_data->log2_page_size);

    if(addr & (page_size - 1))
        return NAND_ERR_UNALIGNED;
    if(size & (page_size - 1))
        return NAND_ERR_UNALIGNED;
    if(size <= 0)
        return NAND_SUCCESS;
    if(write && !nand_drv.write_enabled)
        return NAND_ERR_WRITE_PROTECT;
    if((uint32_t)buf & (CACHEALIGN_SIZE - 1))
        return NAND_ERR_UNALIGNED;

    addr >>= nand_drv.chip_data->log2_page_size;
    size >>= nand_drv.chip_data->log2_page_size;

    int rc = NAND_SUCCESS;
    sfc_lock();

    for(; size > 0; --size, ++addr, buf += page_size) {
        if(write)
            rc = nandop_write_page(addr, buf);
        else
            rc = nandop_read_page(addr, buf);

        if(rc)
            break;
    }

    sfc_unlock();
    return rc;
}

int nand_read(uint32_t addr, uint32_t size, uint8_t* buf)
{
    return nand_rdwr(false, addr, size, buf);
}

int nand_write(uint32_t addr, uint32_t size, const uint8_t* buf)
{
    return nand_rdwr(true, addr, size, (uint8_t*)buf);
}

int nand_erase(uint32_t addr, uint32_t size)
{
    const uint32_t page_size = 1 << nand_drv.chip_data->log2_page_size;
    const uint32_t block_size = page_size << nand_drv.chip_data->log2_block_size;
    const uint32_t pages_per_block = 1 << nand_drv.chip_data->log2_page_size;

    if(addr & (block_size - 1))
        return NAND_ERR_UNALIGNED;
    if(size & (block_size - 1))
        return NAND_ERR_UNALIGNED;
    if(size <= 0)
        return NAND_SUCCESS;
    if(!nand_drv.write_enabled)
        return NAND_ERR_WRITE_PROTECT;

    addr >>= nand_drv.chip_data->log2_page_size;
    size >>= nand_drv.chip_data->log2_page_size;
    size >>= nand_drv.chip_data->log2_block_size;

    int rc = NAND_SUCCESS;
    sfc_lock();

    for(; size > 0; --size, addr += pages_per_block)
        if((rc = nandop_erase_block(addr)))
            break;

    sfc_unlock();
    return rc;
}

/*
 * NAND ops
 */

static int nandop_wait_status(int errbit)
{
    int reg;
    do {
        reg = nandcmd_get_feature(NAND_FREG_STATUS);
        if(reg < 0)
            return reg;
    } while(reg & NAND_FREG_STATUS_OIP);

    if(reg & errbit)
        return NAND_ERR_COMMAND;

    return reg;
}

static int nandop_read_page(uint32_t row_addr, uint8_t* buf)
{
    int status;

    if((status = nandcmd_page_read_to_cache(row_addr)) < 0)
        return status;
    if((status = nandop_wait_status(0)) < 0)
        return status;
    if((status = nandcmd_read_from_cache(buf)) < 0)
        return status;

    return NAND_SUCCESS;
}

static int nandop_write_page(uint32_t row_addr, const uint8_t* buf)
{
    int status;

    if((status = nandcmd_write_enable()) < 0)
        return status;
    if((status = nandcmd_program_load(buf)) < 0)
        return status;
    if((status = nandcmd_program_execute(row_addr)) < 0)
        return status;
    if((status = nandop_wait_status(NAND_FREG_STATUS_P_FAIL)) < 0)
        return status;

    return NAND_SUCCESS;
}

static int nandop_erase_block(uint32_t block_addr)
{
    int status;

    if((status = nandcmd_write_enable()) < 0)
        return status;
    if((status = nandcmd_block_erase(block_addr)) < 0)
        return status;
    if((status = nandop_wait_status(NAND_FREG_STATUS_E_FAIL)) < 0)
        return status;

    return NAND_SUCCESS;
}

static int nandop_set_write_protect(bool en)
{
    int val = nandcmd_get_feature(NAND_FREG_PROTECTION);
    if(val < 0)
        return val;

    if(en) {
        val &= ~NAND_FREG_PROTECTION_ALLBP;
        if(nand_drv.chip_data->flags & NANDCHIP_FLAG_USE_BRWD)
            val &= ~NAND_FREG_PROTECTION_BRWD;
    } else {
        val |= NAND_FREG_PROTECTION_ALLBP;
        if(nand_drv.chip_data->flags & NANDCHIP_FLAG_USE_BRWD)
            val |= NAND_FREG_PROTECTION_BRWD;
    }

    /* NOTE: The WP pin typically only protects changes to the protection
     * register -- it doesn't actually prevent writing to the chip. That's
     * why it should be re-enabled after setting the new protection status.
     */
    sfc_set_wp_enable(false);
    int status = nandcmd_set_feature(NAND_FREG_PROTECTION, val);
    sfc_set_wp_enable(true);

    if(status < 0)
        return status;

    return NAND_SUCCESS;
}

/*
 * Low-level NAND commands
 */

static int nandcmd_read_id(int* mf_id, int* dev_id)
{
    sfc_op op = {0};
    op.command = NAND_CMD_READ_ID;
    op.flags = SFC_FLAG_READ;
    op.addr_bytes = 1;
    op.addr_lo = 0;
    op.data_bytes = 2;
    op.buffer = nand_auxbuf;
    if(sfc_exec(&op))
        return NAND_ERR_CONTROLLER;

    *mf_id = nand_auxbuf[0];
    *dev_id = nand_auxbuf[1];
    return NAND_SUCCESS;
}

static int nandcmd_write_enable(void)
{
    sfc_op op = {0};
    op.command = NAND_CMD_WRITE_ENABLE;
    if(sfc_exec(&op))
        return NAND_ERR_CONTROLLER;

    return NAND_SUCCESS;
}

static int nandcmd_get_feature(uint8_t reg)
{
    sfc_op op = {0};
    op.command = NAND_CMD_GET_FEATURE;
    op.flags = SFC_FLAG_READ;
    op.addr_bytes = 1;
    op.addr_lo = reg;
    op.data_bytes = 1;
    op.buffer = nand_auxbuf;
    if(sfc_exec(&op))
        return NAND_ERR_CONTROLLER;

    return nand_auxbuf[0];
}

static int nandcmd_set_feature(uint8_t reg, uint8_t val)
{
    sfc_op op = {0};
    op.command = NAND_CMD_SET_FEATURE;
    op.flags = SFC_FLAG_READ;
    op.addr_bytes = 1;
    op.addr_lo = reg;
    op.data_bytes = 1;
    op.buffer = nand_auxbuf;
    nand_auxbuf[0] = val;
    if(sfc_exec(&op))
        return NAND_ERR_CONTROLLER;

    return NAND_SUCCESS;
}

static int nandcmd_page_read_to_cache(uint32_t row_addr)
{
    sfc_op op = {0};
    op.command = NAND_CMD_PAGE_READ_TO_CACHE;
    op.addr_bytes = nand_drv.chip_data->rowaddr_width;
    op.addr_lo = row_addr;
    if(sfc_exec(&op))
        return NAND_ERR_CONTROLLER;

    return NAND_SUCCESS;
}

static int nandcmd_read_from_cache(uint8_t* buf)
{
    sfc_op op = {0};
    if(nand_drv.chip_data->flags & NANDCHIP_FLAG_QUAD) {
        op.command = NAND_CMD_READ_FROM_CACHEx4;
        op.mode = SFC_MODE_QUAD_IO;
    } else {
        op.command = NAND_CMD_READ_FROM_CACHE;
        op.mode = SFC_MODE_STANDARD;
    }

    op.flags = SFC_FLAG_READ;
    op.addr_bytes = nand_drv.chip_data->coladdr_width;
    op.addr_lo = 0;
    op.dummy_bits = 8; // NOTE: this may need a chip_data parameter
    op.data_bytes = (1 << nand_drv.chip_data->log2_page_size);
    op.buffer = buf;
    if(sfc_exec(&op))
        return NAND_ERR_CONTROLLER;

    return NAND_SUCCESS;
}

static int nandcmd_program_load(const uint8_t* buf)
{
    sfc_op op = {0};
    if(nand_drv.chip_data->flags & NANDCHIP_FLAG_QUAD) {
        op.command = NAND_CMD_PROGRAM_LOADx4;
        op.mode = SFC_MODE_QUAD_IO;
    } else {
        op.command = NAND_CMD_PROGRAM_LOAD;
        op.mode = SFC_MODE_STANDARD;
    }

    op.flags = SFC_FLAG_WRITE;
    op.addr_bytes = nand_drv.chip_data->coladdr_width;
    op.addr_lo = 0;
    op.data_bytes = (1 << nand_drv.chip_data->log2_page_size);
    op.buffer = (void*)buf;
    if(sfc_exec(&op))
        return NAND_ERR_CONTROLLER;

    return NAND_SUCCESS;
}

static int nandcmd_program_execute(uint32_t row_addr)
{
    sfc_op op = {0};
    op.command = NAND_CMD_PROGRAM_EXECUTE;
    op.addr_bytes = nand_drv.chip_data->rowaddr_width;
    op.addr_lo = row_addr;
    if(sfc_exec(&op))
        return NAND_ERR_CONTROLLER;

    return NAND_SUCCESS;
}

static int nandcmd_block_erase(uint32_t block_addr)
{
    sfc_op op = {0};
    op.command = NAND_CMD_BLOCK_ERASE;
    op.addr_bytes = nand_drv.chip_data->rowaddr_width;
    op.addr_lo = block_addr;
    if(sfc_exec(&op))
        return NAND_ERR_CONTROLLER;

    return NAND_SUCCESS;
}
