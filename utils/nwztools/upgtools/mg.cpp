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

/* on Windows, we use Cryptography API, on other platforms we rely on crypto++ */
#if defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>

static HCRYPTPROV g_hCryptProv;

typedef struct
{
    BLOBHEADER hdr;
    DWORD      dwKeySize;
    BYTE       rgbKeyData[];
} PLAINTEXTKEY;

static bool check_context()
{
    if(g_hCryptProv)
        return true;
    return CryptAcquireContext(&g_hCryptProv, NULL, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
}

static HCRYPTKEY import_key(ALG_ID algid, DWORD mode, const uint8_t *key, int key_size, const uint8_t *iv)
{
    if(!check_context())
        return 0;
    DWORD blob_size = sizeof(PLAINTEXTKEY) + key_size;
    PLAINTEXTKEY *key_blob = (PLAINTEXTKEY *)malloc(blob_size);
    key_blob->hdr.bType = PLAINTEXTKEYBLOB;
    key_blob->hdr.bVersion = CUR_BLOB_VERSION;
    key_blob->hdr.reserved = 0;
    key_blob->hdr.aiKeyAlg = algid;
    key_blob->dwKeySize = key_size;
    memcpy(key_blob->rgbKeyData, key, key_size);
    HCRYPTKEY hKey;
    if(!CryptImportKey(g_hCryptProv, (const BYTE *)key_blob, blob_size, 0, 0, &hKey))
        return 0;
    CryptSetKeyParam(hKey, KP_MODE, (const BYTE *)&mode, 0);
    if(iv)
        CryptSetKeyParam(hKey, KP_IV, (const BYTE *)iv, 0);
    return hKey;
}

void MD5_CalculateDigest(void *digest, const void *input, size_t length)
{
    if(!check_context())
        return;
    HCRYPTHASH hHash;
    if(!CryptCreateHash(g_hCryptProv, CALG_MD5, 0, 0, &hHash))
        return;
    if(!CryptHashData(hHash, (const BYTE *)input, length, 0))
        return;
    DWORD dwSize = 16;
    if(!CryptGetHashParam(hHash, HP_HASHVAL, (BYTE *)digest, &dwSize, 0))
        return;
    CryptDestroyHash(hHash);
}

void *md5_start()
{
    if(!check_context())
        return NULL;
    HCRYPTHASH hHash;
    if(!CryptCreateHash(g_hCryptProv, CALG_MD5, 0, 0, &hHash))
        return NULL;
    return reinterpret_cast<void *>(hHash);
}

void md5_update(void *md5_obj, const void *input, size_t length)
{
    HCRYPTHASH hHash = reinterpret_cast<HCRYPTHASH>(md5_obj);
    CryptHashData(hHash, (const BYTE *)input, length, 0);
}

void md5_final(void *md5_obj, void *digest)
{
    HCRYPTHASH hHash = reinterpret_cast<HCRYPTHASH>(md5_obj);
    DWORD dwSize = 16;
    if(!CryptGetHashParam(hHash, HP_HASHVAL, (BYTE *)digest, &dwSize, 0))
        return;
    CryptDestroyHash(hHash);
}

void mg_decrypt_fw(void *in, int size, void *out, uint8_t *key)
{
    if(!check_context() || (size % 8) != 0)
        abort(); /* size must be a multiple of 8 */
    HCRYPTKEY hKey = import_key(CALG_DES, CRYPT_MODE_ECB, key, 8, NULL);
    memcpy(out, in, size);
    DWORD dwSize = size;
    CryptDecrypt(hKey, 0, FALSE, 0, (BYTE *)out, &dwSize);
    CryptDestroyKey(hKey);
}

static HCRYPTKEY g_des_ecb_dec_key;

void des_ecb_dec_set_key(const uint8_t key[8])
{
    if(g_des_ecb_dec_key)
        CryptDestroyKey(g_des_ecb_dec_key);
    g_des_ecb_dec_key = import_key(CALG_DES, CRYPT_MODE_ECB, key, 8, NULL);
}

void des_ecb_dec(void *in, int size, void *out)
{
    if(size % 8)
        abort(); /* size must be a multiple of 8 */
    memcpy(out, in, size);
    DWORD dwSize = size;
    CryptDecrypt(g_des_ecb_dec_key, 0, FALSE, 0, (BYTE *)out, &dwSize);
}

static HCRYPTKEY g_des_ecb_enc_key;

void des_ecb_enc_set_key(const uint8_t key[8])
{
    if(g_des_ecb_enc_key)
        CryptDestroyKey(g_des_ecb_enc_key);
    g_des_ecb_enc_key = import_key(CALG_DES, CRYPT_MODE_ECB, key, 8, NULL);
}

void des_ecb_enc(void *in, int size, void *out)
{
    if(size % 8)
        abort(); /* size must be a multiple of 8 */
    memcpy(out, in, size);
    DWORD dwSize = size;
    CryptEncrypt(g_des_ecb_enc_key, 0, FALSE, 0, (BYTE *)out, &dwSize, dwSize);
}

static HCRYPTKEY g_aes_ecb_dec_key;

void aes_ecb_dec_set_key(const uint8_t key[16])
{
    if(g_aes_ecb_dec_key)
        CryptDestroyKey(g_aes_ecb_dec_key);
    g_aes_ecb_dec_key = import_key(CALG_AES_128, CRYPT_MODE_ECB, key, 16, NULL);
}

void aes_ecb_dec(void *in, int size, void *out)
{
    if(size % 16)
        abort(); /* size must be a multiple of 16 */
    memcpy(out, in, size);
    DWORD dwSize = size;
    CryptDecrypt(g_aes_ecb_dec_key, 0, FALSE, 0, (BYTE *)out, &dwSize);
}

static HCRYPTKEY g_aes_ecb_enc_key;

void aes_ecb_enc_set_key(const uint8_t key[16])
{
    if(g_aes_ecb_enc_key)
        CryptDestroyKey(g_aes_ecb_enc_key);
    g_aes_ecb_enc_key = import_key(CALG_AES_128, CRYPT_MODE_ECB, key, 16, NULL);
}

void aes_ecb_enc(void *in, int size, void *out)
{
    if(size % 16)
        abort(); /* size must be a multiple of 16 */
    memcpy(out, in, size);
    DWORD dwSize = size;
    CryptEncrypt(g_aes_ecb_enc_key, 0, FALSE, 0, (BYTE *)out, &dwSize, dwSize);
}

static HCRYPTKEY g_aes_cbc_dec_key;

void aes_cbc_dec_set_key_iv(const uint8_t key[16], const uint8_t iv[16])
{
    if(g_aes_cbc_dec_key)
        CryptDestroyKey(g_aes_cbc_dec_key);
    g_aes_cbc_dec_key = import_key(CALG_AES_128, CRYPT_MODE_CBC, key, 16, iv);
}

void aes_cbc_dec(void *in, int size, void *out)
{
    if(size % 16)
        abort(); /* size must be a multiple of 16 */
    memcpy(out, in, size);
    DWORD dwSize = size;
    CryptDecrypt(g_aes_cbc_dec_key, 0, FALSE, 0, (BYTE *)out, &dwSize);
}

static HCRYPTKEY g_aes_cbc_enc_key;

void aes_cbc_enc_set_key_iv(const uint8_t key[16], const uint8_t iv[16])
{
    if(g_aes_cbc_enc_key)
        CryptDestroyKey(g_aes_cbc_enc_key);
    g_aes_cbc_enc_key = import_key(CALG_AES_128, CRYPT_MODE_CBC, key, 16, iv);
}

void aes_cbc_enc(void *in, int size, void *out)
{
    if(size % 16)
        abort(); /* size must be a multiple of 16 */
    memcpy(out, in, size);
    DWORD dwSize = size;
    CryptEncrypt(g_aes_cbc_enc_key, 0, FALSE, 0, (BYTE *)out, &dwSize, dwSize);
}


#else /* Not on Windows */
/* MD5 is considered insecure by crypto++ */
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK  1
#include <cryptopp/cryptlib.h>
#include <cryptopp/modes.h>
#include <cryptopp/des.h>
#include <cryptopp/aes.h>
#include <cryptopp/md5.h>

using namespace CryptoPP;
using namespace CryptoPP::Weak;

void MD5_CalculateDigest(void *digest, const void *input, size_t length)
{
    MD5().CalculateDigest((byte *)digest, (const byte *)input, length);
}

void *md5_start()
{
    return new MD5;
}

void md5_update(void *md5_obj, const void *input, size_t length)
{
    MD5 *md5 = reinterpret_cast<MD5 *>(md5_obj);
    md5->Update(reinterpret_cast<const uint8_t *>(input), length);
}

void md5_final(void *md5_obj, void *digest)
{
    MD5 *md5 = reinterpret_cast<MD5 *>(md5_obj);
    md5->Final(reinterpret_cast<uint8_t *>(digest));
    delete md5;
}

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
#endif
