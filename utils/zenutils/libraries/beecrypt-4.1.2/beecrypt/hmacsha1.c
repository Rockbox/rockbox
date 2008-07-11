/*
 * Copyright (c) 1999, 2000, 2001, 2002 Virtual Unlimited B.V.
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

/*!\file hmacsha1.c
 * \brief HMAC-SHA-1 message authentication code.
 *
 * \see RFC2202 - Test Cases for HMAC-MD5 and HMAC-SHA-1.
 *                P. Cheng, R. Glenn.
 *
 * \author Bob Deblier <bob.deblier@pandora.be>
 * \ingroup HMAC_m HMAC_sha1_m
 */

#define BEECRYPT_DLL_EXPORT

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/hmacsha1.h"

/*!\addtogroup HMAC_sha1_m
 * \{
 */

const keyedHashFunction hmacsha1 = {
	"HMAC-SHA-1",
	sizeof(hmacsha1Param),
	64,
	20,
	64,
	512,
	32,
	(keyedHashFunctionSetup) hmacsha1Setup,
	(keyedHashFunctionReset) hmacsha1Reset,
	(keyedHashFunctionUpdate) hmacsha1Update,
	(keyedHashFunctionDigest) hmacsha1Digest
};

int hmacsha1Setup (hmacsha1Param* sp, const byte* key, size_t keybits)
{
	return hmacSetup(sp->kxi, sp->kxo, &sha1, &sp->sparam, key, keybits);
}

int hmacsha1Reset (hmacsha1Param* sp)
{
	return hmacReset(sp->kxi, &sha1, &sp->sparam);
}

int hmacsha1Update(hmacsha1Param* sp, const byte* data, size_t size)
{
	return hmacUpdate(&sha1, &sp->sparam, data, size);
}

int hmacsha1Digest(hmacsha1Param* sp, byte* data)
{
	return hmacDigest(sp->kxo, &sha1, &sp->sparam, data);
}

/*!\}
 */
