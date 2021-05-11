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

#include "installer-fiiom3k.h"
#include "nand-x1000.h"
#include "system.h"
#include "core_alloc.h"
#include "file.h"
#include "microtar.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define IMAGE_SIZE  (128 * 1024)
#define TAR_SIZE    (256 * 1024)

static int flash_prepare(void)
{
    int mf_id, dev_id;
    int rc;

    rc = nand_open();
    if(rc < 0)
        return INSTALL_ERR_FLASH(NAND_OPEN, rc);

    rc = nand_identify(&mf_id, &dev_id);
    if(rc < 0) {
        nand_close();
        return INSTALL_ERR_FLASH(NAND_IDENTIFY, rc);
    }

    return INSTALL_SUCCESS;
}

static void flash_finish(void)
{
    /* Ensure writes are always disabled when we finish.
     * Errors are safe to ignore here, there's nothing we could do anyway. */
    nand_enable_writes(false);
    nand_close();
}

static int flash_img_read(uint8_t* buffer)
{
    int rc = flash_prepare();
    if(rc < 0)
        goto error;

    rc = nand_read(0, IMAGE_SIZE, buffer);
    if(rc < 0) {
        rc = INSTALL_ERR_FLASH(NAND_READ, rc);
        goto error;
    }

  error:
    flash_finish();
    return rc;
}

static int flash_img_write(const uint8_t* buffer)
{
    int rc = flash_prepare();
    if(rc < 0)
        goto error;

    rc = nand_enable_writes(true);
    if(rc < 0) {
        rc = INSTALL_ERR_FLASH(NAND_ENABLE_WRITES, rc);
        goto error;
    }

    rc = nand_erase(0, IMAGE_SIZE);
    if(rc < 0) {
        rc = INSTALL_ERR_FLASH(NAND_ERASE, rc);
        goto error;
    }

    rc = nand_write(0, IMAGE_SIZE, buffer);
    if(rc < 0) {
        rc = INSTALL_ERR_FLASH(NAND_WRITE, rc);
        goto error;
    }

  error:
    flash_finish();
    return rc;
}

static int patch_img(mtar_t* tar, uint8_t* buffer, const char* filename,
                     size_t patch_offset, size_t patch_size)
{
    /* Seek to file */
    mtar_header_t h;
    int rc = mtar_find(tar, filename, &h);
    if(rc != MTAR_ESUCCESS) {
        rc = INSTALL_ERR_MTAR(TAR_FIND, rc);
        return rc;
    }

    /* We need a normal file */
    if(h.type != 0 && h.type != MTAR_TREG)
        return INSTALL_ERR_BAD_FORMAT;

    /* Check size does not exceed patch area */
    if(h.size > patch_size)
        return INSTALL_ERR_BAD_FORMAT;

    /* Read data directly into patch area, fill unused bytes with 0xff */
    memset(&buffer[patch_offset], 0xff, patch_size);
    rc = mtar_read_data(tar, &buffer[patch_offset], h.size);
    if(rc != MTAR_ESUCCESS) {
        rc = INSTALL_ERR_MTAR(TAR_READ, rc);
        return rc;
    }

    return INSTALL_SUCCESS;
}

int install_boot(const char* srcfile)
{
    int rc;
    mtar_t* tar = NULL;
    int handle = -1;

    /* Allocate enough memory for image and tar state */
    size_t bufsize = IMAGE_SIZE + sizeof(mtar_t) + 2*CACHEALIGN_SIZE;
    handle = core_alloc("boot_image", bufsize);
    if(handle < 0) {
        rc = INSTALL_ERR_OUT_OF_MEMORY;
        goto error;
    }

    uint8_t* buffer = core_get_data(handle);

    /* Tar state alloc */
    CACHEALIGN_BUFFER(buffer, bufsize);
    tar = (mtar_t*)buffer;
    memset(tar, 0, sizeof(tar));

    /* Image buffer alloc */
    buffer += sizeof(mtar_t);
    CACHEALIGN_BUFFER(buffer, bufsize);

    /* Read the flash -- we need an existing image to patch */
    rc = flash_img_read(buffer);
    if(rc < 0)
        goto error;

    /* Open the tarball */
    rc = mtar_open(tar, srcfile, "r");
    if(rc != MTAR_ESUCCESS) {
        rc = INSTALL_ERR_MTAR(TAR_OPEN, rc);
        goto error;
    }

    /* Extract the needed files & patch 'em in */
    rc = patch_img(tar, buffer, "spl.m3k", 0, 12 * 1024);
    if(rc < 0)
        goto error;

    rc = patch_img(tar, buffer, "bootloader.ucl", 0x6800, 102 * 1024);
    if(rc < 0)
        goto error;

    /* Flash the new image */
    rc = flash_img_write(buffer);
    if(rc < 0)
        goto error;

    rc = INSTALL_SUCCESS;

  error:
    if(tar && tar->close)
        mtar_close(tar);
    if(handle >= 0)
        core_free(handle);
    return rc;
}

int backup_boot(const char* destfile)
{
    int rc;
    int handle = -1;
    int fd = -1;
    size_t bufsize = IMAGE_SIZE + CACHEALIGN_SIZE - 1;
    handle = core_alloc("boot_image", bufsize);
    if(handle < 0) {
        rc = INSTALL_ERR_OUT_OF_MEMORY;
        goto error;
    }

    uint8_t* buffer = core_get_data(handle);
    CACHEALIGN_BUFFER(buffer, bufsize);

    rc = flash_img_read(buffer);
    if(rc < 0)
        goto error;

    fd = open(destfile, O_CREAT|O_TRUNC|O_WRONLY);
    if(fd < 0) {
        rc = INSTALL_ERR_FILE_IO;
        goto error;
    }

    ssize_t cnt = write(fd, buffer, IMAGE_SIZE);
    if(cnt != IMAGE_SIZE) {
        rc = INSTALL_ERR_FILE_IO;
        goto error;
    }

  error:
    if(fd >= 0)
        close(fd);
    if(handle >= 0)
        core_free(handle);
    return rc;
}

int restore_boot(const char* srcfile)
{
    int rc;
    int handle = -1;
    int fd = -1;
    size_t bufsize = IMAGE_SIZE + CACHEALIGN_SIZE - 1;
    handle = core_alloc("boot_image", bufsize);
    if(handle < 0) {
        rc = INSTALL_ERR_OUT_OF_MEMORY;
        goto error;
    }

    uint8_t* buffer = core_get_data(handle);
    CACHEALIGN_BUFFER(buffer, bufsize);

    fd = open(srcfile, O_RDONLY);
    if(fd < 0) {
        rc = INSTALL_ERR_FILE_NOT_FOUND;
        goto error;
    }

    off_t fsize = filesize(fd);
    if(fsize != IMAGE_SIZE) {
        rc = INSTALL_ERR_BAD_FORMAT;
        goto error;
    }

    ssize_t cnt = read(fd, buffer, IMAGE_SIZE);
    if(cnt != IMAGE_SIZE) {
        rc = INSTALL_ERR_FILE_IO;
        goto error;
    }

    close(fd);
    fd = -1;

    rc = flash_img_write(buffer);
    if(rc < 0)
        goto error;

  error:
    if(fd >= 0)
        close(fd);
    if(handle >= 0)
        core_free(handle);
    return rc;
}
