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

#if defined(HAVE_BOOTDATA)
#include "bootdata.h"
#include "crc32.h"

/* Write boot data into location marked by magic header
 * buffer is already loaded with the firmware image
 * we just need to find the location and write
 * data into the payload along with the crc
 * for later verification and use.
 * Returns payload len on success,
 * On error returns EKEY_NOT_FOUND
 */
static int write_bootdata(unsigned char* buf, int len, unsigned int boot_volume)
{
    struct boot_data_t bl_boot_data;
    struct boot_data_t *fw_boot_data = NULL;
    int search_len = MIN(len, BOOT_DATA_SEARCH_SIZE) - sizeof(struct boot_data_t);
    int payload_len = EKEY_NOT_FOUND;

    /* search for boot data header prior to search_len */
    for(int i = 0;i < search_len;i++)
    {
        fw_boot_data = (struct boot_data_t*) &buf[i];
        if (fw_boot_data->magic[0] != BOOT_DATA_MAGIC0 ||
            fw_boot_data->magic[1] != BOOT_DATA_MAGIC1)
            continue;
        /* 0 fill bootloader struct then add our data */
        memset(&bl_boot_data.payload, 0, BOOT_DATA_PAYLOAD_SIZE);
        bl_boot_data.boot_volume = boot_volume;
        /* 0 fill payload region in firmware */
        memset(fw_boot_data->payload, 0, fw_boot_data->length);
        /* determine maximum bytes we can write to firmware
           BOOT_DATA_PAYLOAD_SIZE is the size the bootloader expects */
        payload_len = MIN(BOOT_DATA_PAYLOAD_SIZE, fw_boot_data->length);
        /* write payload size back to firmware struct */
        fw_boot_data->length = payload_len;
        /* copy data to firmware bootdata struct */
        memcpy(fw_boot_data->payload, &bl_boot_data.payload, payload_len);
        /* calculate and write the crc for the payload */
        fw_boot_data->crc = crc_32(fw_boot_data->payload,
                                   payload_len,
                                   0xffffffff);
        break;

    }
    return payload_len;
}
#endif /* HAVE_BOOTDATA */
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
#ifdef HAVE_BOOTDATA
    /* 0 is the default boot volume */
    write_bootdata(buf, ret, 0);
#endif
    ret = len;


end:
    close(fd);
    return ret;
}
