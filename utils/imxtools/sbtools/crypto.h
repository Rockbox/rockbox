/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Amaury Pouly
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
#ifndef __CRYPTO_H__
#define __CRYPTO_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t byte;

/* crypto.cpp */
enum crypto_method_t
{
    CRYPTO_NONE, /* disable */
    CRYPTO_KEY, /* key */
    CRYPTO_XOR_KEY, /* XOR key */
};

union xorcrypt_key_t
{
    uint8_t key[64];
    uint32_t k[16];
};

/* all-in-one function */
struct crypto_key_t
{
    enum crypto_method_t method;
    union
    {
        byte key[16];
        union xorcrypt_key_t xor_key[2];
    }u;
};

#define CRYPTO_ERROR_SUCCESS    0
#define CRYPTO_ERROR_BADSETUP   -1
#define CRYPTO_ERROR_INVALID_OP -2

/* parameter can be:
 * - CRYPTO_KEY: array of 16-bytes (the key)
 * return 0 on success, <0 on error */
int crypto_setup(struct crypto_key_t *key);

/* return 0 on success, <0 on error */
int crypto_apply(
    byte *in_data, /* Input data */
    byte *out_data, /* Output data (or NULL) */
    int nr_blocks, /* Number of blocks (one block=16 bytes) */
    byte iv[16], /* IV */
    byte (*out_cbc_mac)[16], /* CBC-MAC of the result (or NULL) */
    bool encrypt);

/* crc.c */
uint32_t crc(byte *data, int size);
uint32_t crc_continue(uint32_t previous_crc, byte *data, int size);

/* sha1.c */
struct sha_1_params_t
{
    byte hash[20]; /* final hash */
    void *object; /* pointer to CryptoPP::SHA1 object */
};

void sha_1_init(struct sha_1_params_t *params);
void sha_1_update(struct sha_1_params_t *params, byte *buffer, int size);
void sha_1_finish(struct sha_1_params_t *params);
void sha_1_output(struct sha_1_params_t *params, byte *out);

/* xorcrypt.c */

// WARNING those functions modifies the keys !!
uint32_t xor_encrypt(union xorcrypt_key_t keys[2], void *data, int size);
uint32_t xor_decrypt(union xorcrypt_key_t keys[2], void *data, int size);
void xor_generate_key(uint32_t laserfuse[3], union xorcrypt_key_t key[2]);

#ifdef __cplusplus
}
#endif

#endif /* __CRYPTO_H__ */
