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

int install_boot(const char* srcfile)
{
    int rc;
    int fd = -1;
    int handle = -1;

    /* Allocate enough memory for aligned tarball and image buffers */
    size_t bufsize = IMAGE_SIZE + TAR_SIZE + 2*CACHEALIGN_SIZE;
    handle = core_alloc("boot_image", bufsize);
    if(handle < 0) {
        rc = INSTALL_ERR_OUT_OF_MEMORY;
        goto error;
    }

    /* Get buffer */
    uint8_t* buffer = core_get_data(handle);
    CACHEALIGN_BUFFER(buffer, bufsize);

    /* Setup buffer pointers */
    uint8_t* tarbuf = buffer;
    buffer += TAR_SIZE;
    bufsize -= TAR_SIZE;
    CACHEALIGN_BUFFER(buffer, bufsize);

    /* Read the tarball into tarbuf */
    fd = open(srcfile, O_RDONLY);
    if(fd < 0) {
        rc = INSTALL_ERR_FILE_NOT_FOUND;
        goto error;
    }

    off_t fsize = filesize(fd);
    if(fsize > TAR_SIZE) {
        rc = INSTALL_ERR_BAD_FORMAT;
        goto error;
    }

    ssize_t cnt = read(fd, tarbuf, fsize);
    if(cnt != fsize) {
        rc = INSTALL_ERR_FILE_IO;
        goto error;
    }

    /* We're done with the file */
    close(fd);
    fd = -1;

    /* Read the flash -- we need an existing image to patch */
    rc = flash_img_read(buffer);
    if(rc < 0)
        goto error;

    /* Walk the tarball & extract the needed files.
     * Based on untar() function in bootloader/gigabeat-s.c */
    int files_done = 0;
    char* ptr = (char*)tarbuf;
    char* endptr = ptr + cnt;
    char* header = NULL;
    do {
        /* Read header */
        header = ptr;
        ptr += 512;

        /* Check for EOF */
        if(header[0] == 0)
            break;

        /* Parse size field */
        ptrdiff_t size = 0;
        for(int i = 124; i < 124 + 11; ++i) {
            if(header[i] < '0' || header[i] > '9') {
                rc = INSTALL_ERR_BAD_FORMAT;
                goto error;
            }

            size = (8*size) + (header[i] - '0');
        }

        /* Sanity check */
        if(endptr - ptr < size) {
            rc = INSTALL_ERR_BAD_FORMAT;
            goto error;
        }

        /* The filename determines where to place the patch data
         * Unknown files are silently skipped, to allow for metadata files */
        int patch_offset = -1;
        int patch_size = 0;
        if(header[156] != '0') {
            /* Skip non-files */
        } else if(strncmp(header, "spl.m3k", 512) == 0) {
            patch_offset = 0;
            patch_size = 12 * 1024;
        } else if(strncmp(header, "bootloader.ucl", 512) == 0) {
            patch_offset = 0x6800;
            patch_size = 102 * 1024;
        }

        if(patch_offset >= 0) {
            memset(&buffer[patch_offset], 0xff, patch_size);
            memcpy(&buffer[patch_offset], ptr, size);
            ++files_done;
        }

        /* Move pointer to next entry */
        ptr += (size + 511) & ~511;
    } while(files_done < 2 && ptr < endptr);

    /* Make sure we got all the files */
    if(files_done != 2) {
        rc = INSTALL_ERR_BAD_FORMAT;
        goto error;
    }

    /* Flash the new image */
    rc = flash_img_write(buffer);
    if(rc < 0)
        goto error;

    rc = INSTALL_SUCCESS;

  error:
    if(fd >= 0)
        close(fd);
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

static const char* nand_strerror(int rc_nand)
{
    switch(rc_nand) {
    case NAND_SUCCESS:
        return "Success";
    case NAND_ERR_UNKNOWN_CHIP:
        return "Unknown chip";
    case NAND_ERR_UNALIGNED:
        return "Unaligned arg";
    case NAND_ERR_WRITE_PROTECT:
        return "Write protected";
    case NAND_ERR_CONTROLLER:
        return "Controller error";
    case NAND_ERR_COMMAND:
        return "Command error";
    default:
        return "Unknown error";
    }
}

const char* install_strerror(int rc, char* buf, size_t bufsize)
{
    /* handle INSTALL_ERR_FLASH codes */
    if(rc <= -10000) {
        int rc_nand = rc % 100;
        int rc_step = (rc / 100) % 100;
        const char* nand_err = nand_strerror(rc_nand);
        const char* our_err = install_strerror(rc_step, NULL, 0);
        snprintf(buf, bufsize, "%s: %s", our_err, nand_err);
        buf[bufsize-1] = 0; /* ensure null termination */
        return buf;
    }

    /* all other error codes */
    switch(rc) {
    case INSTALL_SUCCESS:
        return "Success";
    case INSTALL_ERR_OUT_OF_MEMORY:
        return "Not enough memory";
    case INSTALL_ERR_FILE_NOT_FOUND:
        return "File not found";
    case INSTALL_ERR_FILE_IO:
        return "File I/O error";
    case INSTALL_ERR_BAD_FORMAT:
        return "Bad file format";
    case INSTALL_ERR_NAND_OPEN:
        return "open";
    case INSTALL_ERR_NAND_IDENTIFY:
        return "identify";
    case INSTALL_ERR_NAND_READ:
        return "read";
    case INSTALL_ERR_NAND_ENABLE_WRITES:
        return "enable_writes";
    case INSTALL_ERR_NAND_ERASE:
        return "erase";
    case INSTALL_ERR_NAND_WRITE:
        return "write";
    default:
        return "Unknown error";
    }
}
