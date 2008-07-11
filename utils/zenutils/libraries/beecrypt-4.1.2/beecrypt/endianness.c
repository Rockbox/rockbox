/*
 * Copyright (c) 1998, 1999, 2000, 2001 Virtual Unlimited B.V.
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

/*!\file endianness.c
 * \brief Endian-dependant encoding/decoding.
 * \author Bob Deblier <bob.deblier@pandora.be>
 */

#define BEECRYPT_DLL_EXPORT

#if HAVE_CONFIG_H
# include "config.h"
#endif

#if HAVE_ENDIAN_H && HAVE_ASM_BYTEORDER_H
# include <endian.h>
#endif

#include "beecrypt/endianness.h"

#undef swap16
#undef swapu16
#undef swap32
#undef swapu32
#undef swap64
#undef swapu64

int16_t swap16(int16_t n)
{
	return (    ((n & 0xff) << 8) |
				((n & 0xff00) >> 8) );
}

uint16_t swapu16(uint16_t n)
{
	return (    ((n & 0xffU) << 8) |
				((n & 0xff00U) >> 8) );
}

int32_t swap32(int32_t n)
{
	return (    ((n & 0xff) << 24) |
				((n & 0xff00) << 8) |
				((n & 0xff0000) >> 8) |
				((n & 0xff000000) >> 24) );
}

uint32_t swapu32(uint32_t n)
{
	return (    ((n & 0xffU) << 24) |
				((n & 0xff00U) << 8) |
				((n & 0xff0000U) >> 8) |
				((n & 0xff000000U) >> 24) );
}

int64_t swap64(int64_t n)
{
	return (    ((n & (((int64_t) 0xff)      )) << 56) |
				((n & (((int64_t) 0xff) <<  8)) << 40) |
				((n & (((int64_t) 0xff) << 16)) << 24) |
				((n & (((int64_t) 0xff) << 24)) <<  8) |
				((n & (((int64_t) 0xff) << 32)) >>  8) |
				((n & (((int64_t) 0xff) << 40)) >> 24) |
				((n & (((int64_t) 0xff) << 48)) >> 40) |
				((n & (((int64_t) 0xff) << 56)) >> 56) );
}

uint64_t swapu64(uint64_t n)
{
	return (    ((n & (((uint64_t) 0xff)      )) << 56) |
				((n & (((uint64_t) 0xff) <<  8)) << 40) |
				((n & (((uint64_t) 0xff) << 16)) << 24) |
				((n & (((uint64_t) 0xff) << 24)) <<  8) |
				((n & (((uint64_t) 0xff) << 32)) >>  8) |
				((n & (((uint64_t) 0xff) << 40)) >> 24) |
				((n & (((uint64_t) 0xff) << 48)) >> 40) |
				((n & (((uint64_t) 0xff) << 56)) >> 56) );
}
