/* zenutils - Utilities for working with creative firmwares.
 * Copyright 2007 (c) Rasmus Ry <rasmus.ry{at}gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "crypt.h"
#include <stdexcept>
#include <beecrypt/hmacsha1.h>
#include <beecrypt/blockmode.h>
#include <beecrypt/blowfish.h>


bool zen::hmac_sha1_calc(const byte* key, size_t keylen, const byte* data,
                         size_t datalen, byte* sig, size_t* siglen)
{
    hmacsha1Param param;
    if (hmacsha1Setup(&param, key, keylen * 8))
        return false;
    if (hmacsha1Update(&param, data, datalen))
        return false;
    if (hmacsha1Digest(&param, sig))
        return false;
    return true;
}

bool zen::bf_cbc_encrypt(const byte* key, size_t keylen, byte* data,
                         size_t datalen, const byte* iv)
{
    if (datalen % blowfish.blocksize)
        throw std::invalid_argument(
            "The length must be aligned on a 8 byte boundary.");

	blowfishParam param;
    if (blowfishSetup(&param, key, keylen * 8, ENCRYPT))
        return false;
    if (blowfishSetIV(&param, iv))
        return false;

    byte* plain = new byte[datalen];
    memcpy(plain, data, datalen);

    unsigned int nblocks = datalen / blowfish.blocksize;
    if (blockEncryptCBC(&blowfish, &param, (uint32_t*)data, (uint32_t*)plain,
                        nblocks))
    {
        delete [] plain;
        return false;
    }

    return true;
}

bool zen::bf_cbc_decrypt(const byte* key, size_t keylen, byte* data,
                         size_t datalen, const byte* iv)
{
    if (datalen % blowfish.blocksize)
        throw std::invalid_argument(
            "The length must be aligned on a 8 byte boundary.");

	blowfishParam param;
    if (blowfishSetup(&param, key, keylen * 8, ENCRYPT))
        return false;
    if (blowfishSetIV(&param, iv))
        return false;

    byte* cipher = new byte[datalen];
    memcpy(cipher, data, datalen);

    unsigned int nblocks = datalen / blowfish.blocksize;
    if (blockDecryptCBC(&blowfish, &param, (uint32_t*)data, (uint32_t*)cipher,
                        nblocks))
    {
        delete [] cipher;
        return false;
    }

    return true;
}
