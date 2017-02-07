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

#include "bootdata.h"
#include "crc32.h"

#if defined(HAVE_MULTIVOLUME) && defined(MODEL_NAME_SHORT)
#include "mv.h"
#include "pathfuncs.h"
/* Write boot data into location marked by magic header
 * buffer is already loaded with the firmware image
 * we just need to find the location and write
 * data into the payload along with the crc
 * for later verification and use.
 * Returns payload len on success,
 * On error returns EKEY_NOT_FOUND
 */
int write_bootdata(unsigned char* buf, int len, unsigned int boot_volume)
{
    struct boot_data_t bl_boot_data;
    struct boot_data_t *fw_boot_data = NULL;
    unsigned int magic[2] = {BOOT_DATA_MAGIC0, BOOT_DATA_MAGIC1};
    int search_len = MIN(len, BOOT_DATA_SEARCH_SIZE);
    int payload_len = EKEY_NOT_FOUND;

    /* 0 fill bootloader struct then add our data */
    memset(&bl_boot_data.payload, 0, BOOT_DATA_PAYLOAD_SIZE);
    bl_boot_data.boot_volume = boot_volume;

    /* search for boot data header prior to search_len */
    for(unsigned int i = 0;i <= (search_len - sizeof(struct boot_data_t));i++)
    {
        if (memcmp (&buf[i], &magic[0], sizeof(magic)) == 0)
        {
            fw_boot_data = (struct boot_data_t*) &buf[i];
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
    }
    return payload_len;
}
/* returns path for firmware in volumes/drives other than player root */
int get_redirect_dir(char* buf, int buffer_size, const char* firmware, int volume)
{
    int fd = EFILE_NOT_FOUND;
    int f_offset = 0;
    char add_path[MAX_PATH] = "\0";
    char buf_vol_name[VOL_MAX_LEN+1] = "\0";
    if (get_volume_name(volume, buf_vol_name) > 0)
    {
        /* Check in root for rockbox_main.playername redirect */
        snprintf(buf, buffer_size, "/%s/"BOOT_REDIR"."MODEL_NAME_SHORT,
                                        buf_vol_name);
        fd = open(buf, O_RDONLY);
    }
    if (fd < 0)
    {
        return EFILE_NOT_FOUND;
    }
    else
    {
        /* path is /buf_vol_name/add_path/BOOTDIR/ */
        f_offset = read(fd, buf, MIN(buffer_size - 1, filesize(fd)));
        /* there is a redirect specified in the file */
        add_path[0] = '\0';
        if ( f_offset > 0)
        {
            add_path[f_offset] = '\0';
            buf[f_offset] = '\0';
            for(int i = 0;i < f_offset; i++)
            {
                if (buf[i] >= 0x20) /* strip control chars < SPACE */
                    add_path[i] = buf[i];
                else
                    add_path[i] = '\0';
            }
        }
        f_offset = snprintf(buf, buffer_size, "/%s/%s/%s/%s", buf_vol_name,
                                                              add_path,
                                                              BOOTDIR,
                                                              firmware);

    }
    close(fd);
    return f_offset;
}
#endif /* HAVE_MULTIVOLUME && MODEL_NAME_SHORT */

/* loads a firmware file from supplied filename
 * file opened, checks firmware size and checksum
 * if no error, firmware loaded to supplied buffer
 * file closed
 * Returns size of loaded image on success
 * On error returns Negative value deciphered by means
 * of strerror() function
 */
int load_firmware_filename(unsigned char* buf, const char* filename, int buffer_size)
{

    int len;
    unsigned long chksum, sum;
    int rc;
    int i;
    int ret = EFILE_NOT_FOUND;
    int fd = open(filename, O_RDONLY);

    if (fd < 0)
    {
        ret = EFILE_NOT_FOUND;
        goto end;
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

/* Load firmware image in a format created by add method of tools/scramble
 * on success we return size loaded image
 * on error we return negative value which can be deciphered by means
 * of strerror() function
 */
int load_firmware(unsigned char* buf, const char* firmware, int buffer_size)
{
    char filename[MAX_PATH];
    int ret = EFILE_NOT_FOUND;;
    /*0 is the default boot volume*/

    /* only filename passed */
    if (firmware[0] != '/')
    {
/*If multivolume check volume highest index to lowest for firmware file*/
#if defined(HAVE_MULTIVOLUME) && defined(MODEL_NAME_SHORT)
        /*look for rockbox_main.clip+*/
        for (int i = NUM_VOLUMES; i > 1 && ret < 0; i--)
        {
            if (get_redirect_dir(filename, sizeof(filename), firmware, i-1) > 0)
                ret = load_firmware_filename(buf, filename, buffer_size);
           /* if firmware has no boot_data don't load from external drives */
            if (write_bootdata(buf, ret, i-1) <= 0)
                    ret = EKEY_NOT_FOUND;
            /*if ret is valid break from loop to continue loading */
        }
#endif
        if (ret < 0)
        {
            /* First check in BOOTDIR */
            snprintf(filename, sizeof(filename), BOOTDIR "/%s",firmware);

            ret = load_firmware_filename(buf, filename, buffer_size);

            if(ret < 0)
            {
                /* Check in root dir */
                snprintf(filename, sizeof(filename),"/%s",firmware);
                ret = load_firmware_filename(buf, filename, buffer_size);
            }
        }
    }
    else /* full path passed */
        ret = load_firmware_filename(buf, firmware, buffer_size);


    return ret;
}
