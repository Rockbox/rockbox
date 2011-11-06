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

typedef uint8_t byte;

/* aes128.c */
void xor_(byte *a, byte *b, int n);
void EncryptAES(byte *msg, byte *key, byte *c);
void DecryptAES(byte *c, byte *key, byte *m);
void Pretty(byte* b,int len,const char* label);
void cbc_mac(
    byte *in_data, /* Input data */
    byte *out_data, /* Output data (or NULL) */
    int nr_blocks, /* Number of blocks to encrypt/decrypt (one block=16 bytes) */
    byte key[16], /* Key */
    byte iv[16], /* Initialisation Vector */
    byte (*out_cbc_mac)[16], /* CBC-MAC of the result (or NULL) */
    int encrypt /* 1 to encrypt, 0 to decrypt */
    );

/* crypto.c */
enum crypto_method_t
{
    CRYPTO_NONE, /* disable */
    CRYPTO_KEY, /* key */
    CRYPTO_USBOTP, /* use usbotp device */
};

/* parameter can be:
 * - CRYPTO_KEY: array of 16-bytes (the key)
 * - CRYPTO_USBOTP: 32-bit integer: vid << 16 | pid */
void crypto_setup(enum crypto_method_t method, void *param);

#define CRYPTO_ERROR_SUCCESS    0
#define CRYPTO_ERROR_BADSETUP   -1  /* bad crypto setup */
#define CRYPTO_ERROR_NODEVICE   -2 /* no device with vid:pid */
#define CRYPTO_ERROR_BADENDP    -3 /* device doesn't have the required endpoints */
#define CRYPTO_ERROR_CLAIMFAIL  -4 /* device interface claim error */
#define CRYPTO_ERROR_DEVREJECT  -5 /* device rejected cypto operation */
#define CRYPTO_ERROR_DEVSILENT  -6 /* device did not notify completion */
#define CRYPTO_ERROR_DEVERR     -7 /* device did something wrong (like return too small buffer) */
#define CRYPTO_NUM_ERRORS       8
/* return 0 on success, <0 on error */
int crypto_apply(
    byte *in_data, /* Input data */
    byte *out_data, /* Output data (or NULL) */
    int nr_blocks, /* Number of blocks (one block=16 bytes) */
    byte iv[16], /* IV */
    byte (*out_cbc_mac)[16], /* CBC-MAC of the result (or NULL) */
    int encrypt);

/* all-in-one function */
struct crypto_key_t
{
    enum crypto_method_t method;
    union
    {
        byte key[16];
        uint32_t vid_pid;
        byte param[0];
    }u;
};

int crypto_cbc(
    byte *in_data, /* Input data */
    byte *out_data, /* Output data (or NULL) */
    int nr_blocks, /* Number of blocks (one block=16 bytes) */
    struct crypto_key_t *key, /* Key */
    byte iv[16], /* IV */
    byte (*out_cbc_mac)[16], /* CBC-MAC of the result (or NULL) */
    int encrypt);

/* crc.c */
uint32_t crc(byte *data, int size);
uint32_t crc_continue(uint32_t previous_crc, byte *data, int size);

/* sha1.c */
struct sha_1_params_t
{
    uint32_t hash[5];
    uint64_t buffer_nr_bits;
    uint32_t w[80];
};

void sha_1_init(struct sha_1_params_t *params);
void sha_1_block(struct sha_1_params_t *params, uint32_t cur_hash[5], byte *data);
void sha_1_update(struct sha_1_params_t *params, byte *buffer, int size);
void sha_1_finish(struct sha_1_params_t *params);
void sha_1_output(struct sha_1_params_t *params, byte *out);

#endif /* __CRYPTO_H__ */
