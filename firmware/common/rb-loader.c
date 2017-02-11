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
#include "loader_strerror.h"
#include "rb-loader.h"

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

#ifdef HAVE_MULTIBOOT
/* places path in supplied buffer for firmware in volumes/drives other
 * than player root
 * on error returns Negative number or 0
 * on success returns bytes from snprintf
 * note: if supplied buffer is too small return will be
 * the number of bytes that would have been written
 */
int get_redirect_dir(char* buf, int buffer_size,
                            const char* firmware, int volume)
{
    int fd;
    int f_offset;
    char add_path[MAX_PATH];
    /* Check in root of volume for rockbox_main.playername redirect */
    snprintf(add_path, sizeof(add_path), "/<%d>/"BOOT_REDIR, volume);
    fd = open(add_path, O_RDONLY);
    if (fd < 0)
        return EFILE_NOT_FOUND;
    /*clear add_path for re-use*/
    memset(add_path, 0, sizeof(add_path));
    f_offset = read(fd, add_path,sizeof(add_path));
    close(fd);
    /* walk string from end */
    for(int i = f_offset - 1;i > 0; i--)
    {
        if (add_path[i] < 0x20) /* strip control chars < SPACE */
           add_path[i] = '\0';
    }
    /* path is /<vol#>/add_path/BOOTDIR/firmwarename
       if add_path is empty /<vol#>//BOOTDIR/firmwarename */
    return snprintf(buf, buffer_size, "/<%d>/%s/"BOOTDIR"/%s", volume,
                                                               add_path,
                                                               firmware);
}
#endif /* HAVE_MULTIBOOT */

/* loads a firmware file from supplied filename
 * file opened, checks firmware size and checksum
 * if no error, firmware loaded to supplied buffer
 * file closed
 * Returns size of loaded image on success
 * On error returns Negative value deciphered by means
 * of strerror() function
 */
static int load_firmware_filename(unsigned char* buf,
                                  const char* filename,
                                  int buffer_size)
{
    int len;
    unsigned long chksum, sum;
    int i;
    int ret;
    int fd = open(filename, O_RDONLY);

    if (fd < 0)
        return EFILE_NOT_FOUND;

    len = filesize(fd) - 8;

    if (len > buffer_size)
    {
        ret = EFILE_TOO_BIG;
        goto end;
    }

    lseek(fd, FIRMWARE_OFFSET_FILE_CRC, SEEK_SET);

    if (read(fd, &chksum, 4) < 4)
    {
        ret = EREAD_CHKSUM_FAILED;
        goto end;
    }
    chksum = betoh32(chksum); /* Rockbox checksums are big-endian */

    lseek(fd, FIRMWARE_OFFSET_FILE_DATA, SEEK_SET);

    if (read(fd, buf, len) < len)
    {
        ret = EREAD_IMAGE_FAILED;
        goto end;
    }

    sum = MODEL_NUMBER;

    for(i = 0;i < len;i++)
    {
        sum += buf[i];
    }

    if (sum != chksum)
    {
        ret = EBAD_CHKSUM;
        goto end;
    }
    ret = len;

end:
    close(fd);
    return ret;
}

/* Load firmware image in a format created by add method of tools/scramble
 * on success we return size loaded image
 * on error we return negative value which can be deciphered by means
 * of strerror() function
 */
int load_firmware(unsigned char* buf, const char* firmware, int buffer_size)
{

    int ret = EFILE_NOT_FOUND;
    char filename[MAX_PATH];
    /* only filename passed */
    if (firmware[0] != '/')
    {
/*If multivolume check volume highest index to lowest for firmware file*/
#ifdef HAVE_MULTIBOOT
        /* look for rockbox_main.<playername> */
        /* 0 is the default boot volume, not checked here */
        for (unsigned int i = NUM_VOLUMES - 1; i > 0 && ret < 0; i--)
        {
            if (get_redirect_dir(filename, sizeof(filename), firmware, i) > 0)
            {
                ret = load_firmware_filename(buf, filename, buffer_size);
            /* if firmware has no boot_data don't load from external drives */
                if (write_bootdata(buf, ret, i) <= 0)
                    ret = EKEY_NOT_FOUND;
            }
            /* if ret is valid break from loop to continue loading */
        }
#endif

        if (ret < 0)
        {
            /* First check in BOOTDIR */
            snprintf(filename, sizeof(filename), BOOTDIR "/%s",firmware);

            ret = load_firmware_filename(buf, filename, buffer_size);

            if (ret < 0)
            {
                /* Check in root dir */
                snprintf(filename, sizeof(filename),"/%s",firmware);
                ret = load_firmware_filename(buf, filename, buffer_size);
            }
#ifdef HAVE_BOOTDATA
                /* 0 is the default boot volume */
                write_bootdata(buf, ret, 0);
#endif
        }
    }
    else /* full path passed */
        ret = load_firmware_filename(buf, firmware, buffer_size);


    return ret;
}
