/*
 * endianness.h
 *
 * Endian-dependant encoding/decoding, header
 *
 * Copyright (c) 1998, 1999, 2000, 2001, 2004 Beeyond Software Holding
 *
 * Author: Bob Deblier <bob.deblier@telenet.be>
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

#ifndef _ENDIANNESS_H
#define _ENDIANNESS_H

#include "beecrypt/beecrypt.h"

#if defined(__cplusplus) || HAVE_INLINE

static inline int16_t _swap16(int16_t n)
{
	return (    ((n & 0xff) << 8) |
				((n & 0xff00) >> 8) );
}
# define swap16(n) _swap16(n)

static inline uint16_t _swapu16(uint16_t n)
{
	return (    ((n & 0xffU) << 8) |
				((n & 0xff00U) >> 8) );
}
# define swapu16(n) _swap16(n)

# ifdef __arch__swab32
#  define swap32(n) __arch__swab32(n)
#  define swapu32(n) __arch__swab32(n)
# else

static inline int32_t _swap32(int32_t n)
{
	return (    ((n & 0xff) << 24) |
				((n & 0xff00) << 8) |
				((n & 0xff0000) >> 8) |
				((n & 0xff000000) >> 24) );
}
#  define swap32(n) _swap32(n)

static inline uint32_t _swapu32(uint32_t n)
{
	return (    ((n & 0xffU) << 24) |
				((n & 0xff00U) << 8) |
				((n & 0xff0000U) >> 8) |
				((n & 0xff000000U) >> 24) );
}
#  define swapu32(n) _swapu32(n)

# endif

# ifdef __arch__swab64
#  define swap64(n) __arch__swab64(n)
#  define swapu64(n) __arch__swab64(n)
# else

static inline int64_t _swap64(int64_t n)
{
	return (    ((n & ((int64_t) 0xff)      ) << 56) |
				((n & ((int64_t) 0xff) <<  8) << 40) |
				((n & ((int64_t) 0xff) << 16) << 24) |
				((n & ((int64_t) 0xff) << 24) <<  8) |
				((n & ((int64_t) 0xff) << 32) >>  8) |
				((n & ((int64_t) 0xff) << 40) >> 24) |
				((n & ((int64_t) 0xff) << 48) >> 40) |
				((n & ((int64_t) 0xff) << 56) >> 56) );
}
#  define swap64(n) _swap64(n)

static inline uint64_t _swapu64(uint64_t n)
{
	return (    ((n & ((uint64_t) 0xff)      ) << 56) |
				((n & ((uint64_t) 0xff) <<  8) << 40) |
				((n & ((uint64_t) 0xff) << 16) << 24) |
				((n & ((uint64_t) 0xff) << 24) <<  8) |
				((n & ((uint64_t) 0xff) << 32) >>  8) |
				((n & ((uint64_t) 0xff) << 40) >> 24) |
				((n & ((uint64_t) 0xff) << 48) >> 40) |
				((n & ((uint64_t) 0xff) << 56) >> 56) );
}
#  define swapu64(n) _swapu64(n)

# endif

#else
BEECRYPTAPI
 int16_t swap16 (int16_t);
BEECRYPTAPI
uint16_t swapu16(uint16_t);
BEECRYPTAPI
 int32_t swap32 (int32_t);
BEECRYPTAPI
uint32_t swapu32(uint32_t);
BEECRYPTAPI
 int64_t swap64 (int64_t);
BEECRYPTAPI
uint64_t swapu64(uint64_t);
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif
