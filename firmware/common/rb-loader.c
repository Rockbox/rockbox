/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
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
#include "system.h"
#include "file.h"
#include "rb-loader.h"
#include "loader_strerror.h"

/* Load firmware image in a format created by add method of tools/scramble
 * on success we return size loaded image
 * on error we return negative value which can be deciphered by means
 * of strerror() function
 */
int load_firmware(unsigned char* buf, const char* firmware, int buffer_size)
{
    char filename[MAX_PATH];
    int fd;
    int rc;
    int len;
    int ret = 0;
    unsigned long chksum;
    unsigned long sum;
    int i;

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

    len = filesize(fd) - 8;

    if (len > buffer_size)
    {
        ret = EFILE_TOO_BIG;
        goto end;
    }

    lseek(fd, FIRMWARE_OFFSET_FILE_CRC, SEEK_SET);

    rc = read(fd, &chksum, 4);
    chksum = betoh32(chksum); /* Rockbox checksums are big-endian */
    if(rc < 4)
    {
        ret = EREAD_CHKSUM_FAILED;
        goto end;
    }

    lseek(fd, FIRMWARE_OFFSET_FILE_DATA, SEEK_SET);

    rc = read(fd, buf, len);
    if(rc < len)
    {
        ret = EREAD_IMAGE_FAILED;
        goto end;
    }

    sum = MODEL_NUMBER;

    for(i = 0;i < len;i++)
    {
        sum += buf[i];
    }

    if(sum != chksum)
    {
        ret = EBAD_CHKSUM;
        goto end;
    }

    ret = len;

end:
    close(fd);
    return ret;
}

