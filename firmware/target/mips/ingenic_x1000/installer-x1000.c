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

#include "installer-x1000.h"
#include "nand-x1000.h"
#include "core_alloc.h"
#include "file.h"
#include "microtar-rockbox.h"
#include <stddef.h>

struct update_part {
    const char* filename;
    size_t offset;
    size_t length;
};

/* Parts of the flash to update. The offset and length are given in bytes,
 * offset relative to start of flash. The region's new contents are given
 * by the named file inside the update archive. If any file is missing, the
 * update will fail. (gracefully! nothing is written unless the package has
 * all its components)
 *
 * If the update file is smaller than the region size, unused space at the
 * end of the region is padded with 0xff.
 *
 * NOTE: The current code assumes all parts are contiguous. The current
 * update map fits in one eraseblock, but if it ever needs extending beyond
 * that, better implement a bitmap to indicate which blocks need updating
 * and which can be skipped. We don't want to erase and reprogram blocks
 * for no good reason, it's bad for the flash lifespan.
 */
static const struct update_part updates[] = {
    {
        .filename = "spl." BOOTFILE_EXT,
        .offset = 0,
        .length = 12 * 1024,
    },
    {
        .filename = "bootloader.ucl",
        .offset = 0x6800,
        .length = 102 * 1024,
    },
};

static const int num_updates = sizeof(updates) / sizeof(struct update_part);

/* calculate the offset and length of the update image; this is constant
 * for a given target, based on the update parts and the NAND chip geometry.
 */
static void get_image_loc(nand_drv* ndrv, size_t* offptr, size_t* lenptr)
{
    size_t blk_size = ndrv->chip->page_size << ndrv->chip->log2_ppb;
    size_t img_off = 0;
    size_t img_len = 0;

    /* calculate minimal image needed to contain all update blocks */
    for(int i = 0; i < num_updates; ++i) {
        img_len = MAX(img_len, updates[i].offset + updates[i].length);
        img_off = MIN(img_off, updates[i].offset);
    }

    /* round everything to a multiple of the block size */
    size_t r_off = blk_size * (img_off / blk_size);
    size_t r_len = blk_size * ((img_len + img_off - r_off + blk_size - 1) / blk_size);

    *offptr = r_off;
    *lenptr = r_len;
}

/* Read in a single part of the update from the tarball, and patch it
 * into the image */
static int patch_part(mtar_t* tar, const struct update_part* part,
                      uint8_t* img_buf, size_t img_off)
{
    int rc = mtar_find(tar, part->filename);
    if(rc != MTAR_ESUCCESS)
        return IERR_BAD_FORMAT;

    const mtar_header_t* h = mtar_get_header(tar);
    if(h->type != 0 && h->type != MTAR_TREG)
        return IERR_BAD_FORMAT;

    if(h->size > part->length)
        return IERR_BAD_FORMAT;

    /* wipe the patched area, and read in the new data */
    memset(&img_buf[part->offset - img_off], 0xff, part->length);
    rc = mtar_read_data(tar, &img_buf[part->offset - img_off], h->size);
    if(rc < 0 || (unsigned)rc != h->size)
        return IERR_FILE_IO;

    return IERR_SUCCESS;
}

struct updater {
    int buf_hnd; /* core_alloc handle for our memory buffer */
    size_t buf_len; /* sizeof the buffer */

    uint8_t* img_buf;
    size_t img_off; /* image address in flash */
    size_t img_len; /* image length in flash = size of the buffer */

    mtar_t* tar;
    nand_drv* ndrv;
};

static int updater_init(struct updater* u)
{
    int rc;

    /* initialize stuff correctly */
    u->buf_hnd = -1;
    u->buf_len = 0;
    u->img_buf = NULL;
    u->img_off = 0;
    u->img_len = 0;
    u->tar = NULL;
    u->ndrv = NULL;

    /* open NAND */
    u->ndrv = nand_init();
    nand_lock(u->ndrv);
    rc = nand_open(u->ndrv);
    if(rc != NAND_SUCCESS) {
        rc = IERR_NAND_OPEN;
        goto error;
    }

    get_image_loc(u->ndrv, &u->img_off, &u->img_len);

    /* buf_len is a bit oversized here, but it's not really important */
    u->buf_len = u->img_len + sizeof(mtar_t) + 2*CACHEALIGN_SIZE;
    u->buf_hnd = core_alloc_ex("boot_image", u->buf_len, &buflib_ops_locked);
    if(u->buf_hnd < 0) {
        rc = IERR_OUT_OF_MEMORY;
        goto error;
    }

    /* allocate from the buffer */
    uint8_t* buffer = (uint8_t*)core_get_data(u->buf_hnd);
    size_t buf_len = u->buf_len;

    CACHEALIGN_BUFFER(buffer, buf_len);
    u->img_buf = buffer;
    buffer += u->img_len;
    buf_len -= u->img_len;

    CACHEALIGN_BUFFER(buffer, buf_len);
    u->tar = (mtar_t*)buffer;
    memset(u->tar, 0, sizeof(mtar_t));

    rc = IERR_SUCCESS;

  error:
    return rc;
}

static void updater_cleanup(struct updater* u)
{
    if(u->tar && mtar_is_open(u->tar))
        mtar_close(u->tar);

    if(u->buf_hnd >= 0)
        core_free(u->buf_hnd);

    if(u->ndrv) {
        nand_close(u->ndrv);
        nand_unlock(u->ndrv);
    }
}

int install_bootloader(const char* filename)
{
    struct updater u;
    int rc = updater_init(&u);
    if(rc != IERR_SUCCESS)
        goto error;

    /* get the image */
    rc = nand_read_bytes(u.ndrv, u.img_off, u.img_len, u.img_buf);
    if(rc != NAND_SUCCESS) {
        rc = IERR_NAND_READ;
        goto error;
    }

    /* get the tarball */
    rc = mtar_open(u.tar, filename, O_RDONLY);
    if(rc != MTAR_ESUCCESS) {
        if(rc == MTAR_EOPENFAIL)
            rc = IERR_FILE_NOT_FOUND;
        else if(rc == MTAR_EREADFAIL)
            rc = IERR_FILE_IO;
        else
            rc = IERR_BAD_FORMAT;
        goto error;
    }

    /* patch stuff */
    for(int i = 0; i < num_updates; ++i) {
        rc = patch_part(u.tar, &updates[i], u.img_buf, u.img_off);
        if(rc != IERR_SUCCESS)
            goto error;
    }

    /* write back the patched image */
    rc = nand_write_bytes(u.ndrv, u.img_off, u.img_len, u.img_buf);
    if(rc != NAND_SUCCESS) {
        rc = IERR_NAND_WRITE;
        goto error;
    }

    rc = IERR_SUCCESS;

  error:
    updater_cleanup(&u);
    return rc;
}

int backup_bootloader(const char* filename)
{
    int rc, fd = 0;
    struct updater u;

    rc = updater_init(&u);
    if(rc != IERR_SUCCESS)
        goto error;

    /* read image */
    rc = nand_read_bytes(u.ndrv, u.img_off, u.img_len, u.img_buf);
    if(rc != NAND_SUCCESS) {
        rc = IERR_NAND_READ;
        goto error;
    }

    /* write to file */
    fd = open(filename, O_CREAT|O_TRUNC|O_WRONLY);
    if(fd < 0) {
        rc = IERR_FILE_IO;
        goto error;
    }

    ssize_t cnt = write(fd, u.img_buf, u.img_len);
    if(cnt < 0 || (size_t)cnt != u.img_len) {
        rc = IERR_FILE_IO;
        goto error;
    }

    rc = IERR_SUCCESS;

  error:
    if(fd >= 0)
        close(fd);
    updater_cleanup(&u);
    return rc;
}

int restore_bootloader(const char* filename)
{
    int rc, fd = 0;
    struct updater u;

    rc = updater_init(&u);
    if(rc != IERR_SUCCESS)
        goto error;

    /* read from file */
    fd = open(filename, O_RDONLY);
    if(fd < 0) {
        rc = IERR_FILE_NOT_FOUND;
        goto error;
    }

    ssize_t cnt = read(fd, u.img_buf, u.img_len);
    if(cnt < 0 || (size_t)cnt != u.img_len) {
        rc = IERR_FILE_IO;
        goto error;
    }

    /* write image */
    rc = nand_write_bytes(u.ndrv, u.img_off, u.img_len, u.img_buf);
    if(rc != NAND_SUCCESS) {
        rc = IERR_NAND_WRITE;
        goto error;
    }

    rc = IERR_SUCCESS;

  error:
    if(fd >= 0)
        close(fd);
    updater_cleanup(&u);
    return rc;
}

const char* installer_strerror(int rc)
{
    switch(rc) {
    case IERR_SUCCESS:          return "Success";
    case IERR_OUT_OF_MEMORY:    return "Out of memory";
    case IERR_FILE_NOT_FOUND:   return "File not found";
    case IERR_FILE_IO:          return "Disk I/O error";
    case IERR_BAD_FORMAT:       return "Bad archive";
    case IERR_NAND_OPEN:        return "NAND open error";
    case IERR_NAND_READ:        return "NAND read error";
    case IERR_NAND_WRITE:       return "NAND write error";
    default:                    return "Unknown error!?";
    }
}
