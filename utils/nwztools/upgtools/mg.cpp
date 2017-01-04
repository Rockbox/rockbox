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
#include <crypto++/cryptlib.h>
#include <crypto++/modes.h>
#include <crypto++/des.h>
#include <crypto++/aes.h>
#include <stdio.h>

using namespace CryptoPP;
namespace
{
    inline void dec_des_ecb(void *in, int size, void *out, uint8_t *key)
    {
        ECB_Mode< DES >::Decryption dec;
        if(size % 8)
            abort(); /* size must be a multiple of 8 */
        dec.SetKey(key, 8);
        dec.ProcessData((byte*)out, (byte*)in, size);
    }

    inline void enc_des_ecb(void *in, int size, void *out, uint8_t *key)
    {
        ECB_Mode< DES >::Encryption enc;
        if(size % 8)
            abort(); /* size must be a multiple of 8 */
        enc.SetKey(key, 8);
        enc.ProcessData((byte*)out, (byte*)in, size);
    }
}

void mg_decrypt_fw(void *in, int size, void *out, uint8_t *key)
{
    dec_des_ecb(in, size, out, key);
}

void mg_encrypt_fw(void *in, int size, void *out, uint8_t *key)
{
    enc_des_ecb(in, size, out, key);
}

void mg_decrypt_pass(void *in, int size, void *out, uint8_t *key)
{
    dec_des_ecb(in, size, out, key);
}

void mg_encrypt_pass(void *in, int size, void *out, uint8_t *key)
{
    enc_des_ecb(in, size, out, key);
}
