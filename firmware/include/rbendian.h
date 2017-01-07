/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
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
#ifndef _RBENDIAN_H_
#define _RBENDIAN_H_

#include "config.h"

#ifndef __MINGW32__
#include <endian.h>
#endif

/* clear these out since we redefine them to be truely constant compatible */
#undef swap16
#undef swap32
#undef swap64

#undef letoh16
#undef letoh32
#undef letoh64
#undef htole16
#undef htole32
#undef htole64
#undef betoh16
#undef betoh32
#undef betoh64
#undef htobe16
#undef htobe32
#undef htobe64

/* static/generic endianness conversion */
#define SWAP16_CONST(x) \
    ((typeof(x))( ((uint16_t)(x) >> 8) | ((uint16_t)(x) << 8) ))

#define SWAP32_CONST(x) \
    ((typeof(x))( (((uint32_t)(x) & 0xff000000) >> 24) | \
                  (((uint32_t)(x) & 0x00ff0000) >>  8) | \
                  (((uint32_t)(x) & 0x0000ff00) <<  8) | \
                  (((uint32_t)(x) & 0x000000ff) << 24) ))

#define SWAP64_CONST(x) \
    ((typeof(x))( (((uint64_t)(x) & 0xff00000000000000ull) >> 56) | \
                  (((uint64_t)(x) & 0x00ff000000000000ull) >> 40) | \
                  (((uint64_t)(x) & 0x0000ff0000000000ull) >> 24) | \
                  (((uint64_t)(x) & 0x000000ff00000000ull) >>  8) | \
                  (((uint64_t)(x) & 0x00000000ff000000ull) <<  8) | \
                  (((uint64_t)(x) & 0x0000000000ff0000ull) << 24) | \
                  (((uint64_t)(x) & 0x000000000000ff00ull) << 40) | \
                  (((uint64_t)(x) & 0x00000000000000ffull) << 56) ))

#define SWAP_ODD_EVEN32_CONST(x) \
    ((typeof(x))( ((uint32_t)SWAP16_CONST((uint32_t)(x) >> 16) << 16) | \
                             SWAP16_CONST((uint32_t)(x))) )

#define SWAW32_CONST(x) \
    ((typeof(x))( ((uint32_t)(x) << 16) | ((uint32_t)(x) >> 16) ))


#ifndef __ENDIAN_H_NATIVE_RB

#if defined (__bswap_16)
  #define __swap16_os(x)    __bswap_16(x)
  #define __swap32_os(x)    __bswap_32(x)
  #define __swap64_os(x)    __bswap_64(x)
#elif defined (bswap_16)
  #define __swap16_os(x)    bswap_16(x)
  #define __swap32_os(x)    bswap_32(x)
  #define __swap64_os(x)    bswap_64(x)
#elif defined (__swap16)
  #define __swap16_os(x)    __swap16(x)
  #define __swap32_os(x)    __swap32(x)
  #define __swap64_os(x)    __swap64(x)
#elif defined (swap16)
  #define __swap16_os(x)    swap16(x)
  #define __swap32_os(x)    swap32(x)
  #define __swap64_os(x)    swap64(x)
#elif defined (__MINGW32__) || (CONFIG_PLATFORM & PLATFORM_MAEMO)
  /* kinda hacky but works */
  #define __swap16_os(x)    SWAP16_CONST(x)
  #define __swap32_os(x)    SWAP32_CONST(x)
  #define __swap64_os(x)    SWAP64_CONST(x)
#else
  #error "Missing OS swap defines."
#endif

/* wrap these because they aren't always compatible with compound initializers */
static FORCE_INLINE uint16_t swap16_hw(uint16_t x)
  { return __swap16_os(x); }
static FORCE_INLINE uint32_t swap32_hw(uint32_t x)
  { return __swap32_os(x); }
static FORCE_INLINE uint64_t swap64_hw(uint64_t x)
  { return __swap64_os(x); }

#endif /* __ENDIAN_H_NATIVE_RB */

#if defined(NEED_GENERIC_BYTESWAPS) || !defined(__ENDIAN_H_NATIVE_RB)
/* these are uniquely ours it seems */
static inline uint32_t swap_odd_even32_hw(uint32_t value)
    /*
     * result[31..24],[15.. 8] = value[23..16],[ 7.. 0]
     * result[23..16],[ 7.. 0] = value[31..24],[15.. 8]
     */
{
    uint32_t t = value & 0xff00ff00;
    return (t >> 8) | ((t ^ value) << 8);
}

static inline uint32_t swaw32_hw(uint32_t value)
    /*
     * result[31..16] = value[15.. 0];
     * result[15.. 0] = value[31..16];
     */
{
    return (value >> 16) | (value << 16);
}
#endif /* Generic */

/* select best method based upon whether x is a constant expression */
#define swap16(x) \
    ( __builtin_constant_p(x) ? SWAP16_CONST(x) : (typeof(x))swap16_hw(x) )

#define swap32(x) \
    ( __builtin_constant_p(x) ? SWAP32_CONST(x) : (typeof(x))swap32_hw(x) )

#define swap64(x) \
    ( __builtin_constant_p(x) ? SWAP64_CONST(x) : (typeof(x))swap64_hw(x) )

#define swap_odd_even32(x) \
    ( __builtin_constant_p(x) ? SWAP_ODD_EVEN32_CONST(x) : (typeof(x))swap_odd_even32_hw(x) )

#define swaw32(x) \
    ( __builtin_constant_p(x) ? SWAW32_CONST(x) : (typeof(x))swaw32_hw(x) )

#if defined(ROCKBOX_LITTLE_ENDIAN)
  #define letoh16(x) (x)
  #define letoh32(x) (x)
  #define letoh64(x) (x)
  #define htole16(x) (x)
  #define htole32(x) (x)
  #define htole64(x) (x)
  #define betoh16(x) swap16(x)
  #define betoh32(x) swap32(x)
  #define betoh64(x) swap64(x)
  #define htobe16(x) swap16(x)
  #define htobe32(x) swap32(x)
  #define htobe64(x) swap64(x)
  #define swap_odd_even_be32(x) (x)
  #define swap_odd_even_le32(x) swap_odd_even32(x)
#elif defined(ROCKBOX_BIG_ENDIAN)
  #define letoh16(x) swap16(x)
  #define letoh32(x) swap32(x)
  #define letoh64(x) swap64(x)
  #define htole16(x) swap16(x)
  #define htole32(x) swap32(x)
  #define htole64(x) swap64(x)
  #define betoh16(x) (x)
  #define betoh32(x) (x)
  #define betoh64(x) (x)
  #define htobe16(x) (x)
  #define htobe32(x) (x)
  #define htobe64(x) (x)
  #define swap_odd_even_be32(x) swap_odd_even32(x)
  #define swap_odd_even_le32(x) (x)
#else
  #error "Unknown endianness!"
#endif

#endif /* _RBENDIAN_H_ */
