/* Raw - Another World Interpreter
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __SYS_H__
#define __SYS_H__

#include "rbendian.h"
#include "stdint.h"


#ifdef ROCKBOX_LITTLE_ENDIAN
#define SYS_LITTLE_ENDIAN
#else
#define SYS_BIG_ENDIAN
#endif


#if defined SYS_LITTLE_ENDIAN
//#warning "LITTLE ENDIAN SELECTED"
#define READ_BE_UINT16(p) ((((const uint8_t*)p)[0] << 8) | ((const uint8_t*)p)[1])
#define READ_BE_UINT32(p) ((((const uint8_t*)p)[0] << 24) | (((const uint8_t*)p)[1] << 16) | (((const uint8_t*)p)[2] << 8) | ((const uint8_t*)p)[3])

/*
inline uint32_t READ_BE_UINT32(const void *ptr) {
    const uint8_t *b = (const uint8_t *)ptr;
    return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

inline uint16_t READ_BE_UINT16(const void *ptr) {
    const uint8_t *b = (const uint8_t *)ptr;
    return (b[0] << 8) | b[1];
}
*/
#elif defined SYS_BIG_ENDIAN
//#warning "BIG ENDIAN SELECTED"
#define READ_BE_UINT16(p) (*(const uint16_t*)p)
#define READ_BE_UINT32(p) (*(const uint32_t*)p)
/*
inline uint16_t READ_BE_UINT16(const void *ptr) {
    return *(const uint16_t *)ptr;
}

inline uint32_t READ_BE_UINT32(const void *ptr) {
    return *(const uint32_t *)ptr;
}
*/
#else

#error No endianness defined

#endif

#endif
