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
#include <cryptopp/cryptlib.h>
#include <cryptopp/modes.h>
#include <cryptopp/des.h>
#include <cryptopp/aes.h>
#include <stdio.h>

using namespace CryptoPP;

void mg_decrypt_fw(void *in, int size, void *out, uint8_t *key)
{
    ECB_Mode< DES >::Decryption dec;
    if(size % 8)
        abort(); /* size must be a multiple of 8 */
    dec.SetKey(key, 8);
    dec.ProcessData((byte*)out, (byte*)in, size);
}

static ECB_Mode< DES >::Decryption g_des_ecb_dec;

void des_ecb_dec_set_key(const uint8_t key[8])
{
    g_des_ecb_dec.SetKey(key, 8);
}

void des_ecb_dec(void *in, int size, void *out)
{
    if(size % 8)
        abort(); /* size must be a multiple of 8 */
    g_des_ecb_dec.ProcessData((byte*)out, (byte*)in, size);
}

static ECB_Mode< DES >::Encryption g_des_ecb_enc;

void des_ecb_enc_set_key(const uint8_t key[8])
{
    g_des_ecb_enc.SetKey(key, 8);
}

void des_ecb_enc(void *in, int size, void *out)
{
    if(size % 8)
        abort(); /* size must be a multiple of 8 */
    g_des_ecb_enc.ProcessData((byte*)out, (byte*)in, size);
}

static ECB_Mode< AES >::Decryption g_aes_ecb_dec;

void aes_ecb_dec_set_key(const uint8_t key[16])
{
    g_aes_ecb_dec.SetKey(key, 16);
}

void aes_ecb_dec(void *in, int size, void *out)
{
    if(size % 16)
        abort(); /* size must be a multiple of 16 */
    g_aes_ecb_dec.ProcessData((byte*)out, (byte*)in, size);
}

static ECB_Mode< AES >::Encryption g_aes_ecb_enc;

void aes_ecb_enc_set_key(const uint8_t key[16])
{
    g_aes_ecb_enc.SetKey(key, 16);
}

void aes_ecb_enc(void *in, int size, void *out)
{
    if(size % 16)
        abort(); /* size must be a multiple of 16 */
    g_aes_ecb_enc.ProcessData((byte*)out, (byte*)in, size);
}

static CBC_Mode< AES >::Decryption g_aes_cbc_dec;

void aes_cbc_dec_set_key_iv(const uint8_t key[16], const uint8_t iv[16])
{
    g_aes_cbc_dec.SetKeyWithIV(key, 16, iv);
}

void aes_cbc_dec(void *in, int size, void *out)
{
    if(size % 16)
        abort(); /* size must be a multiple of 16 */
    g_aes_cbc_dec.ProcessData((byte*)out, (byte*)in, size);
}

static CBC_Mode< AES >::Encryption g_aes_cbc_enc;

void aes_cbc_enc_set_key_iv(const uint8_t key[16], const uint8_t iv[16])
{
    g_aes_cbc_enc.SetKeyWithIV(key, 16, iv);
}

void aes_cbc_enc(void *in, int size, void *out)
{
    if(size % 16)
        abort(); /* size must be a multiple of 16 */
    g_aes_cbc_enc.ProcessData((byte*)out, (byte*)in, size);
}
