/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017, 2020 by William Wilgus
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

#include "system.h"
#include "bootdata.h"
#include "crc32.h"
#include "loader_strerror.h"
#include "file.h"
#include <string.h>
#include <stdio.h>

/* Write bootdata into location in FIRMWARE marked by magic header
 * Assumes buffer is already loaded with the firmware image
 * We just need to find the location and write data into the
 * payload region along with the crc for later verification and use.
 * Returns payload len on success,
 * On error returns EKEY_NOT_FOUND
 */
int write_bootdata(unsigned char* buf, int len, unsigned int boot_volume)
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

        memset(&bl_boot_data.payload, 0, BOOT_DATA_PAYLOAD_SIZE);
        bl_boot_data.boot_volume = boot_volume;

        memset(fw_boot_data->payload, 0, fw_boot_data->length);
        /* determine maximum bytes we can write to firmware
           BOOT_DATA_PAYLOAD_SIZE is the size the bootloader expects */
        payload_len = MIN(BOOT_DATA_PAYLOAD_SIZE, fw_boot_data->length);
        fw_boot_data->length = payload_len;
        /* copy data to FIRMWARE bootdata struct */
        memcpy(fw_boot_data->payload, &bl_boot_data.payload, payload_len);
        /* crc will be used within the firmware to check validity of bootdata */
        fw_boot_data->crc = crc_32(fw_boot_data->payload, payload_len, 0xffffffff);
        break;

    }
    return payload_len;
}

#ifdef HAVE_MULTIBOOT
/* Check in root of this <volume> for rockbox_main.<playername>
 * if this file empty or there is a single slash '/'
 * buf = '<volume#>/<rootdir>/<firmware(name)>\0'
 * If instead '/<*DIRECTORY*>' is supplied
 * addpath will be set to this DIRECTORY buf =
 * '/<volume#>/addpath/<rootdir>/<firmware(name)>\0'
 * On error returns Negative number or 0
 * On success returns bytes from snprintf
 * and generated path will be placed in buf
 * note: if supplied buffer is too small return will be
 * the number of bytes that would have been written
 */
int get_redirect_dir(char* buf, int buffer_size, int volume,
                     const char* rootdir, const char* firmware)
{
    int fd;
    int f_offset;
    char add_path[MAX_PATH];
    /* Check in root of volume for rockbox_main.<playername> redirect */
    snprintf(add_path, sizeof(add_path), "/<%d>/"BOOT_REDIR, volume);
    fd = open(add_path, O_RDONLY);
    if (fd < 0)
        return EFILE_NOT_FOUND;

    /*clear add_path for re-use*/
    memset(add_path, 0, sizeof(add_path));
    f_offset = read(fd, add_path,sizeof(add_path));
    close(fd);

    for(int i = f_offset - 1;i > 0; i--)
    {
        /* strip control chars < SPACE or all if path doesn't start with '/' */
        if (add_path[i] < 0x20 || add_path[0] != '/')
           add_path[i] = '\0';
    }
    /* if '/add_path' is specified in rockbox_main.<playername>
       path is /<vol#>/add_path/rootdir/firmwarename
       if add_path is empty or '/' is missing from beginning
       path is /<vol#>/rootdir/firmwarename
    */
    return snprintf(buf, buffer_size, "/<%d>%s/%s/%s", volume, add_path,
                                                        rootdir, firmware);
}
#endif
