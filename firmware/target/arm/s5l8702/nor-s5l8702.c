/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:
 *
 * Copyright © 2009 Michael Sparmann
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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "config.h"
#include "system.h"
#include "s5l8702.h"
#include "spi-s5l8702.h"
#include "crypto-s5l8702.h"
#include "nor-target.h"

static void bootflash_ce(int port, bool state)
{
    spi_ce(port, state);
}

void bootflash_init(int port)
{
    spi_init(port, true);
    bootflash_ce(port, false);
}

void bootflash_close(int port)
{
    spi_init(port, false);
}

static void bootflash_wait_ready(int port)
{
    while (true)
    {
        bootflash_ce(port, true);
        spi_write(port, 5);
        if (!(spi_write(port, 0xff) & 1)) break;
        bootflash_ce(port, false);
    }
    bootflash_ce(port, false);
}

static void bootflash_enable_writing(int port, bool state)
{
    if (!state)
    {
        bootflash_ce(port, true);
        spi_write(port, 4);
        bootflash_ce(port, false);
    }
    bootflash_ce(port, true);
    spi_write(port, 0x50);
    bootflash_ce(port, false);
    bootflash_ce(port, true);
    spi_write(port, 1);
    spi_write(port, state ? 0 : 0x1c);
    bootflash_ce(port, false);
    if (state)
    {
        bootflash_ce(port, true);
        spi_write(port, 6);
        bootflash_ce(port, false);
    }
}

void bootflash_read(int port, uint32_t addr, uint32_t size, void* buf)
{
    spi_prepare(port);
    bootflash_wait_ready(port);
    bootflash_ce(port, true);
    spi_write(port, 3);
    spi_write(port, (addr >> 16) & 0xff);
    spi_write(port, (addr >> 8) & 0xff);
    spi_write(port, addr & 0xff);
    spi_read(port, size, buf);
    bootflash_ce(port, false);
    spi_release(port);
}

static void bootflash_write_internal(int port, uint32_t addr, uint32_t size, void* buf)
{
    uint8_t* buffer = (uint8_t*)buf;
    bool first = true;
    spi_prepare(port);
    bootflash_wait_ready(port);
    bootflash_enable_writing(port, true);
    while (size)
    {
        bootflash_ce(port, true);
        spi_write(port, 0xad);
        if (first)
        {
            spi_write(port, (addr >> 16) & 0xff);
            spi_write(port, (addr >> 8) & 0xff);
            spi_write(port, addr & 0xff);
            first = false;
        }
        spi_write(port, *buffer++);
        spi_write(port, *buffer++);
        bootflash_ce(port, false);
        bootflash_wait_ready(port);
        size -= 2;
    }
    bootflash_enable_writing(port, false);
    spi_release(port);
}

static void bootflash_erase_internal(int port, uint32_t addr)
{
    spi_prepare(port);
    bootflash_wait_ready(port);
    bootflash_enable_writing(port, true);
    bootflash_ce(port, true);
    spi_write(port, 0x20);
    spi_write(port, (addr >> 16) & 0xff);
    spi_write(port, (addr >> 8) & 0xff);
    spi_write(port, addr & 0xff);
    bootflash_ce(port, false);
    bootflash_enable_writing(port, false);
    spi_release(port);
}

void bootflash_erase_blocks(int port, int first, int n)
{
    uint32_t offset = first << 12;
    while (n--)
    {
        bootflash_erase_internal(port, offset);
        offset += 0x1000;
    }
}

int bootflash_compare(int port, int offset, void* addr, int size)
{
    int i;
    int result = 0;
    uint8_t buf[32];
    spi_prepare(port);
    bootflash_wait_ready(port);
    bootflash_ce(port, true);
    spi_write(port, 3);
    spi_write(port, (offset >> 16) & 0xff);
    spi_write(port, (offset >> 8) & 0xff);
    spi_write(port, offset & 0xff);
    while (size > 0)
    {
        spi_read(port, MIN((int)sizeof(buf), size), buf);
        if (memcmp((uint8_t*)addr, buf, MIN((int)sizeof(buf), size)))
            result |= 1;
        for (i = 0; i < MIN((int)sizeof(buf), size); i += 2)
            if (buf[i] != 0xff) result |= 2;
        addr = (void*)(((uint32_t)addr) + sizeof(buf));
        size -= sizeof(buf);
    }
    bootflash_ce(port, false);
    spi_release(port);
    return result;
}

void bootflash_write(int port, int offset, void* addr, int size)
{
    int i;
    bool needswrite;
    while (size > 0)
    {
        int remainder = MIN(0x1000 - (offset & 0xfff), size);
        int contentinfo = bootflash_compare(port, offset, addr, remainder);
        if (contentinfo & 1)
        {
            if (contentinfo & 2)
                bootflash_erase_internal(port, offset & ~0xfff);
            needswrite = false;
            for (i = 0; i < remainder; i += 1)
                if (((uint8_t*)addr)[i] != 0xff)
                {
                    needswrite = true;
                    break;
                }
            if (needswrite)
                bootflash_write_internal(port, offset, remainder, addr);
        }
        addr = (void*)(((uint32_t)addr) + remainder);
        offset += remainder;
        size -= remainder;
    }
}


/*
 * IM3
 */
static uint32_t get_uint32le(unsigned char *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

/* return full IM3 size aligned to NOR sector size */
unsigned im3_nor_sz(struct Im3Info* hinfo)
{
    return ALIGN_UP(IM3HDR_SZ + get_uint32le(hinfo->data_sz), 0x1000);
}

/* calculates SHA1, truncate the result to 128 bits, and encrypt it */
void im3_sign(uint32_t keyidx, void* data, uint32_t size, void* sign)
{
    unsigned char hash[SHA1_SZ];
    sha1(data, size, hash);
    memcpy(sign, hash, SIGN_SZ);
    hwkeyaes(HWKEYAES_ENCRYPT, keyidx, sign, SIGN_SZ);
}

/* only supports enc_type 1 and 2 (UKEY) */
void im3_crypt(enum hwkeyaes_direction direction,
                            struct Im3Info *hinfo, void *fw_addr)
{
    uint32_t fw_size = get_uint32le(hinfo->data_sz);
    hinfo->enc_type = (direction == HWKEYAES_ENCRYPT) ? 1 : 2;
    im3_sign(HWKEYAES_UKEY, hinfo, IM3INFOSIGN_SZ, hinfo->info_sign);
    hwkeyaes(direction, HWKEYAES_UKEY, fw_addr, fw_size);
}

int im3_read(uint32_t offset, struct Im3Info *hinfo, void *fw_addr)
{
    unsigned char hash[SIGN_SZ];
    uint32_t fw_size;

    /* header */
    bootflash_init(SPI_PORT);
    bootflash_read(SPI_PORT, offset, IM3HDR_SZ, hinfo);
    bootflash_close(SPI_PORT);

    if (memcmp(hinfo, IM3_IDENT, 4) != 0)
        return -1; /* OF not found */

    im3_sign(HWKEYAES_UKEY, hinfo, IM3INFOSIGN_SZ, hash);
    if (memcmp(hash, hinfo->info_sign, SIGN_SZ) != 0)
        return -2; /* corrupt header */

    fw_size = get_uint32le(hinfo->data_sz);
    if ((fw_size > NORBOOT_MAXSZ - IM3HDR_SZ) ||
                (get_uint32le(hinfo->entry) > fw_size))
        return -3; /* wrong info */

    if (hinfo->enc_type != 1 && hinfo->enc_type != 2)
        return -4; /* encrypt type not supported */

    if (fw_addr)
    {
        /* body */
        bootflash_init(SPI_PORT);
        bootflash_read(SPI_PORT, offset + IM3HDR_SZ, fw_size, fw_addr);
        bootflash_close(SPI_PORT);
        if (hinfo->enc_type == 1)
            im3_crypt(HWKEYAES_DECRYPT, hinfo, fw_addr);
        im3_sign(HWKEYAES_UKEY, fw_addr, fw_size, hash);
        if (memcmp(hash, hinfo->u.enc12.data_sign, SIGN_SZ) != 0)
            return -5; /* corrupt data */
    }

    return 0;
}

bool im3_write(int offset, void *im3_addr)
{
    bool res;
    struct Im3Info *hinfo = (struct Im3Info *)im3_addr;
    uint32_t im3_size = get_uint32le(hinfo->data_sz) + IM3HDR_SZ;
    bootflash_init(SPI_PORT);
    bootflash_write(SPI_PORT, offset, im3_addr, im3_size);
    /* check if the IM3 image was written correctly */
    res = !(bootflash_compare(SPI_PORT, offset, im3_addr, im3_size) & 1);
    bootflash_close(SPI_PORT);
    return res;
}


/*
 * flsh FS
 */
unsigned flsh_get_unused(void)
{
    unsigned tail = DIR_OFF;
    bootflash_init(SPI_PORT);
    for (int i = 0; i < MAX_ENTRY; i++)
    {
        image_t entry;
        bootflash_read(SPI_PORT, DIR_OFF + ENTRY_SZ*i, ENTRY_SZ, &entry);
        if (memcmp(entry.type, "hslf", 4))
            break; /* no more entries */
        if (entry.devOffset < tail)
            tail = entry.devOffset;
    }
    bootflash_close(SPI_PORT);
    return tail - (NORBOOT_OFF + NORBOOT_MAXSZ);
}

int flsh_find_file(char *name, image_t *entry)
{
    int found = 0;
    bootflash_init(SPI_PORT);
    for (int off = 0; off < ENTRY_SZ*MAX_ENTRY; off += ENTRY_SZ)
    {
        bootflash_read(SPI_PORT, DIR_OFF+off, ENTRY_SZ, entry);
        if (memcmp(&(entry->type), "hslf", 4))
            break; /* no more entries */
        if (!memcmp(&(entry->id), name, 4)) {
            found = 1;
            break;
        }
    }
    bootflash_close(SPI_PORT);
    return found;
}

#define KEYIDX_OFF      0x8 /* TBC */
#define DATALEN_OFF     0x14
#define DATASHA_OFF     0x1c
#define HEADSHA_OFF     0x1d4

int flsh_load_file(image_t *entry, void *hdr, void *data)
{
    uint8_t orig_hash[SHA1_SZ];
    uint8_t calc_hash[SHA1_SZ];
    uint32_t data_len;

    /* header */
    bootflash_init(SPI_PORT);
    bootflash_read(SPI_PORT, entry->devOffset, FILEHDR_SZ, hdr);
    bootflash_close(SPI_PORT);
    memcpy(orig_hash, hdr + HEADSHA_OFF, SHA1_SZ);
    memset(hdr+HEADSHA_OFF, 0, SHA1_SZ);
    sha1(hdr, FILEHDR_SZ, calc_hash);
    if (memcmp(calc_hash, orig_hash, SHA1_SZ))
        return -1; /* corrupt header */
    memcpy(hdr+HEADSHA_OFF, orig_hash, SHA1_SZ);
    data_len = ALIGN_UP(*(uint32_t*)(hdr+DATALEN_OFF), 0x10);
    if (data_len > ALIGN_UP(entry->len, 0x10))
        return -2; /* bad size */

    if (data)
    {
        bootflash_init(SPI_PORT);
        bootflash_read(SPI_PORT, entry->devOffset+FILEHDR_SZ, data_len, data);
        bootflash_close(SPI_PORT);
        hwkeyaes(HWKEYAES_DECRYPT, *(uint8_t*)(hdr+KEYIDX_OFF), data, data_len);
        sha1(data, data_len, calc_hash);
        if (memcmp(calc_hash, hdr+DATASHA_OFF, SHA1_SZ))
            return -3; /* corrupt data */
    }

    return 0;
}
