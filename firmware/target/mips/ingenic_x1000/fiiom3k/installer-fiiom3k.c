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

#include "installer.h"
#include "nand-x1000.h"
#include "core_alloc.h"
#include "file.h"

#define INSTALL_SUCCESS             0
#define ERR_FLASH_OPEN_FAILED       (-1)
#define ERR_FLASH_ENABLE_WP_FAILED  (-2)
#define ERR_FLASH_DISABLE_WP_FAILED (-3)
#define ERR_FLASH_ERASE_FAILED      (-4)
#define ERR_FLASH_WRITE_FAILED      (-5)
#define ERR_FLASH_READ_FAILED       (-6)
#define ERR_OUT_OF_MEMORY           (-7)
#define ERR_CANNOT_READ_FILE        (-8)
#define ERR_CANNOT_WRITE_FILE       (-9)
#define ERR_WRONG_SIZE              (-10)

#define BOOT_IMAGE_SIZE (128 * 1024)

static int install_from_buffer(const void* buf)
{
    if(nand_open())
        return ERR_FLASH_OPEN_FAILED;

    int status = INSTALL_SUCCESS;

    if(nand_enable_writes(true)) {
        status = ERR_FLASH_DISABLE_WP_FAILED;
        goto _exit;
    }

    if(nand_erase_bytes(0, BOOT_IMAGE_SIZE)) {
        status = ERR_FLASH_ERASE_FAILED;
        goto _exit;
    }

    if(nand_write_bytes(0, BOOT_IMAGE_SIZE, buf)) {
        status = ERR_FLASH_WRITE_FAILED;
        goto _exit;
    }

    if(nand_enable_writes(false)) {
        status = ERR_FLASH_ENABLE_WP_FAILED;
        goto _exit;
    }

  _exit:
    nand_close();
    return status;
}

static int dump_to_buffer(void* buf)
{
    if(nand_open())
        return ERR_FLASH_OPEN_FAILED;

    int status = INSTALL_SUCCESS;

    if(nand_read_bytes(0, BOOT_IMAGE_SIZE, buf)) {
        status = ERR_FLASH_READ_FAILED;
        goto _exit;
    }

  _exit:
    nand_close();
    return status;
}

int install_bootloader(const char* path)
{
    /* Allocate memory to hold image */
    int handle = core_alloc("boot_image", BOOT_IMAGE_SIZE);
    if(handle < 0)
        return ERR_OUT_OF_MEMORY;

    int status = INSTALL_SUCCESS;
    void* buffer = core_get_data(handle);

    /* Open the boot image */
    int fd = open(path, O_RDONLY);
    if(fd < 0) {
        status = ERR_CANNOT_READ_FILE;
        goto _exit;
    }

    /* Check file size */
    off_t fsize = filesize(fd);
    if(fsize != BOOT_IMAGE_SIZE) {
        status = ERR_WRONG_SIZE;
        goto _exit;
    }

    /* Read the file into the buffer */
    ssize_t cnt = read(fd, buffer, BOOT_IMAGE_SIZE);
    if(cnt != BOOT_IMAGE_SIZE) {
        status = ERR_CANNOT_READ_FILE;
        goto _exit;
    }

    /* Perform the installation */
    status = install_from_buffer(buffer);

  _exit:
    if(fd >= 0)
        close(fd);
    core_free(handle);
    return status;
}

/* Dump the current bootloader to a file */
int dump_bootloader(const char* path)
{
    /* Allocate memory to hold image */
    int handle = core_alloc("boot_image", BOOT_IMAGE_SIZE);
    if(handle < 0)
        return -1;

    /* Read data from flash */
    int fd = -1;
    void* buffer = core_get_data(handle);
    int status = dump_to_buffer(buffer);
    if(status)
        goto _exit;

    /* Open file */
    fd = open(path, O_CREAT|O_TRUNC|O_WRONLY);
    if(fd < 0) {
        status = ERR_CANNOT_WRITE_FILE;
        goto _exit;
    }

    /* Write data to file */
    ssize_t cnt = write(fd, buffer, BOOT_IMAGE_SIZE);
    if(cnt != BOOT_IMAGE_SIZE) {
        status = ERR_CANNOT_WRITE_FILE;
        goto _exit;
    }

  _exit:
    if(fd >= 0)
        close(fd);
    core_free(handle);
    return status;
}

const char* installer_strerror(int rc)
{
    switch(rc) {
    case INSTALL_SUCCESS:
        return "Success";
    case ERR_FLASH_OPEN_FAILED:
        return "Can't open flash device";
    case ERR_FLASH_ENABLE_WP_FAILED:
        return "Couldn't re-enable write protect";
    case ERR_FLASH_DISABLE_WP_FAILED:
        return "Can't disable write protect";
    case ERR_FLASH_ERASE_FAILED:
        return "Flash erase failed";
    case ERR_FLASH_WRITE_FAILED:
        return "Flash write error";
    case ERR_FLASH_READ_FAILED:
        return "Flash read error";
    case ERR_OUT_OF_MEMORY:
        return "Out of memory";
    case ERR_CANNOT_READ_FILE:
        return "Error reading file";
    case ERR_CANNOT_WRITE_FILE:
        return "Error writing file";
    case ERR_WRONG_SIZE:
        return "Wrong file size";
    default:
        return "Unknown error";
    }
}
