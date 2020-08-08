/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 Amaury Pouly
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
#include "crypto.h"
#include "misc.h"

#include "tomcrypt.h"



enum crypto_method_t g_cur_method = CRYPTO_NONE;
uint8_t g_key[16];

int cbc_mac2(
    const uint8_t *in_data, /* Input data */
    uint8_t *out_data, /* Output data (or NULL) */
    int nr_blocks, /* Number of blocks to encrypt/decrypt (one block=16 bytes) */
    uint8_t key[16], /* Key */
    uint8_t iv[16], /* Initialisation Vector */
    uint8_t (*out_cbc_mac)[16], /* CBC-MAC of the result (or NULL) */
    bool encrypt /* 1 to encrypt, 0 to decrypt */
    )
{
    int cipher = register_cipher(&aes_desc);
    symmetric_CBC cbc;
    cbc_start(cipher, iv, key, 16, 0, &cbc);

    /* encrypt */
    if(encrypt)
    {
        uint8_t tmp[16];
        /* we need some output buffer, either a temporary one if we are CBC-MACing
         * only, or use output buffer if available */
        uint8_t *out_ptr = (out_data == NULL) ? tmp : out_data;
        while(nr_blocks-- > 0)
        {
            cbc_encrypt(in_data, out_ptr, 16, &cbc);
            /* if this is the last block, copy CBC-MAC */
            if(nr_blocks == 0 && out_cbc_mac)
                memcpy(out_cbc_mac, out_ptr, 16);
            /* if we are writing data to the output buffer, advance output pointer */
            if(out_data != NULL)
                out_ptr += 16;
            in_data += 16;
        }
        return CRYPTO_ERROR_SUCCESS;
    }
    /* decrypt */
    else
    {
        cbc_decrypt(in_data, out_data, nr_blocks * 16, &cbc);

        /* update keys if neeeded */
        /* we cannot produce a CBC-MAC in decrypt mode, output buffer exists */
        if(out_cbc_mac || out_data == NULL)
            return CRYPTO_ERROR_INVALID_OP;

        return CRYPTO_ERROR_SUCCESS;
    }
}


int crypto_setup(struct crypto_key_t *key)
{
    g_cur_method = key->method;
    switch(g_cur_method)
    {
        case CRYPTO_KEY:
            memcpy(g_key, key->u.key, 16);

            return CRYPTO_ERROR_SUCCESS;
        default:
            return CRYPTO_ERROR_BADSETUP;
    }
}

int crypto_apply(
    uint8_t *in_data, /* Input data */
    uint8_t *out_data, /* Output data (or NULL) */
    int nr_blocks, /* Number of blocks (one block=16 bytes) */
    uint8_t iv[16], /* Key */
    uint8_t (*out_cbc_mac)[16], /* CBC-MAC of the result (or NULL) */
    bool encrypt)
{
    if(g_cur_method == CRYPTO_KEY)
        return cbc_mac2(in_data, out_data, nr_blocks, g_key, iv, out_cbc_mac, encrypt);
    else
        return CRYPTO_ERROR_BADSETUP;
}

void sha_1_init(struct sha_1_params_t *params)
{
    sha1_init(&params->state);
}

void sha_1_update(struct sha_1_params_t *params, uint8_t *buffer, int size)
{
    sha1_process(&params->state, buffer, size);
}

void sha_1_finish(struct sha_1_params_t *params)
{
    sha1_done(&params->state, params->hash);
}

void sha_1_output(struct sha_1_params_t *params, uint8_t *out)
{
    memcpy(out, params->hash, 20);
}
