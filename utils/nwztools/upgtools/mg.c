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
#include "mg.h"
#include "des.h"
#include <stdio.h>

static int dec_des_ecb(uint8_t *in, int size, uint8_t *out, uint8_t *key)
{
    if(size % DES_BLOCK_SIZE)
        return 42;
    BYTE schedule[16][6];
    des_key_setup(key, schedule, DES_DECRYPT);
    while (size)
    {
      des_crypt(in, out, schedule);
      in += DES_BLOCK_SIZE;
      out += DES_BLOCK_SIZE;
      size -= DES_BLOCK_SIZE;
    }
    return 0;
}

static int enc_des_ecb(uint8_t *in, int size, uint8_t *out, uint8_t *key)
{
    if(size % DES_BLOCK_SIZE)
        return 42;
    BYTE schedule[16][6];
    des_key_setup(key, schedule, DES_ENCRYPT);
    while (size)
    {
      des_crypt(in, out, schedule);
      in += DES_BLOCK_SIZE;
      out += DES_BLOCK_SIZE;
      size -= DES_BLOCK_SIZE;
    }
    return 0;
}


int mg_decrypt_fw(void *in, int size, void *out, uint8_t *key)
{
    return dec_des_ecb(in, size, out, key);
}

int mg_encrypt_fw(void *in, int size, void *out, uint8_t *key)
{
    return enc_des_ecb((uint8_t*)in, size, (uint8_t*)out, key);
}

int mg_decrypt_pass(void *in, int size, void *out, uint8_t *key)
{
    return dec_des_ecb((uint8_t*)in, size, (uint8_t*)out, key);
}

int mg_encrypt_pass(void *in, int size, void *out, uint8_t *key)
{
    return enc_des_ecb((uint8_t*)in, size, (uint8_t*)out, key);
}
