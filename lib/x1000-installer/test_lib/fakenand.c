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
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

const char* nand_backing_file = "fakeNAND.bin";
const char* nand_meta_file = "fakeNAND_meta.bin";

struct nand_trace* nand_trace = NULL;
size_t nand_trace_capacity = 0;
size_t nand_trace_length = 0;

static struct nand_trace* nand_trace_cur = NULL;

static int injected_err = 0;

#define METAF_PROGRAMMED 1

static const nand_chip fake_chip = {
    /* ATO25D1GA */
    .log2_ppb = 6,      /* 64 pages */
    .page_size = 2048,
    .oob_size = 64,
    .nr_blocks = 1024,
};

void nand_trace_reset(size_t size)
{
    nand_trace = realloc(nand_trace, size);
    nand_trace_capacity = size;
    nand_trace_length = 0;
    nand_trace_cur = nand_trace;
}

void nand_inject_error(int rc)
{
    injected_err = rc;
}

nand_drv* nand_init(void)
{
    static bool inited = false;
    static uint8_t scratch_buf[NAND_DRV_SCRATCHSIZE];
    static uint8_t page_buf[NAND_DRV_MAXPAGESIZE];
    static nand_drv d;

    if(!inited) {
        d.scratch_buf = scratch_buf;
        d.page_buf = page_buf;
        d.chip = &fake_chip;
        inited = true;
    }

    return &d;
}

static void lock_assert(bool cond, const char* msg)
{
    if(!cond) {
        fprintf(stderr, "%s\n", msg);
        fflush(stderr);
        abort();
    }
}

void nand_lock(nand_drv* drv)
{
    drv->lock_count++;
}

void nand_unlock(nand_drv* drv)
{
    lock_assert(drv->lock_count > 0, "nand_unlock() called when not locked");
    drv->lock_count--;
}

#define CHECK_INJECTED_ERROR \
    do { int __err = injected_err; injected_err = 0; if(__err) return __err; } while(0)

int nand_open(nand_drv* drv)
{
    lock_assert(drv->lock_count > 0, "nand_open(): lock not held");
    CHECK_INJECTED_ERROR;

    if(drv->refcount > 0) {
        drv->refcount++;
        return NAND_SUCCESS;
    }

    /* leaks an fd on error but this is only testing... */
    drv->fd = open(nand_backing_file, O_RDWR|O_CREAT, 0644);
    drv->metafd = open(nand_meta_file, O_RDWR|O_CREAT, 0644);
    if(drv->fd < 0 || drv->metafd < 0)
        goto err;

    drv->ppb = 1 << drv->chip->log2_ppb;
    drv->fpage_size = drv->chip->page_size + drv->chip->oob_size;

    /* make backing file the correct size */
    if(ftruncate(drv->fd, drv->chip->page_size * drv->ppb * drv->chip->nr_blocks) < 0)
        goto err;
    if(ftruncate(drv->metafd, drv->chip->nr_blocks * drv->ppb) < 0)
        goto err;

    drv->refcount++;
    return NAND_SUCCESS;

  err:
    if(drv->fd >= 0)
        close(drv->fd);
    if(drv->metafd >= 0)
        close(drv->metafd);
    return NAND_ERR_OTHER;
}

void nand_close(nand_drv* drv)
{
    lock_assert(drv->lock_count > 0, "nand_close(): lock not held");

    if(--drv->refcount > 0)
        return;

    close(drv->fd);
    close(drv->metafd);
    drv->fd = -1;
    drv->metafd = -1;
}

static int read_meta(nand_drv* drv, nand_page_t page)
{
    /* probably won't fail */
    if(lseek(drv->metafd, page, SEEK_SET) < 0)
        return NAND_ERR_OTHER;
    if(read(drv->metafd, drv->scratch_buf, 1) != 1)
        return NAND_ERR_OTHER;

    return drv->scratch_buf[0];
}

static int write_meta(nand_drv* drv, nand_page_t page, int val)
{
    drv->scratch_buf[0] = val;

    if(lseek(drv->metafd, page, SEEK_SET) < 0)
        return NAND_ERR_OTHER;
    if(write(drv->metafd, drv->scratch_buf, 1) != 1)
        return NAND_ERR_OTHER;

    return NAND_SUCCESS;
}

static int upd_meta(nand_drv* drv, nand_page_t page, uint8_t clr, uint8_t set)
{
    int meta = read_meta(drv, page);
    if(meta < 0)
        return meta;

    meta &= ~clr;
    meta |= set;

    return write_meta(drv, page, meta);
}

static int page_program(nand_drv* drv, nand_page_t page, const void* buffer,
                        uint8_t clr, uint8_t set)
{
    if(lseek(drv->fd, page * drv->chip->page_size, SEEK_SET) < 0)
        return NAND_ERR_OTHER;
    if(write(drv->fd, buffer, drv->chip->page_size) != (ssize_t)drv->chip->page_size)
        return NAND_ERR_PROGRAM_FAIL;

    return upd_meta(drv, page, clr, set);
}

static void trace(enum nand_trace_type ty, enum nand_trace_exception ex, nand_page_t addr)
{
    if(nand_trace_length < nand_trace_capacity) {
        nand_trace_cur->type = ty;
        nand_trace_cur->exception = ex;
        nand_trace_cur->addr = addr;
        nand_trace_cur++;
        nand_trace_length++;
    }
}

int nand_block_erase(nand_drv* drv, nand_block_t block)
{
    lock_assert(drv->lock_count > 0, "nand_block_erase(): lock not held");
    CHECK_INJECTED_ERROR;

    trace(NTT_ERASE, NTE_NONE, block);

    memset(drv->page_buf, 0xff, drv->fpage_size);

    for(unsigned i = 0; i < drv->ppb; ++i) {
        int rc = page_program(drv, block + i, drv->page_buf, METAF_PROGRAMMED, 0);
        if(rc < 0)
            return NAND_ERR_ERASE_FAIL;
    }

    return NAND_SUCCESS;
}

int nand_page_program(nand_drv* drv, nand_page_t page, const void* buffer)
{
    lock_assert(drv->lock_count > 0, "nand_page_program(): lock not held");
    CHECK_INJECTED_ERROR;

    int meta = read_meta(drv, page);
    if(meta < 0)
        return meta;

    enum nand_trace_exception exception = NTE_NONE;
    if(meta & METAF_PROGRAMMED)
        exception = NTE_DOUBLE_PROGRAMMED;

    trace(NTT_PROGRAM, exception, page);

    return page_program(drv, page, buffer, 0, METAF_PROGRAMMED);
}

int nand_page_read(nand_drv* drv, nand_page_t page, void* buffer)
{
    lock_assert(drv->lock_count > 0, "nand_page_read(): lock not held");
    CHECK_INJECTED_ERROR;

    enum nand_trace_exception exception = NTE_NONE;

    int meta = read_meta(drv, page);
    if(meta < 0)
        return meta;

    if(meta & METAF_PROGRAMMED) {
        if(lseek(drv->fd, page * drv->chip->page_size, SEEK_SET) < 0)
            return NAND_ERR_OTHER;
        if(read(drv->fd, buffer, drv->chip->page_size) != (ssize_t)drv->chip->page_size)
            return NAND_ERR_OTHER;
    } else {
        memset(buffer, 0xff, drv->chip->page_size);
        exception = NTE_CLEARED;
    }

    trace(NTT_READ, exception, page);

    memset(buffer + drv->chip->page_size, 0xff, drv->chip->oob_size);
    return NAND_SUCCESS;
}
