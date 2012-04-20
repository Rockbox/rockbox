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

#include "config.h"
#include "rkw-loader.h"
#include "crc32-rkw.h"
#include "file.h"
#include "panic.h"

/* error strings sorted by number */
/* error 0 is empty */
static const char *err_str[] = {
    "Loading OK",
    "Can't open RKW file",
    "Can't read RKW header",
    "Invalid RKW magic number",
    "RKW header CRC error",
    "RKW file too big",
    "RKW Load address mismatch",
    "Bad model number",
    "Error Reading File",
    "RKW firmware CRC error"
};

const char *rkw_strerror(int8_t errno)
{
    if ((uint8_t)-errno >= sizeof(err_str)/sizeof(err_str[0]) || errno > 0)
        return "Unknown error";

    return err_str[-errno];
}

/* loosely based on load_firmware()
 * on success we return size loaded image
 * on error we return negative value which can be deciphered by means
 * of rkw_strerror() function
 */
int load_rkw(unsigned char* buf, const char* filename, int buffer_size)
{
    int fd;
    int rc;
    int len;
    int ret;
    uint32_t crc, fw_crc;
    struct rkw_header_t rkw_info;

    fd = open(filename, O_RDONLY);

    if(fd < 0)
        return -1;

    rc = read(fd, &rkw_info, sizeof(rkw_info));
    if (rc < (int)sizeof(rkw_info))
    {
        ret = -2;
        goto end;
    }

    /* check if RKW is valid */
    if (rkw_info.magic_number != RKLD_MAGIC)
    {
        ret = -3;
        goto end;
    }

    /* check header crc if present */
    if (rkw_info.load_options & RKW_HEADER_CRC)
    {
        crc = crc32_rkw((uint8_t *)&rkw_info, sizeof(rkw_info)-sizeof(uint32_t));
        if (rkw_info.crc != crc)
        {
            ret = -4;
            goto end;
        }
    }

    /* check image size */
    len = rkw_info.load_limit - rkw_info.load_address;
    if (len > buffer_size)
    {
        ret = -5;
        goto end;
    }

    /* check load address - we support loading only at 0x60000000 */
    if (rkw_info.load_address != 0x60000000)
    {
        ret = -6;
        goto end;
    }

    /* rockbox extension - we use one of reserved fields to store
     * model number information. This prevents from loading
     * rockbox RKW for different player.
     */
    if (rkw_info.reserved0 != 0 && rkw_info.reserved0 != MODEL_NUMBER)
    {
        ret = -7;
        goto end;
    }

    /* skip header */
    lseek(fd, sizeof(rkw_info), SEEK_SET);

    /* load image into buffer */
    rc = read(fd, buf, len);

    if(rc < len)
    {
        ret = -8;
        goto end;
    }

    if (rkw_info.load_options & RKW_IMAGE_CRC)
    {
        rc = read(fd, &fw_crc, sizeof(uint32_t));

        crc = crc32_rkw((uint8_t *)buf, len);

        if (fw_crc != crc)
        {
            ret = -9;
            goto end;
        }
    }

    ret = len;
end:
    close(fd);
    return ret;
}

