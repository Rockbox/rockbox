/*
 * Copyright (c) 1999, 2000, 2002 Virtual Unlimited B.V.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*!\file hmac.c
 * \brief HMAC algorithm.
 *
 * \see RFC2104 HMAC: Keyed-Hashing for Message Authentication.
 *                    H. Krawczyk, M. Bellare, R. Canetti.
 *
 * \author Bob Deblier <bob.deblier@pandore.be>
 * \ingroup HMAC_m
 */

#define BEECRYPT_DLL_EXPORT

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/hmac.h"
#include "beecrypt/endianness.h"

/*!\addtogroup HMAC_m
 * \{
 */

#define HMAC_IPAD	0x36
#define HMAC_OPAD	0x5c

int hmacSetup(byte* kxi, byte* kxo, const hashFunction* hash, hashFunctionParam* param, const byte* key, size_t keybits)
{
	register unsigned int i;

	size_t keybytes = keybits >> 3;

	/* if the key is too large, hash it first */
	if (keybytes > hash->blocksize)
	{
		/* if the hash digest is too large, this doesn't help; this is really a sanity check */
		if (hash->digestsize > hash->blocksize)
			return -1;

		if (hash->reset(param))
			return -1;

		if (hash->update(param, key, keybytes))
			return -1;

		if (hash->digest(param, kxi))
			return -1;

		memcpy(kxo, kxi, keybytes = hash->digestsize);
	}
	else if (keybytes > 0)
	{
		memcpy(kxi, key, keybytes);
		memcpy(kxo, key, keybytes);
	}
	else
		return -1;

	for (i = 0; i < keybytes; i++)
	{
		kxi[i] ^= HMAC_IPAD;
		kxo[i] ^= HMAC_OPAD;
	}

	for (i = keybytes; i < hash->blocksize; i++)
	{
		kxi[i] = HMAC_IPAD;
		kxo[i] = HMAC_OPAD;
	}

	return hmacReset(kxi, hash, param);
}

int hmacReset(const byte* kxi, const hashFunction* hash, hashFunctionParam* param)
{
	if (hash->reset(param))
		return -1;
	if (hash->update(param, kxi, hash->blocksize))
		return -1;

	return 0;
}

int hmacUpdate(const hashFunction* hash, hashFunctionParam* param, const byte* data, size_t size)
{
	return hash->update(param, data, size);
}

int hmacDigest(const byte* kxo, const hashFunction* hash, hashFunctionParam* param, byte* data)
{
	if (hash->digest(param, data))
		return -1;
	if (hash->update(param, kxo, hash->blocksize))
		return -1;
	if (hash->update(param, data, hash->digestsize))
		return -1;
	if (hash->digest(param, data))
		return -1;

	return 0;
}

/*!\}
 */
