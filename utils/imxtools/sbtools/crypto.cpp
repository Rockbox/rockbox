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
#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#include <cryptopp/sha.h>

using namespace CryptoPP;

namespace
{

enum crypto_method_t g_cur_method = CRYPTO_NONE;
byte g_key[16];
CBC_Mode<AES>::Encryption g_aes_enc;
CBC_Mode<AES>::Decryption g_aes_dec;
bool g_aes_enc_key_dirty; /* true of g_aes_enc key needs to be updated */
bool g_aes_dec_key_dirty; /* same for g_aes_dec */

int cbc_mac2(
    const byte *in_data, /* Input data */
    byte *out_data, /* Output data (or NULL) */
    int nr_blocks, /* Number of blocks to encrypt/decrypt (one block=16 bytes) */
    byte key[16], /* Key */
    byte iv[16], /* Initialisation Vector */
    byte (*out_cbc_mac)[16], /* CBC-MAC of the result (or NULL) */
    bool encrypt /* 1 to encrypt, 0 to decrypt */
    )
{
    /* encrypt */
    if(encrypt)
    {
        /* update keys if neeeded */
        if(g_aes_enc_key_dirty)
        {
            /* we need to provide an IV with the key, although we change it
             * everytime we run the cipher anyway */
            g_aes_enc.SetKeyWithIV(g_key, 16, iv, 16);
            g_aes_enc_key_dirty = false;
        }
        g_aes_enc.Resynchronize(iv, 16);
        byte tmp[16];
        /* we need some output buffer, either a temporary one if we are CBC-MACing
         * only, or use output buffer if available */
        byte *out_ptr = (out_data == NULL) ? tmp : out_data;
        while(nr_blocks-- > 0)
        {
            g_aes_enc.ProcessData(out_ptr, in_data, 16);
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
        /* update keys if neeeded */
        if(g_aes_dec_key_dirty)
        {
            /* we need to provide an IV with the key, although we change it
             * everytime we run the cipher anyway */
            g_aes_dec.SetKeyWithIV(g_key, 16, iv, 16);
            g_aes_dec_key_dirty = false;
        }
        /* we cannot produce a CBC-MAC in decrypt mode, output buffer exists */
        if(out_cbc_mac || out_data == NULL)
            return CRYPTO_ERROR_INVALID_OP;
        g_aes_dec.Resynchronize(iv, 16);
        g_aes_dec.ProcessData(out_data, in_data, nr_blocks * 16);
        return CRYPTO_ERROR_SUCCESS;
    }
}

}

int crypto_setup(struct crypto_key_t *key)
{
    g_cur_method = key->method;
    switch(g_cur_method)
    {
        case CRYPTO_KEY:
            memcpy(g_key, key->u.key, 16);
            g_aes_dec_key_dirty = true;
            g_aes_enc_key_dirty = true;
            return CRYPTO_ERROR_SUCCESS;
        default:
            return CRYPTO_ERROR_BADSETUP;
    }
}

int crypto_apply(
    byte *in_data, /* Input data */
    byte *out_data, /* Output data (or NULL) */
    int nr_blocks, /* Number of blocks (one block=16 bytes) */
    byte iv[16], /* Key */
    byte (*out_cbc_mac)[16], /* CBC-MAC of the result (or NULL) */
    bool encrypt)
{
    if(g_cur_method == CRYPTO_KEY)
        return cbc_mac2(in_data, out_data, nr_blocks, g_key, iv, out_cbc_mac, encrypt);
    else
        return CRYPTO_ERROR_BADSETUP;
}

void sha_1_init(struct sha_1_params_t *params)
{
    params->object = new SHA1;
}

void sha_1_update(struct sha_1_params_t *params, byte *buffer, int size)
{
    reinterpret_cast<SHA1 *>(params->object)->Update(buffer, size);
}

void sha_1_finish(struct sha_1_params_t *params)
{
    SHA1 *obj = reinterpret_cast<SHA1 *>(params->object);
    obj->Final(params->hash);
    delete obj;
}

void sha_1_output(struct sha_1_params_t *params, byte *out)
{
    memcpy(out, params->hash, 20);
}
