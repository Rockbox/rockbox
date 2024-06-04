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
#include "disk.h"
#include <string.h>
#include <stdio.h>

static void write_bootdata_v0(struct boot_data_t *data, unsigned int boot_volume)
{
    memset(data->payload, 0, data->length);

    data->_boot_volume = boot_volume;
    data->version = 0;
}

static void write_bootdata_v1(struct boot_data_t *data, unsigned int boot_volume)
{
    memset(data->payload, 0, data->length);

    data->_boot_volume = 0xff;
    data->version = 1;
    data->boot_drive = volume_drive(boot_volume);
    data->boot_partition = volume_partition(boot_volume);
}

/* Write bootdata into location in FIRMWARE marked by magic header
 * Assumes buffer is already loaded with the firmware image
 * We just need to find the location and write data into the
 * payload region along with the crc for later verification and use.
 * Returns payload len on success,
 * On error returns false
 */
bool write_bootdata(unsigned char* buf, int len, unsigned int boot_volume)
{
    int search_len = MIN(len, BOOT_DATA_SEARCH_SIZE) - sizeof(struct boot_data_t);

    /* search for boot data header prior to search_len */
    for(int i = 0; i < search_len; i++)
    {
        struct boot_data_t *data = (struct boot_data_t *)&buf[i];
        if (data->magic[0] != BOOT_DATA_MAGIC0 ||
            data->magic[1] != BOOT_DATA_MAGIC1)
            continue;

        /* Ignore it if the length extends past the end of the buffer. */
        int data_len = offsetof(struct boot_data_t, payload) + data->length;
        if (i + data_len > len)
            continue;

        /* Determine the maximum supported boot protocol version.
         * Version 0 firmware may use 0 or 0xff, all other versions
         * declare the highest supported version in the version byte. */
        int proto_version = 0;
        if (data->version < 0xff)
            proto_version = MIN(BOOT_DATA_VERSION, data->version);

        /* Write boot data according to the selected protocol */
        if (proto_version == 0)
            write_bootdata_v0(data, boot_volume);
        else if (proto_version == 1)
            write_bootdata_v1(data, boot_volume);
        else
            break;

        /* Calculate payload CRC, used by all protocol versions. */
        data->crc = crc_32(data->payload, data->length, 0xffffffff);
        return true;
    }

    return false;
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
    size_t f_offset;
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

    for(size_t i = f_offset - 1; i < f_offset; i--)
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
