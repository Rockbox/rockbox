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
#ifndef __mg_h__
#define __mg_h__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Compute the MD5 digest of a buffer */
void MD5_CalculateDigest(void *digest, const void *input, size_t length);
/* Compute MD5 in more than one step */
void *md5_start(); /* return an opaque pointer */
void md5_update(void *md5_obj, const void *input, size_t length);
void md5_final(void *md5_obj, void *digest); /* destroys the MD5 object */

/* size must be a multiple of 8, this function is thread-safe */
void mg_decrypt_fw(void *in, int size, void *out, uint8_t key[8]);

/* for simplicity, these function use some global variables, this could be
 * change if necessary in the future */

/* DES: sizes must be a multiple of 8 */
void des_ecb_dec_set_key(const uint8_t key[8]);
void des_ecb_dec(void *in, int size, void *out);
void des_ecb_enc_set_key(const uint8_t key[8]);
void des_ecb_enc(void *in, int size, void *out);

/* AES: size must be a multiple of 16 */
void aes_ecb_dec_set_key(const uint8_t key[16]);
void aes_ecb_dec(void *in, int size, void *out);
void aes_ecb_enc_set_key(const uint8_t key[16]);
void aes_ecb_enc(void *in, int size, void *out);
void aes_cbc_dec_set_key_iv(const uint8_t key[16], const uint8_t iv[16]);
void aes_cbc_dec(void *in, int size, void *out);
void aes_cbc_enc_set_key_iv(const uint8_t key[16], const uint8_t iv[16]);
void aes_cbc_enc(void *in, int size, void *out);
#ifdef __cplusplus
}
#endif

#endif /* __mg_h__ */
