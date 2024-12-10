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
#ifndef __CRYPTO_S5L8702_H__
#define __CRYPTO_S5L8702_H__

#include <stdint.h>

#include "config.h"


/* IM3 */
#if CONFIG_CPU == S5L8702
#define IM3_IDENT       "8702"
#define IM3_VERSION     "1.0"
#define IM3HDR_SZ       0x800
#elif CONFIG_CPU == S5L8720
#define IM3_IDENT       "8720"
#define IM3_VERSION     "2.0"
#define IM3HDR_SZ       0x600
#endif

#define SHA1_SZ     20  /* bytes */
#define SIGN_SZ     16

#ifndef ASM
#define IM3INFO_SZ      (sizeof(struct Im3Info))
#define IM3INFOSIGN_SZ  (offsetof(struct Im3Info, info_sign))

struct Im3Info
{
    uint8_t ident[4];
    uint8_t version[3];
    uint8_t enc_type;
    uint8_t entry[4];   /* LE */
    uint8_t data_sz[4]; /* LE */
    union {
        struct {
            uint8_t data_sign[SIGN_SZ];
            uint8_t _reserved[32];
        } enc12;
        struct {
            uint8_t sign_off[4]; /* LE */
            uint8_t cert_off[4]; /* LE */
            uint8_t cert_sz[4];  /* LE */
            uint8_t _reserved[36];
        } enc34;
    } u;
    uint8_t info_sign[SIGN_SZ];
} __attribute__ ((packed));

struct Im3Hdr
{
    struct Im3Info info;
    uint8_t _zero[IM3HDR_SZ - sizeof(struct Im3Info)];
} __attribute__ ((packed));

enum hwkeyaes_direction
{
    HWKEYAES_DECRYPT = 0,
    HWKEYAES_ENCRYPT = 1
};

enum hwkeyaes_keyidx
{
    HWKEYAES_GKEY = 1,  /* device model key */
    HWKEYAES_UKEY = 2   /* device unique key */
};

void hwkeyaes(enum hwkeyaes_direction direction,
        uint32_t keyidx, void* data, uint32_t size);
void sha1(void* data, uint32_t size, void* hash);

void im3_sign(uint32_t keyidx, void* data, uint32_t size, void* sign);
void im3_crypt(enum hwkeyaes_direction direction,
                            struct Im3Info *hinfo, void *fw_addr);

#endif /* ASM */

#endif /* __CRYPTO_S5L8702_H__ */
