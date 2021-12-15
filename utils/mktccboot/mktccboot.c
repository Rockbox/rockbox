/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Dave Chapman
 *
 * Based on mkboot, Copyright (C) 2005 by Linus Nielsen Feltzing
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include "mktccboot.h"
#include "telechips.h"

/*

Append a Rockbox bootloader to a Telechips original firmware file.

The first instruction in a TCC firmware file is always of the form:

   ldr     pc, [pc, #xxx]

where [pc, #xxx] is the entry point of the firmware - e.g. 0x20000020

mktccboot appends the Rockbox bootloader to the end of the original
firmware image and replaces the contents of [pc, #xxx] with the entry
point of our bootloader - i.e. the length of the original firmware plus
0x20000000.

It then stores the original entry point from [pc, #xxx] in a fixed
offset in the Rockbox boootloader, which is used by the bootloader to
dual-boot.

Finally, mktccboot corrects the length and CRCs in the main firmware
header, creating a new legal firmware file which can be installed on
the device.

*/

/* win32 compatibility */

#ifndef O_BINARY
#define O_BINARY 0
#endif

static void put_uint32le(uint32_t x, unsigned char* p)
{
    p[0] = x & 0xff;
    p[1] = (x >> 8) & 0xff;
    p[2] = (x >> 16) & 0xff;
    p[3] = (x >> 24) & 0xff;
}

static uint32_t get_uint32le(unsigned char* p)
{
    return (p[3] << 24) | (p[2] << 16) | (p[1]<<8) | p[0];
}

static off_t filesize(int fd) {
    struct stat buf;

    if (fstat(fd,&buf) < 0) {
        perror("[ERR]  Checking filesize of input file");
        return -1;
    } else {
        return(buf.st_size);
    }
}

#define DRAMORIG 0x20000000
/* Injects a bootloader into a Telechips 77X/78X firmware file */
unsigned char *patch_firmware_tcc(unsigned char *of_buf, int of_size,
        unsigned char *boot_buf, int boot_size, int *patched_size)
{
    unsigned char *patched_buf;
    uint32_t ldr, old_ep_offset, new_ep_offset;
    int of_offset;

    patched_buf = malloc(of_size + boot_size);
    if (!patched_buf)
        return NULL;

    memcpy(patched_buf, of_buf, of_size);
    memcpy(patched_buf + of_size, boot_buf, boot_size);

    ldr = get_uint32le(patched_buf);

    /* TODO: Verify it's a LDR instruction */
    of_offset = (ldr & 0xfff) + 8;
    old_ep_offset = get_uint32le(patched_buf + of_offset);
    new_ep_offset = DRAMORIG + of_size;

    printf("OF entry point: 0x%08x\n", old_ep_offset);
    printf("New entry point: 0x%08x\n", new_ep_offset + 8);

    /* Save the OF entry point at the start of the bootloader image */
    put_uint32le(old_ep_offset, patched_buf + of_size);
    put_uint32le(new_ep_offset, patched_buf + of_size + 4);

    /* Change the OF entry point to the third word in our bootloader */
    put_uint32le(new_ep_offset + 8, patched_buf + of_offset);

    telechips_encode_crc(patched_buf, of_size + boot_size);
    *patched_size = of_size + boot_size;

    return patched_buf;
}

unsigned char *file_read(char *filename, int *size)
{
    unsigned char *buf = NULL;
    int n, fd = -1;

    /* Open file for reading */
    fd = open(filename, O_RDONLY|O_BINARY);
    if (fd < 0)
    {
        printf("[ERR] Could open file for reading, aborting\n");
        perror(filename);
        goto error;
    }

    /* Get file size, and allocate a buffer of that size */
    *size = filesize(fd);
    buf = malloc(*size);
    if (buf == NULL)
    {
        printf("[ERR] Could not allocate memory, aborting\n");
        goto error;
    }

    /* Read the file's content to the buffer */
    n = read(fd, buf, *size);
    if (n != *size)
    {
        printf("[ERR] Could not read from %s\n", filename);
        goto error;
    }

    return buf;

error:
    if (fd >= 0)
        close(fd);

    if (buf)
        free(buf);

    return NULL;
}

/* A CRC test in order to reject non OF file */
int test_firmware_tcc(unsigned char* buf, int length)
{
    return telechips_test_crc(buf, length);
}

