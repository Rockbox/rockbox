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

#include "jztool_private.h"
#include "../../../firmware/target/mips/ingenic_x1000/spl-x1000-defs.h"
#include "../../../firmware/target/mips/ingenic_x1000/nand-x1000-err.h"
#include <endian.h> // TODO: portability
#include <string.h>

static const char* jz_x1000_nand_strerror(int rc)
{
    switch(rc) {
    case NANDERR_CHIP_UNSUPPORTED:
        return "Chip unsupported";
    case NANDERR_WRITE_PROTECTED:
        return "Operation forbidden by write-protect";
    case NANDERR_UNALIGNED_ADDRESS:
        return "Improperly aligned address";
    case NANDERR_UNALIGNED_LENGTH:
        return "Improperly aligned length";
    case NANDERR_READ_FAILED:
        return "Read operation failed";
    case NANDERR_ECC_FAILED:
        return "Uncorrectable ECC error on read";
    case NANDERR_ERASE_FAILED:
        return "Erase operation failed";
    case NANDERR_PROGRAM_FAILED:
        return "Program operation failed";
    case NANDERR_COMMAND_FAILED:
        return "NAND command failed";
    default:
        return "Unknown NAND error";
    }
}


static int jz_x1000_send_args(jz_usbdev* dev, struct x1000_spl_arguments* args)
{
    args->command = htole32(args->command);
    args->param1 = htole32(args->param1);
    args->param2 = htole32(args->param2);
    args->flags = htole32(args->flags);
    return jz_usb_send(dev, SPL_ARGUMENTS_ADDRESS, sizeof(*args), args);
}

static int jz_x1000_recv_status(jz_usbdev* dev, struct x1000_spl_status* status)
{
    int rc = jz_usb_recv(dev, SPL_STATUS_ADDRESS, sizeof(*status), status);
    if(rc < 0)
        return rc;

    status->err_code = le32toh(status->err_code);
    status->reserved = le32toh(status->reserved);
    return JZ_SUCCESS;
}

int jz_x1000_setup(jz_usbdev* dev, size_t spl_len, const void* spl_data)
{
    int rc = jz_identify_x1000_spl(spl_data, spl_len);
    if(rc < 0)
        return JZ_ERR_BAD_FILE_FORMAT;
    if(spl_len > SPL_MAX_SIZE)
        return JZ_ERR_BAD_FILE_FORMAT;

    rc = jz_usb_send(dev, SPL_LOAD_ADDRESS, spl_len, spl_data);
    if(rc < 0)
        return rc;

    struct x1000_spl_arguments args;
    args.command = SPL_CMD_BOOT;
    args.param1 = SPL_BOOTOPT_NONE;
    args.param2 = 0;
    args.flags = 0;
    rc = jz_x1000_send_args(dev, &args);
    if(rc < 0)
        return rc;

    rc = jz_usb_start1(dev, SPL_EXEC_ADDRESS);
    if(rc < 0)
        return rc;

    jz_sleepms(100);

    struct x1000_spl_status status;
    rc = jz_x1000_recv_status(dev, &status);
    if(rc < 0)
        return rc;

    if(status.err_code != 0) {
        jz_log(dev->jz, JZ_LOG_ERROR, "X1000 device init error: %d", status.err_code);
        return JZ_ERR_OTHER;
    }

    return JZ_SUCCESS;
}

int jz_x1000_read_flash(jz_usbdev* dev, uint32_t addr, size_t len, void* data)
{
    struct x1000_spl_arguments args;
    args.command = SPL_CMD_FLASH_READ;
    args.param1 = addr;
    args.param2 = len;
    args.flags = SPL_FLAG_SKIP_INIT;
    int rc = jz_x1000_send_args(dev, &args);
    if(rc < 0)
        return rc;

    rc = jz_usb_start1(dev, SPL_EXEC_ADDRESS);
    if(rc < 0)
        return rc;

    jz_sleepms(500);

    struct x1000_spl_status status;
    rc = jz_x1000_recv_status(dev, &status);
    if(rc < 0)
        return rc;

    if(status.err_code != 0) {
        jz_log(dev->jz, JZ_LOG_ERROR, "X1000 flash read error: %s",
               jz_x1000_nand_strerror(status.err_code));
        return JZ_ERR_FLASH_ERROR;
    }

    return jz_usb_recv(dev, SPL_BUFFER_ADDRESS, len, data);
}

int jz_x1000_write_flash(jz_usbdev* dev, uint32_t addr, size_t len, const void* data)
{
    int rc = jz_usb_send(dev, SPL_BUFFER_ADDRESS, len, data);
    if(rc < 0)
        return rc;

    struct x1000_spl_arguments args;
    args.command = SPL_CMD_FLASH_WRITE;
    args.param1 = addr;
    args.param2 = len;
    args.flags = SPL_FLAG_SKIP_INIT;
    rc = jz_x1000_send_args(dev, &args);
    if(rc < 0)
        return rc;

    rc = jz_usb_start1(dev, SPL_EXEC_ADDRESS);
    if(rc < 0)
        return rc;

    jz_sleepms(500);

    struct x1000_spl_status status;
    rc = jz_x1000_recv_status(dev, &status);
    if(rc < 0)
        return rc;

    if(status.err_code != 0) {
        jz_log(dev->jz, JZ_LOG_ERROR, "X1000 flash write error: %s",
               jz_x1000_nand_strerror(status.err_code));
        return JZ_ERR_FLASH_ERROR;
    }

    return JZ_SUCCESS;
}

int jz_x1000_boot_rockbox(jz_usbdev* dev)
{
    struct x1000_spl_arguments args;
    args.command = SPL_CMD_BOOT;
    args.param1 = SPL_BOOTOPT_ROCKBOX;
    args.param2 = 0;
    args.flags = 0;
    int rc = jz_x1000_send_args(dev, &args);
    if(rc < 0)
        return rc;

    return jz_usb_start1(dev, SPL_EXEC_ADDRESS);
}
