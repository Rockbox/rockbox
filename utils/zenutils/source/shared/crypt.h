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

#ifndef ZEN_CRYPT_H_INCLUDED
#define ZEN_CRYPT_H_INCLUDED

#include <utils.h>

namespace zen {
    bool hmac_sha1_calc(const byte* key, size_t keylen, const byte* data, size_t datalen, byte* sig, size_t* siglen);
    bool bf_cbc_encrypt(const byte* key, size_t keylen, byte* data, size_t datalen, const byte* iv);
    bool bf_cbc_decrypt(const byte* key, size_t keylen, byte* data, size_t datalen, const byte* iv);
}; //namespace zen

#endif //ZEN_CRYPT_H_INCLUDED
