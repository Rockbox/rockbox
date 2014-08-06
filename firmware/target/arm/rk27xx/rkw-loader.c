/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2012 Marcin Bukat
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 1
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include "config.h"
#include "loader_strerror.h"
#include "rkw-loader.h"
#include "crc32-rkw.h"
#include "file.h"
#include "panic.h"

/* loosely based on load_firmware()
 * on success we return size of loaded image
 * on error we return negative value which can be deciphered by means
 * of rkw_strerror() function
 */
int load_rkw(unsigned char* buf, const char* firmware, int buffer_size)
{
    char filename[MAX_PATH];
    int fd;
    int rc;
    int len;
    int ret;
    uint32_t crc, fw_crc;
    struct rkw_header_t rkw_info;

    /* only filename passed */
    if (firmware[0] != '/')
    {
        /* First check in BOOTDIR */
        snprintf(filename, sizeof(filename), BOOTDIR "/%s",firmware);

        fd = open(filename, O_RDONLY);
        if(fd < 0)
        {
            /* Check in root dir */
            snprintf(filename, sizeof(filename),"/%s",firmware);
            fd = open(filename, O_RDONLY);

            if (fd < 0)
                return EFILE_NOT_FOUND;
        }
    }
    else
    {
        /* full path passed */
        fd = open(firmware, O_RDONLY);
        if (fd < 0)
            return EFILE_NOT_FOUND;
    }

    rc = read(fd, &rkw_info, sizeof(rkw_info));
    if (rc < (int)sizeof(rkw_info))
    {
        ret = EREAD_HEADER_FAILED;
        goto end;
    }

    /* check if RKW is valid */
    if (rkw_info.magic_number != RKLD_MAGIC)
    {
        ret = EINVALID_FORMAT;
        goto end;
    }

    /* check header crc if present */
    if (rkw_info.load_options & RKW_HEADER_CRC)
    {
        crc = crc32_rkw((uint8_t *)&rkw_info, sizeof(rkw_info)-sizeof(uint32_t));
        if (rkw_info.crc != crc)
        {
            ret = EBAD_HEADER_CHKSUM;
            goto end;
        }
    }

    /* check image size */
    len = rkw_info.load_limit - rkw_info.load_address;
    if (len > buffer_size)
    {
        ret = EFILE_TOO_BIG;
        goto end;
    }

    /* check load address - we support loading only at 0x60000000 */
    if (rkw_info.load_address != 0x60000000)
    {
        ret = EINVALID_LOAD_ADDR;
        goto end;
    }

    /* rockbox extension - we use one of reserved fields to store
     * model number information. This prevents from loading
     * rockbox RKW for different player.
     */
    if (rkw_info.reserved0 != 0 && rkw_info.reserved0 != MODEL_NUMBER)
    {
        ret = EBAD_MODEL;
        goto end;
    }

    /* skip header */
    lseek(fd, sizeof(rkw_info), SEEK_SET);

    /* load image into buffer */
    rc = read(fd, buf, len);

    if(rc < len)
    {
        ret = EREAD_IMAGE_FAILED;
        goto end;
    }

    if (rkw_info.load_options & RKW_IMAGE_CRC)
    {
        rc = read(fd, &fw_crc, sizeof(uint32_t));

        crc = crc32_rkw((uint8_t *)buf, len);

        if (fw_crc != crc)
        {
            ret = EBAD_CHKSUM;
            goto end;
        }
    }

    ret = len;
end:
    close(fd);
    return ret;
}
