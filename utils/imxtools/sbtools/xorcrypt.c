/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Amaury Pouly
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
#include <stdlib.h>
#include "misc.h"

static uint32_t do_round(union xorcrypt_key_t *key)
{
    uint32_t k7 = key->k[7];
    uint32_t k2 = key->k[2];
    uint32_t k5_3_1 = key->k[5] ^ key->k[3] ^ key->k[1];
    key->k[1] = k2;
    uint32_t k_11_7_5_3_1 = key->k[11] ^ k7 ^ k5_3_1;
    key->k[2] = key->k[3] ^ k2;
    key->k[3] = key->k[4];
    key->k[4] = key->k[5];
    key->k[5] = key->k[6];
    uint32_t k0 = key->k[0];
    key->k[7] = k7 ^ key->k[8];
    uint32_t k13 = key->k[13];
    key->k[8] = key->k[9];
    uint32_t k10 = key->k[10];
    key->k[6] = k7;
    key->k[9] = k10;
    uint32_t k11 = key->k[11];
    key->k[0] = k0 ^ k13 ^ k_11_7_5_3_1;
    key->k[10] = (k11 >> 1) | (k11 << 31);
    uint32_t k11_12 = k11 ^ key->k[12];
    uint32_t k14 = key->k[14];
    key->k[12] = k13;
    key->k[13] = k14;
    uint32_t k15 = key->k[15];
    key->k[15] = k0;
    key->k[11] = k11_12;
    key->k[14] = k15;
    return key->k[0];
}

uint32_t xor_encrypt(union xorcrypt_key_t keys[2], void *_data, int size)
{
    if(size % 4)
        bugp("xor_encrypt: size is not a multiple of 4 !\n");
    size /= 4;
    uint32_t *data = _data;
    uint32_t final_xor = 0;
    for(int i = 0; i < size; i += 4)
    {
        uint32_t x = do_round(&keys[1]);
        /* xor first key's first word with data (at most 4 words of data) */
        for(int j = i; j < i + 4 && j < size; j++)
        {
            keys[0].k[0] ^= data[j];
            data[j] ^= x;
        }
        final_xor = do_round(&keys[0]);
    }
    return final_xor ^ do_round(&keys[1]);
}

uint32_t xor_decrypt(union xorcrypt_key_t keys[2], void *_data, int size)
{
    if(size % 4)
        bugp("xor_decrypt: size is not a multiple of 4 !\n");
    size /= 4;
    uint32_t *data = _data;
    uint32_t final_xor = 0;
    for(int i = 0; i < size; i += 4)
    {
        uint32_t x = do_round(&keys[1]);
        /* xor first key's first word with data (at most 4 words of data) */
        for(int j = i; j < i + 4 && j < size; j++)
        {
            data[j] ^= x;
            keys[0].k[0] ^= data[j];
        }
        final_xor = do_round(&keys[0]);
    }
    return final_xor ^ do_round(&keys[1]);
}