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
/* Based on http://en.wikipedia.org/wiki/SHA-1 */
#include "crypto.h"

static uint32_t rot_left(uint32_t val, int rot)
{
    return (val << rot) | (val >> (32 - rot));
}

static inline void byte_swapxx(byte *ptr, int size)
{
    for(int i = 0; i < size / 2; i++)
    {
        byte c = ptr[i];
        ptr[i] = ptr[size - i - 1];
        ptr[size - i - 1] = c;
    }
}

static void byte_swap32(uint32_t *v)
{
    byte_swapxx((byte *)v, 4);
}

void sha_1_init(struct sha_1_params_t *params)
{
    params->hash[0] = 0x67452301;
    params->hash[1] = 0xEFCDAB89;
    params->hash[2] = 0x98BADCFE;
    params->hash[3] = 0x10325476;
    params->hash[4] = 0xC3D2E1F0;
    params->buffer_nr_bits = 0;
}

void sha_1_update(struct sha_1_params_t *params, byte *buffer, int size)
{
    int buffer_nr_bytes = (params->buffer_nr_bits / 8) % 64;
    params->buffer_nr_bits += 8 * size;
    int pos = 0;
    if(buffer_nr_bytes + size >= 64)
    {
        pos = 64 - buffer_nr_bytes;
        memcpy((byte *)(params->w) + buffer_nr_bytes, buffer, 64 - buffer_nr_bytes);
        sha_1_block(params, params->hash, (byte *)params->w);
        for(; pos + 64 <= size; pos += 64)
            sha_1_block(params, params->hash, buffer + pos);
        buffer_nr_bytes = 0;
    }
    memcpy((byte *)(params->w) + buffer_nr_bytes, buffer + pos, size - pos);
}

void sha_1_finish(struct sha_1_params_t *params)
{
    /* length (in bits) in big endian BEFORE preprocessing */
    byte length_big_endian[8];
    memcpy(length_big_endian, &params->buffer_nr_bits, 8);
    byte_swapxx(length_big_endian, 8);
    /* append '1' and then '0's to the message to get 448 bit length for the last block */
    byte b = 0x80;
    sha_1_update(params, &b, 1);
    b = 0;
    while((params->buffer_nr_bits % 512) != 448)
        sha_1_update(params, &b, 1);
    /* append length */
    sha_1_update(params, length_big_endian, 8);
    /* go back to big endian */
    for(int i = 0; i < 5; i++)
        byte_swap32(&params->hash[i]);
}

void sha_1_output(struct sha_1_params_t *params, byte *out)
{
    memcpy(out, params->hash, 20);
}

void sha_1_block(struct sha_1_params_t *params, uint32_t cur_hash[5], byte *data)
{
    uint32_t a, b, c, d, e;
    a = cur_hash[0];
    b = cur_hash[1];
    c = cur_hash[2];
    d = cur_hash[3];
    e = cur_hash[4];

    #define w params->w

    memmove(w, data, 64);
    for(int i = 0; i < 16; i++)
        byte_swap32(&w[i]);
        
    for(int i = 16; i <= 79; i++)
        w[i] = rot_left(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

    for(int i = 0; i<= 79; i++)
    {
        uint32_t f, k;
        if(i <= 19)
        {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        }
        else if(i <= 39)
        {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        }
        else if(i <= 59)
        {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        }
        else
        {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }
        uint32_t temp = rot_left(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rot_left(b, 30);
        b = a;
        a = temp;
    }
    #undef w
    
    cur_hash[0] += a;
    cur_hash[1] += b;
    cur_hash[2] += c;
    cur_hash[3] += d;
    cur_hash[4] += e;
}
