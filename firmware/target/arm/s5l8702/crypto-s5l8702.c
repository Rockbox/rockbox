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
    hwkeyaes(HWKEYAES_ENCRYPT, keyidx, hash, SIGN_SZ);
    memcpy(sign, hash, SIGN_SZ);
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

bool read_im3(uint32_t offset, struct Im3Info *hinfo, void *fw_addr)
{
    unsigned char hash[SIGN_SZ];
    uint32_t fw_size;
    bool res = 0; /* error */

    bootflash_init(SPI_PORT);

    bootflash_read(SPI_PORT, offset, IM3HDR_SZ, hinfo);

    /* check header */
    if (memcmp(hinfo, IM3_IDENT, 4) != 0)
        goto bye; /* OF not found */

    mksign(HWKEYAES_UKEY, hinfo, IM3INFOSIGN_SZ, hash);
    if (memcmp(hash, hinfo->info_sign, SIGN_SZ) != 0)
        goto bye; /* corrupt header */

    fw_size = get_uint32le(hinfo->data_sz);

    if ((fw_size > NORBOOT_MAXSZ - IM3HDR_SZ) ||
                (get_uint32le(hinfo->entry) > fw_size))
        goto bye; /* wrong info */

    if (hinfo->enc_type != 1 && hinfo->enc_type != 2)
        goto bye; /* encrypt type not supported */

    if (fw_addr)
    {
        /* read data */
        bootflash_read(SPI_PORT, offset + IM3HDR_SZ, fw_size, fw_addr);

        if (hinfo->enc_type == 1)
            crypt_fw(HWKEYAES_DECRYPT, hinfo, fw_addr);

        /* verify data */
        mksign(HWKEYAES_UKEY, fw_addr, fw_size, hash);
        if (memcmp(hash, hinfo->u.enc12.data_sign, SIGN_SZ) != 0)
            goto bye; /* corrupt data */
    }

    res = 1; /* ok */

bye:
    bootflash_close(SPI_PORT);
    return res;
}

bool write_im3(int offset, void *im3_addr)
{
    struct Im3Info *hinfo = (struct Im3Info *)im3_addr;
    uint32_t im3_size = get_uint32le(hinfo->data_sz) + IM3HDR_SZ;
    int n_retry = 1;
    bool res;

    bootflash_init(SPI_PORT);

    do
    {
        bootflash_write(SPI_PORT, offset, im3_addr, im3_size);

        /* verify that the IM3 was written correctly */
        res = !(bootflash_compare(SPI_PORT,
                    offset, im3_addr, im3_size) & 1);
    }
    while (!res && n_retry--);

    bootflash_close(SPI_PORT);
    return res;
}
