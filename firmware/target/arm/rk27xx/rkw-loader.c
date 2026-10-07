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
#include <stdbool.h>
#include "config.h"
#include "loader_strerror.h"
#include "rkw-loader.h"
#include "crc32-rkw.h"
#include "file.h"
#include "panic.h"

#ifdef BOOTLOADER
/* Before the NAND bootloader jumps to an image it writes three words at
 * the address in header field 0x14: a magic, its own version and which
 * copy of the image it loaded. The OF never initialises them; it reports
 * the version over USB and counts its reboots in the third word. When we
 * start the OF ourselves we pass on what the NAND bootloader gave us, so
 * the OF sees the same as when it is started directly.
 */
#define RKW_HANDOFF_MAGIC    0x03df479a
/* written over the magic once the words have been read */
#define RKW_HANDOFF_CONSUMED 0x123456ad

/* The last 12 bytes of DRAM. tools/rkw.c puts this address in every
 * Rockbox RKW, so the NAND bootloader leaves our words here.
 */
#define RKW_HANDOFF_ADDR     (0x60000000 + MEMORYSIZE * 0x100000 - 12)
#if MEMORYSIZE != 16
#error "tools/rkw.c assumes 16 MB of DRAM, update RKW_HANDOFF_ADDR there"
#endif

struct rkw_handoff_t {
    uint32_t magic;
    uint32_t version;
    uint32_t source;
};

/* The NAND bootloader's words, read once: loading an image may overwrite
 * them, and a reboot keeps DRAM, so they are marked consumed.
 */
static struct rkw_handoff_t *rkw_own_handoff(void)
{
    static struct rkw_handoff_t own;
    static bool read_done = false;
    volatile struct rkw_handoff_t *h =
        (volatile struct rkw_handoff_t *)RKW_HANDOFF_ADDR;

    if (!read_done)
    {
        if (h->magic == RKW_HANDOFF_MAGIC)
        {
            own.version = h->version;
            own.source = h->source;
        }
        else
        {
            /* not started by the NAND bootloader, e.g. over USB */
            own.version = 0;
            own.source = 0;
        }
        own.magic = RKW_HANDOFF_MAGIC;
        h->magic = RKW_HANDOFF_CONSUMED;
        read_done = true;
    }

    return &own;
}

/* Hand the words on to an image loaded at its own address, when its
 * header points past the image and below us.
 */
static void rkw_pass_handoff(const struct rkw_header_t *hdr,
                             const unsigned char *buf, int len,
                             int buffer_size)
{
    uintptr_t addr = hdr->handoff;
    uintptr_t lo = (uintptr_t)buf + len;
    uintptr_t hi = (uintptr_t)buf + buffer_size;

    if ((uintptr_t)buf != hdr->load_address || (addr & 3) ||
        addr < lo || addr > hi - sizeof(struct rkw_handoff_t))
        return;

    *(struct rkw_handoff_t *)addr = *rkw_own_handoff();
}
#endif /* BOOTLOADER */

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

#ifdef BOOTLOADER
    /* before the image can overwrite them */
    rkw_own_handoff();
#endif

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

#ifdef BOOTLOADER
    rkw_pass_handoff(&rkw_info, buf, len, buffer_size);
#endif

    ret = len;
end:
    close(fd);
    return ret;
}

