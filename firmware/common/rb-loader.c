/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 * Copyright (C) 2017 by William Wilgus
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
#include "checksum.h"

#if defined(HAVE_BOOTDATA) || defined(HAVE_MULTIBOOT)
#include "multiboot.h"
#endif

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
    unsigned long chksum;
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

    if (!verify_checksum(chksum, buf, len))
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
    char filename[MAX_PATH+2];
    /* only filename passed */
    if (firmware[0] != '/')
    {

#ifdef HAVE_MULTIBOOT /* defined by config.h */
        /* checks <volumes> highest index to lowest for redirect file
         * 0 is the default boot volume, it is not checked here
         * if found <volume>/rockbox_main.<playername> and firmware
         * has a bootdata region this firmware will be loaded */
        for (int i = NUM_VOLUMES - 1; i >= MULTIBOOT_MIN_VOLUME && ret < 0; i--)
        {
            if (get_redirect_dir(filename, sizeof(filename), i,
                                 BOOTDIR, firmware) > 0)
            {
                ret = load_firmware_filename(buf, filename, buffer_size);
            /* if firmware has no boot_data don't load from external drive */
                if (write_bootdata(buf, ret, i) <= 0)
                    ret = EKEY_NOT_FOUND;
            }
            /* if ret is valid breaks from loop to continue loading */
        }
#endif

        if (ret < 0) /* Check default volume, no valid firmware file loaded yet */
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
    else /* full path passed ROLO etc.*/
        ret = load_firmware_filename(buf, firmware, buffer_size);

    return ret;
}
