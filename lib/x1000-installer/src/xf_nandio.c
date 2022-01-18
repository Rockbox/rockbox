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

#include "xf_nandio.h"
#include "xf_error.h"
#include "core_alloc.h"
#include "system.h"
#include <string.h>
#include <stdbool.h>

int xf_nandio_init(struct xf_nandio* nio)
{
    int rc;

    memset(nio, 0, sizeof(*nio));

    /* open NAND */
    nio->ndrv = nand_init();
    nand_lock(nio->ndrv);
    rc = nand_open(nio->ndrv);
    if(rc != NAND_SUCCESS) {
        nio->nand_err = rc;
        rc = XF_E_NAND;
        goto out;
    }

    /* read chip parameters */
    nio->page_size = nio->ndrv->chip->page_size;
    nio->block_size = nio->page_size << nio->ndrv->chip->log2_ppb;

    /* allocate memory */
    size_t alloc_size = 0;
    alloc_size += CACHEALIGN_SIZE - 1;
    alloc_size += nio->block_size * 2;

    nio->alloc_handle = core_alloc_ex("xf_nandio", alloc_size, &buflib_ops_locked);
    if(nio->alloc_handle < 0) {
        rc = XF_E_OUT_OF_MEMORY;
        goto out_nclose;
    }

    uint8_t* buffer = core_get_data(nio->alloc_handle);
    CACHEALIGN_BUFFER(buffer, alloc_size);

    nio->old_buf = buffer;
    nio->new_buf = &buffer[nio->block_size];

    rc = XF_E_SUCCESS;
    goto out;

  out_nclose:
    nand_close(nio->ndrv);
  out:
    nand_unlock(nio->ndrv);
    return rc;
}

void xf_nandio_destroy(struct xf_nandio* nio)
{
    if(nio->alloc_handle > 0) {
        core_free(nio->alloc_handle);
        nio->alloc_handle = 0;
    }

    if(nio->ndrv) {
        nand_lock(nio->ndrv);
        nand_close(nio->ndrv);
        nand_unlock(nio->ndrv);
        nio->ndrv = NULL;
    }
}

static bool is_page_blank(const uint8_t* buf, uint32_t length)
{
    for(uint32_t i = 0; i < length; ++i)
        if(buf[i] != 0xff)
            return false;

    return true;
}

static int flush_block(struct xf_nandio* nio, bool invalidate)
{
    /* no block, or only reading - flush is a no-op */
    if(!nio->block_valid || nio->mode == XF_NANDIO_READ)
        return XF_E_SUCCESS;

    /* nothing to do if new data is same as old data */
    if(!memcmp(nio->old_buf, nio->new_buf, nio->block_size))
        return XF_E_SUCCESS;

    /* data mismatch during verification - report the error */
    if(nio->mode == XF_NANDIO_VERIFY)
        return XF_E_VERIFY_FAILED;

    /* erase the block */
    int rc = nand_block_erase(nio->ndrv, nio->cur_block);
    if(rc != NAND_SUCCESS) {
        nio->block_valid = false;
        nio->nand_err = rc;
        return XF_E_NAND;
    }

    size_t oob_size = nio->ndrv->chip->oob_size;

    unsigned page = 0;
    nand_page_t page_addr = nio->cur_block;
    for(; page < nio->ndrv->ppb; ++page, ++page_addr) {
        /* skip programming blank pages to go faster & reduce wear */
        uint8_t* page_data = &nio->new_buf[page * nio->page_size];
        if(is_page_blank(page_data, nio->page_size))
            continue;

        /* copy page and write blank OOB data */
        memcpy(nio->ndrv->page_buf, page_data, nio->page_size);
        memset(&nio->ndrv->page_buf[nio->page_size], 0xff, oob_size);

        /* program the page */
        rc = nand_page_program(nio->ndrv, page_addr, nio->ndrv->page_buf);
        if(rc != NAND_SUCCESS) {
            nio->block_valid = false;
            nio->nand_err = rc;
            return XF_E_NAND;
        }
    }

    if(invalidate)
        nio->block_valid = false;
    else {
        /* update our 'old' buffer so a subsequent flush
         * will not reprogram the same block */
        memcpy(nio->old_buf, nio->new_buf, nio->block_size);
    }

    return XF_E_SUCCESS;
}

static int seek_to_block(struct xf_nandio* nio, nand_block_t block_addr,
                         size_t offset_in_block)
{
    /* already on this block? */
    if(nio->block_valid && block_addr == nio->cur_block) {
        nio->offset_in_block = offset_in_block;
        return XF_E_SUCCESS;
    }

    /* ensure new block is within range */
    if(block_addr >= (nio->ndrv->chip->nr_blocks << nio->ndrv->chip->log2_ppb))
        return XF_E_OUT_OF_RANGE;

    /* flush old block */
    int rc = flush_block(nio, true);
    if(rc)
        return rc;

    nio->block_valid = false;

    /* read the new block */
    unsigned page = 0;
    nand_page_t page_addr = block_addr;
    for(; page < nio->ndrv->ppb; ++page, ++page_addr) {
        rc = nand_page_read(nio->ndrv, page_addr, nio->ndrv->page_buf);
        if(rc != NAND_SUCCESS) {
            nio->nand_err = rc;
            return XF_E_NAND;
        }

        memcpy(&nio->old_buf[page * nio->page_size], nio->ndrv->page_buf, nio->page_size);
    }

    /* copy to 2nd buffer */
    memcpy(nio->new_buf, nio->old_buf, nio->block_size);

    /* update position */
    nio->cur_block = block_addr;
    nio->offset_in_block = offset_in_block;
    nio->block_valid = true;
    return XF_E_SUCCESS;
}

int xf_nandio_set_mode(struct xf_nandio* nio, enum xf_nandio_mode mode)
{
    nand_lock(nio->ndrv);

    /* flush the current block before switching to the new mode,
     * to ensure consistency */
    int rc = flush_block(nio, false);
    if(rc)
        goto err;

    nio->mode = mode;
    rc = XF_E_SUCCESS;

  err:
    nand_unlock(nio->ndrv);
    return rc;
}

static int nandio_rdwr(struct xf_nandio* nio, void* buf, size_t count, bool write)
{
    while(count > 0) {
        void* ptr;
        size_t amount = count;
        int rc = xf_nandio_get_buffer(nio, &ptr, &amount);
        if(rc)
            return rc;

        if(write)
            memcpy(ptr, buf, amount);
        else
            memcpy(buf, ptr, amount);

        count -= amount;
    }

    return XF_E_SUCCESS;
}

int xf_nandio_seek(struct xf_nandio* nio, size_t offset)
{
    uint32_t block_nr = offset / nio->block_size;
    size_t offset_in_block = offset % nio->block_size;
    nand_block_t block_addr = block_nr << nio->ndrv->chip->log2_ppb;

    nand_lock(nio->ndrv);
    int rc = seek_to_block(nio, block_addr, offset_in_block);
    nand_unlock(nio->ndrv);

    return rc;
}

int xf_nandio_read(struct xf_nandio* nio, void* buf, size_t count)
{
    return nandio_rdwr(nio, buf, count, false);
}

int xf_nandio_write(struct xf_nandio* nio, const void* buf, size_t count)
{
    return nandio_rdwr(nio, (void*)buf, count, true);
}

int xf_nandio_get_buffer(struct xf_nandio* nio, void** buf, size_t* count)
{
    nand_lock(nio->ndrv);

    /* make sure the current block data is read in */
    int rc = seek_to_block(nio, nio->cur_block, nio->offset_in_block);
    if(rc)
        goto err;

    size_t amount_left = nio->block_size - nio->offset_in_block;
    if(amount_left == 0) {
        amount_left = nio->block_size;
        rc = seek_to_block(nio, nio->cur_block + nio->ndrv->ppb, 0);
        if(rc)
            goto err;
    }

    *buf = &nio->new_buf[nio->offset_in_block];
    *count = MIN(*count, amount_left);

    nio->offset_in_block += *count;
    rc = XF_E_SUCCESS;

  err:
    nand_unlock(nio->ndrv);
    return rc;
}

int xf_nandio_flush(struct xf_nandio* nio)
{
    nand_lock(nio->ndrv);
    int rc = flush_block(nio, false);
    nand_unlock(nio->ndrv);

    return rc;
}
