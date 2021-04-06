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
#include "nand-target.h"
#include "sfc-x1000.h"
#include "system.h"
#include <string.h>

#if !defined(NAND_MAX_PAGE_SIZE) ||     \
    !defined(NAND_INIT_SFC_DEV_CONF) || \
    !defined(NAND_INIT_CLOCK_SPEED)
# error "Target needs to specify NAND driver parameters"
#endif

/* Must be at least as big as a cacheline */
#define NAND_AUX_BUFFER_SIZE CACHEALIGN_SIZE

/* Writes have been enabled */
#define NAND_DRV_FLAG_WRITEABLE 0x01

/* Defined by target */
extern const nand_chip_desc target_nand_chip_descs[];

#ifdef BOOTLOADER_SPL
# define NANDBUFFER_ATTR __attribute__((section(".sdram"))) CACHEALIGN_ATTR
#else
# define NANDBUFFER_ATTR CACHEALIGN_ATTR
#endif

/* Globals for the driver */
static unsigned char pagebuffer[NAND_MAX_PAGE_SIZE] NANDBUFFER_ATTR;
static unsigned char auxbuffer[NAND_AUX_BUFFER_SIZE] NANDBUFFER_ATTR;
static nand_drv nand_driver;

static void nand_drv_reset(nand_drv* d)
{
    d->chip_ops = NULL;
    d->chip_data = NULL;
    d->pagebuf = &pagebuffer[0];
    d->auxbuf = &auxbuffer[0];
    d->raw_page_size = 0;
    d->flags = 0;
}

/* Driver code */
int nand_open(void)
{
    sfc_init();
    sfc_lock();

    /* Reset driver state */
    nand_drv_reset(&nand_driver);

    /* Init hardware */
    sfc_open();
    sfc_set_dev_conf(NAND_INIT_SFC_DEV_CONF);
    sfc_set_clock(NAND_CLOCK_SOURCE, NAND_INIT_CLOCK_SPEED);

    /* Identify NAND chip */
    int status = 0;
    int nandid = nandcmd_read_id(&nand_driver);
    if(nandid < 0) {
        status = NANDERR_CHIP_UNSUPPORTED;
        goto _err;
    }

    unsigned char mf_id = nandid >> 8;
    unsigned char dev_id = nandid & 0xff;
    const nand_chip_desc* desc = &target_nand_chip_descs[0];
    while(1) {
        if(desc->data == NULL || desc->ops == NULL) {
            status = NANDERR_CHIP_UNSUPPORTED;
            goto _err;
        }

        if(desc->data->mf_id == mf_id && desc->data->dev_id == dev_id)
            break;
    }

    /* Fill driver parameters */
    nand_driver.chip_ops = desc->ops;
    nand_driver.chip_data = desc->data;
    nand_driver.raw_page_size = desc->data->page_size + desc->data->spare_size;

    /* Configure hardware and run init op */
    sfc_set_dev_conf(desc->data->dev_conf);
    sfc_set_clock(NAND_CLOCK_SOURCE, desc->data->clock_freq);

    if((status = desc->ops->open(&nand_driver)) < 0)
        goto _err;

  _exit:
    sfc_unlock();
    return status;
  _err:
    nand_drv_reset(&nand_driver);
    sfc_close();
    goto _exit;
}

void nand_close(void)
{
    sfc_lock();
    nand_driver.chip_ops->close(&nand_driver);
    nand_drv_reset(&nand_driver);
    sfc_close();
    sfc_unlock();
}

int nand_enable_writes(bool en)
{
    sfc_lock();

    int st = nand_driver.chip_ops->set_wp_enable(&nand_driver, en);
    if(st >= 0) {
        if(en)
            nand_driver.flags |= NAND_DRV_FLAG_WRITEABLE;
        else
            nand_driver.flags &= ~NAND_DRV_FLAG_WRITEABLE;
    }

    sfc_unlock();
    return st;
}

extern int nand_read_bytes(uint32_t byteaddr, int count, void* buf)
{
    if(count <= 0)
        return 0;

    nand_drv* d = &nand_driver;
    uint32_t rowaddr = byteaddr / d->chip_data->page_size;
    uint32_t coladdr = byteaddr % d->chip_data->page_size;
    unsigned char* dstbuf = (unsigned char*)buf;
    int status = 0;

    sfc_lock();
    do {
        if(d->chip_ops->read_page(d, rowaddr, d->pagebuf) < 0) {
            status = NANDERR_READ_FAILED;
            goto _end;
        }

        if(d->chip_ops->ecc_read(d, d->pagebuf) < 0) {
            status = NANDERR_ECC_FAILED;
            goto _end;
        }

        int amount = d->chip_data->page_size - coladdr;
        if(amount > count)
            amount = count;

        memcpy(dstbuf, d->pagebuf, amount);
        dstbuf += amount;
        count -= amount;
        rowaddr += 1;
        coladdr = 0;
    } while(count > 0);

  _end:
    sfc_unlock();
    return status;
}

int nand_write_bytes(uint32_t byteaddr, int count, const void* buf)
{
    nand_drv* d = &nand_driver;

    if((d->flags & NAND_DRV_FLAG_WRITEABLE) == 0)
        return NANDERR_WRITE_PROTECTED;

    if(count <= 0)
        return 0;

    uint32_t rowaddr = byteaddr / d->chip_data->page_size;
    uint32_t coladdr = byteaddr % d->chip_data->page_size;

    /* Only support whole page writes right now */
    if(coladdr != 0)
        return NANDERR_UNALIGNED_ADDRESS;
    if(count % d->chip_data->page_size)
        return NANDERR_UNALIGNED_LENGTH;

    const unsigned char* srcbuf = (const unsigned char*)buf;
    int status = 0;

    sfc_lock();
    do {
        memcpy(d->pagebuf, srcbuf, d->chip_data->page_size);
        d->chip_ops->ecc_write(d, d->pagebuf);

        if(d->chip_ops->write_page(d, rowaddr, d->pagebuf) < 0) {
            status = NANDERR_PROGRAM_FAILED;
            goto _end;
        }

        rowaddr += 1;
        srcbuf += d->chip_data->page_size;
        count -= d->chip_data->page_size;
    } while(count > 0);

  _end:
    sfc_unlock();
    return status;
}

int nand_erase_bytes(uint32_t byteaddr, int count)
{
    nand_drv* d = &nand_driver;

    if((d->flags & NAND_DRV_FLAG_WRITEABLE) == 0)
        return NANDERR_WRITE_PROTECTED;

    /* Ensure address is aligned to a block boundary */
    if(byteaddr % d->chip_data->page_size)
        return NANDERR_UNALIGNED_ADDRESS;

    uint32_t blockaddr = byteaddr / d->chip_data->page_size;
    if(blockaddr % d->chip_data->block_size)
        return NANDERR_UNALIGNED_ADDRESS;

    /* Ensure length is also aligned to the size of a block */
    if(count % d->chip_data->page_size)
        return NANDERR_UNALIGNED_LENGTH;

    count /= d->chip_data->page_size;
    if(count % d->chip_data->block_size)
        return NANDERR_UNALIGNED_LENGTH;

    count /= d->chip_data->block_size;

    int status = 0;
    sfc_lock();

    for(int i = 0; i < count; ++i) {
        if(d->chip_ops->erase_block(d, blockaddr)) {
            status = NANDERR_ERASE_FAILED;
            goto _end;
        }

        /* Advance to next block */
        blockaddr += d->chip_data->block_size;
    }

  _end:
    sfc_unlock();
    return status;
}

int nandcmd_read_id(nand_drv* d)
{
    sfc_op op = {0};
    op.command = NAND_CMD_READ_ID;
    op.flags = SFC_FLAG_READ;
    op.addr_bytes = 1;
    op.addr_lo = 0;
    op.data_bytes = 2;
    op.buffer = d->auxbuf;
    if(sfc_exec(&op))
        return NANDERR_COMMAND_FAILED;

    return (d->auxbuf[0] << 8) | d->auxbuf[1];
}

int nandcmd_write_enable(nand_drv* d)
{
    (void)d;

    sfc_op op = {0};
    op.command = NAND_CMD_WRITE_ENABLE;
    if(sfc_exec(&op))
        return NANDERR_COMMAND_FAILED;

    return 0;
}

int nandcmd_get_feature(nand_drv* d, int reg)
{
    sfc_op op = {0};
    op.command = NAND_CMD_GET_FEATURE;
    op.flags = SFC_FLAG_READ;
    op.addr_bytes = 1;
    op.addr_lo = reg & 0xff;
    op.data_bytes = 1;
    op.buffer = d->auxbuf;
    if(sfc_exec(&op))
        return NANDERR_COMMAND_FAILED;

    return d->auxbuf[0];
}

int nandcmd_set_feature(nand_drv* d, int reg, int val)
{
    sfc_op op = {0};
    op.command = NAND_CMD_SET_FEATURE;
    op.flags = SFC_FLAG_READ;
    op.addr_bytes = 1;
    op.addr_lo = reg & 0xff;
    op.data_bytes = 1;
    op.buffer = d->auxbuf;
    d->auxbuf[0] = val & 0xff;
    if(sfc_exec(&op))
        return NANDERR_COMMAND_FAILED;

    return 0;
}

int nandcmd_page_read_to_cache(nand_drv* d, uint32_t rowaddr)
{
    sfc_op op = {0};
    op.command = NAND_CMD_PAGE_READ_TO_CACHE;
    op.addr_bytes = d->chip_data->rowaddr_width;
    op.addr_lo = rowaddr;
    if(sfc_exec(&op))
        return NANDERR_COMMAND_FAILED;

    return 0;
}

int nandcmd_read_from_cache(nand_drv* d, unsigned char* buf)
{
    sfc_op op = {0};
    if(d->chip_data->flags & NANDCHIP_FLAG_QUAD) {
        op.command = NAND_CMD_READ_FROM_CACHEx4;
        op.mode = SFC_MODE_QUAD_IO;
    } else {
        op.command = NAND_CMD_READ_FROM_CACHE;
        op.mode = SFC_MODE_STANDARD;
    }

    op.flags = SFC_FLAG_READ;
    op.addr_bytes = d->chip_data->coladdr_width;
    op.addr_lo = 0;
    op.dummy_bits = 8;
    op.data_bytes = d->raw_page_size;
    op.buffer = buf;
    if(sfc_exec(&op))
        return NANDERR_COMMAND_FAILED;

    return 0;
}

int nandcmd_program_load(nand_drv* d, const unsigned char* buf)
{
    sfc_op op = {0};
    if(d->chip_data->flags & NANDCHIP_FLAG_QUAD) {
        op.command = NAND_CMD_PROGRAM_LOADx4;
        op.mode = SFC_MODE_QUAD_IO;
    } else {
        op.command = NAND_CMD_PROGRAM_LOAD;
        op.mode = SFC_MODE_STANDARD;
    }

    op.flags = SFC_FLAG_WRITE;
    op.addr_bytes = d->chip_data->coladdr_width;
    op.addr_lo = 0;
    op.data_bytes = d->raw_page_size;
    op.buffer = (void*)buf;
    if(sfc_exec(&op))
        return NANDERR_COMMAND_FAILED;

    return 0;
}

int nandcmd_program_execute(nand_drv* d, uint32_t rowaddr)
{
    sfc_op op = {0};
    op.command = NAND_CMD_PROGRAM_EXECUTE;
    op.addr_bytes = d->chip_data->rowaddr_width;
    op.addr_lo = rowaddr;
    if(sfc_exec(&op))
        return NANDERR_COMMAND_FAILED;

    return 0;
}

int nandcmd_block_erase(nand_drv* d, uint32_t blockaddr)
{
    sfc_op op = {0};
    op.command = NAND_CMD_BLOCK_ERASE;
    op.addr_bytes = d->chip_data->rowaddr_width;
    op.addr_lo = blockaddr;
    if(sfc_exec(&op))
        return NANDERR_COMMAND_FAILED;

    return 0;
}

const nand_chip_ops nand_chip_ops_std = {
    .open = nandop_std_open,
    .close = nandop_std_close,
    .read_page = nandop_std_read_page,
    .write_page = nandop_std_write_page,
    .erase_block = nandop_std_erase_block,
    .set_wp_enable = nandop_std_set_wp_enable,
    .ecc_read = nandop_ecc_none_read,
    .ecc_write = nandop_ecc_none_write,
};

/* Helper needed by other ops */
static int nandop_std_wait_status(nand_drv* d, int errbit)
{
    int reg;
    do {
        reg = nandcmd_get_feature(d, NAND_FREG_STATUS);
        if(reg < 0)
            return reg;
    } while(reg & NAND_FREG_STATUS_OIP);

    if(reg & errbit)
        return NANDERR_OTHER;

    return reg;
}

int nandop_std_open(nand_drv* d)
{
    (void)d;
    return 0;
}

void nandop_std_close(nand_drv* d)
{
    (void)d;
}

int nandop_std_read_page(nand_drv* d, uint32_t rowaddr, unsigned char* buf)
{
    int status;

    if((status = nandcmd_page_read_to_cache(d, rowaddr)) < 0)
        return status;
    if((status = nandop_std_wait_status(d, 0)) < 0)
        return status;
    if((status = nandcmd_read_from_cache(d, buf)) < 0)
        return status;

    return 0;
}

int nandop_std_write_page(nand_drv* d, uint32_t rowaddr, const unsigned char* buf)
{
    int status;

    if((status = nandcmd_write_enable(d)) < 0)
        return status;
    if((status = nandcmd_program_load(d, buf)) < 0)
        return status;
    if((status = nandcmd_program_execute(d, rowaddr)) < 0)
        return status;
    if((status = nandop_std_wait_status(d, NAND_FREG_STATUS_P_FAIL)) < 0)
        return status;

    return 0;
}

int nandop_std_erase_block(nand_drv* d, uint32_t blockaddr)
{
    int status;

    if((status = nandcmd_write_enable(d)) < 0)
        return status;
    if((status = nandcmd_block_erase(d, blockaddr)) < 0)
        return status;
    if((status = nandop_std_wait_status(d, NAND_FREG_STATUS_E_FAIL) < 0))
        return status;

    return 0;
}

int nandop_std_set_wp_enable(nand_drv* d, bool en)
{
    int val = nandcmd_get_feature(d, NAND_FREG_PROTECTION);
    if(val < 0)
        return val;

    if(en) {
        val &= ~NAND_FREG_PROTECTION_ALLBP;
        if(d->chip_data->flags & NANDCHIP_FLAG_USE_BRWD)
            val &= ~NAND_FREG_PROTECTION_BRWD;
    } else {
        val |= NAND_FREG_PROTECTION_ALLBP;
        if(d->chip_data->flags & NANDCHIP_FLAG_USE_BRWD)
            val |= NAND_FREG_PROTECTION_BRWD;
    }

    sfc_set_wp_enable(false);
    int status = nandcmd_set_feature(d, NAND_FREG_PROTECTION, val);
    sfc_set_wp_enable(true);

    if(status < 0)
        return status;

    return 0;
}

int nandop_ecc_none_read(nand_drv* d, unsigned char* buf)
{
    (void)d;
    (void)buf;
    return 0;
}

void nandop_ecc_none_write(nand_drv* d, unsigned char* buf)
{
    memset(&buf[d->chip_data->page_size], 0xff, d->chip_data->spare_size);
}
