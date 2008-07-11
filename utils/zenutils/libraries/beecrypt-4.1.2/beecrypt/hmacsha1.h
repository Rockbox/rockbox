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

/*!\file hmacsha1.h
 * \brief HMAC-SHA-1 message authentication code, headers.
 * \author Bob Deblier <bob.deblier@pandora.be>
 * \ingroup HMAC_m HMAC_sha1_m
 */

#ifndef _HMACSHA1_H
#define _HMACSHA1_H

#include "beecrypt/hmac.h"
#include "beecrypt/sha1.h"

/*!\ingroup HMAC_sha1_m
 */
typedef struct
{
	sha1Param sparam;
	byte kxi[64];
	byte kxo[64];
} hmacsha1Param;

#ifdef __cplusplus
extern "C" {
#endif

extern BEECRYPTAPI const keyedHashFunction hmacsha1;

BEECRYPTAPI
int hmacsha1Setup (hmacsha1Param*, const byte*, size_t);
BEECRYPTAPI
int hmacsha1Reset (hmacsha1Param*);
BEECRYPTAPI
int hmacsha1Update(hmacsha1Param*, const byte*, size_t);
BEECRYPTAPI
int hmacsha1Digest(hmacsha1Param*, byte*);

#ifdef __cplusplus
}
#endif

#endif
