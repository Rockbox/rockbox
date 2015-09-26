/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "clocking-s5l8702.h"
#include "nor-target.h"
#include "crypto-s5l8702.h"

void hwkeyaes(enum hwkeyaes_direction direction, uint32_t keyidx, void* data, uint32_t size)
{
    int i;
    clockgate_enable(CLOCKGATE_AES, true);
    for (i = 0; i < 4; i++) AESIV[i] = 0;
    AESUNKREG0 = 1;
    AESUNKREG0 = 0;
    AESCONTROL = 1;
    AESUNKREG1 = 0;
    AESTYPE = keyidx;
    AESTYPE2 = ~AESTYPE;
    AESUNKREG2 = 0;
    AESKEYLEN = direction == HWKEYAES_ENCRYPT ? 9 : 8;
    AESOUTSIZE = size;
    AESOUTADDR = data;
    AESINSIZE = size;
    AESINADDR = data;
    AESAUXSIZE = size;
    AESAUXADDR = data;
    AESSIZE3 = size;
    commit_discard_dcache();
    AESGO = 1;
    while (!(AESSTATUS & 0xf));
    clockgate_enable(CLOCKGATE_AES, false);
}

void sha1(void* data, uint32_t size, void* hash)
{
    int i, space;
    bool done = false;
    uint32_t tmp32[16];
    uint8_t* tmp8 = (uint8_t*)tmp32;
    uint32_t* databuf = (uint32_t*)data;
    uint32_t* hashbuf = (uint32_t*)hash;
    clockgate_enable(CLOCKGATE_SHA, true);
    SHA1RESET = 1;
    while (SHA1CONFIG & 1) udelay(10);
    SHA1RESET = 0;
    SHA1CONFIG = 0;
    while (!done)
    {
        space = ((uint32_t)databuf) - ((uint32_t)data) - size + 64;
        if (space > 0)
        {
            for (i = 0; i < 16; i++) tmp32[i] = 0;
            if (space <= 64)
            {
                for (i = 0; i < 64 - space; i++)
                    tmp8[i] = ((uint8_t*)databuf)[i];
                tmp8[64 - space] = 0x80;
            }
            if (space >= 8)
            {
                tmp8[0x3b] = (size >> 29) & 0xff;
                tmp8[0x3c] = (size >> 21) & 0xff;
                tmp8[0x3d] = (size >> 13) & 0xff;
                tmp8[0x3e] = (size >> 5) & 0xff;
                tmp8[0x3f] = (size << 3) & 0xff;
                done = true;
            }
            for (i = 0; i < 16; i++) SHA1DATAIN[i] = tmp32[i];
        }
        else for (i = 0; i < 16; i++) SHA1DATAIN[i] = *databuf++;
        SHA1CONFIG |= 2;
        while (SHA1CONFIG & 1) udelay(10);
        SHA1CONFIG |= 8;
    }
    for (i = 0; i < 5; i++) *hashbuf++ = SHA1RESULT[i];
    clockgate_enable(CLOCKGATE_SHA, false);
}

/*
 * IM3
 */
static uint32_t get_uint32le(unsigned char *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

void mksign(uint32_t keyidx, void* data, uint32_t size, void* sign)
{
    unsigned char hash[SHA1_SZ];
    sha1(data, size, hash);
    memcpy(sign, hash, SIGN_SZ);
    hwkeyaes(HWKEYAES_ENCRYPT, keyidx, sign, SIGN_SZ);
}

/* only supports enc_type 1 and 2 (UKEY) */
void crypt_fw(enum hwkeyaes_direction direction,
                            struct Im3Info *hinfo, void *fw_addr)
{
    uint32_t fw_size = get_uint32le(hinfo->data_sz);
    hinfo->enc_type = (direction == HWKEYAES_ENCRYPT) ? 1 : 2;
    mksign(HWKEYAES_UKEY, hinfo, IM3INFOSIGN_SZ, hinfo->info_sign);
    hwkeyaes(direction, HWKEYAES_UKEY, fw_addr, fw_size);
}

int read_im3(uint32_t offset, struct Im3Info *hinfo, void *fw_addr)
{
    unsigned char hash[SIGN_SZ];
    uint32_t fw_size;

    /* header */
    bootflash_init(SPI_PORT);
    bootflash_read(SPI_PORT, offset, IM3HDR_SZ, hinfo);
    bootflash_close(SPI_PORT);

    if (memcmp(hinfo, IM3_IDENT, 4) != 0)
        return -1; /* OF not found */

    mksign(HWKEYAES_UKEY, hinfo, IM3INFOSIGN_SZ, hash);
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
            crypt_fw(HWKEYAES_DECRYPT, hinfo, fw_addr);
        mksign(HWKEYAES_UKEY, fw_addr, fw_size, hash);
        if (memcmp(hash, hinfo->u.enc12.data_sign, SIGN_SZ) != 0)
            return -5; /* corrupt data */
    }

    return 0;
}

bool write_im3(int offset, void *im3_addr)
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
 * bootflash FS
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

        hwkeyaes(HWKEYAES_DECRYPT,
                    *(uint8_t*)(hdr+KEYIDX_OFF), data, data_len);
        sha1(data, data_len, calc_hash);
        if (memcmp(calc_hash, hdr+DATASHA_OFF, SHA1_SZ))
            return -3; /* corrupt data */
    }

    return 0;
}
