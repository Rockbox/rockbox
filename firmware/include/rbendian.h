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

#ifdef OS_USE_BYTESWAP_H
#include <byteswap.h>
#endif

#ifndef __MINGW32__
#ifdef __APPLE__
#include <sys/types.h>
#else
#include <endian.h>
#endif
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
#else
  /* kinda hacky but works */
  #define __swap16_os(x)    SWAP16_CONST(x)
  #define __swap32_os(x)    SWAP32_CONST(x)
  #define __swap64_os(x)    SWAP64_CONST(x)
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

/*
 * Generic unaligned loads
 */
static inline uint16_t _generic_load_le16(const void* p)
{
    const uint8_t* d = p;
    return d[0] | (d[1] << 8);
}

static inline uint32_t _generic_load_le32(const void* p)
{
    const uint8_t* d = p;
    return d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
}

static inline uint64_t _generic_load_le64(const void* p)
{
    const uint8_t* d = p;
    return (((uint64_t)d[0] << 0)  | ((uint64_t)d[1] << 8)  |
            ((uint64_t)d[2] << 16) | ((uint64_t)d[3] << 24) |
            ((uint64_t)d[4] << 32) | ((uint64_t)d[5] << 40) |
            ((uint64_t)d[6] << 48) | ((uint64_t)d[7] << 56));
}

static inline uint16_t _generic_load_be16(const void* p)
{
    const uint8_t* d = p;
    return (d[0] << 8) | d[1];
}

static inline uint32_t _generic_load_be32(const void* p)
{
    const uint8_t* d = p;
    return (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | d[3];
}

static inline uint64_t _generic_load_be64(const void* p)
{
    const uint8_t* d = p;
    return (((uint64_t)d[0] << 56) | ((uint64_t)d[1] << 48) |
            ((uint64_t)d[2] << 40) | ((uint64_t)d[3] << 32) |
            ((uint64_t)d[4] << 24) | ((uint64_t)d[5] << 16) |
            ((uint64_t)d[6] << 8)  | ((uint64_t)d[7] << 0));
}

static inline void _generic_store_le16(void* p, uint16_t val)
{
    uint8_t* d = p;
    d[0] = val & 0xff;
    d[1] = (val >> 8) & 0xff;
}

static inline void _generic_store_le32(void* p, uint32_t val)
{
    uint8_t* d = p;
    d[0] = val & 0xff;
    d[1] = (val >> 8) & 0xff;
    d[2] = (val >> 16) & 0xff;
    d[3] = (val >> 24) & 0xff;
}

static inline void _generic_store_le64(void* p, uint64_t val)
{
    uint8_t* d = p;
    d[0] = val & 0xff;
    d[1] = (val >> 8) & 0xff;
    d[2] = (val >> 16) & 0xff;
    d[3] = (val >> 24) & 0xff;
    d[4] = (val >> 32) & 0xff;
    d[5] = (val >> 40) & 0xff;
    d[6] = (val >> 48) & 0xff;
    d[7] = (val >> 56) & 0xff;
}

static inline void _generic_store_be16(void* p, uint16_t val)
{
    uint8_t* d = p;
    d[0] = (val >> 8) & 0xff;
    d[1] = val & 0xff;
}

static inline void _generic_store_be32(void* p, uint32_t val)
{
    uint8_t* d = p;
    d[0] = (val >> 24) & 0xff;
    d[1] = (val >> 16) & 0xff;
    d[2] = (val >> 8) & 0xff;
    d[3] = val & 0xff;
}

static inline void _generic_store_be64(void* p, uint64_t val)
{
    uint8_t* d = p;
    d[0] = (val >> 56) & 0xff;
    d[1] = (val >> 48) & 0xff;
    d[2] = (val >> 40) & 0xff;
    d[3] = (val >> 32) & 0xff;
    d[4] = (val >> 24) & 0xff;
    d[5] = (val >> 16) & 0xff;
    d[6] = (val >> 8) & 0xff;
    d[7] = val & 0xff;
}

#if !defined(HAVE_UNALIGNED_LOAD_STORE)

/* Use generic unaligned loads */
#define load_le16 _generic_load_le16
#define load_le32 _generic_load_le32
#define load_le64 _generic_load_le64
#define load_be16 _generic_load_be16
#define load_be32 _generic_load_be32
#define load_be64 _generic_load_be64
#define store_le16 _generic_store_le16
#define store_le32 _generic_store_le32
#define store_le64 _generic_store_le64
#define store_be16 _generic_store_be16
#define store_be32 _generic_store_be32
#define store_be64 _generic_store_be64

/* Define host byte order unaligned load */
#if defined(ROCKBOX_LITTLE_ENDIAN)
# define load_h16 load_le16
# define load_h32 load_le32
# define load_h64 load_le64
# define store_h16 store_le16
# define store_h32 store_le32
# define store_h64 store_le64
#elif defined(ROCKBOX_BIG_ENDIAN)
# define load_h16 load_be16
# define load_h32 load_be32
# define load_h64 load_be64
# define store_h16 store_be16
# define store_h32 store_be32
# define store_h64 store_be64
#else
# error
#endif

#else /* HAVE_UNALIGNED_LOAD_STORE */

/* The arch should define unaligned loads in host byte order */
#if defined(ROCKBOX_LITTLE_ENDIAN)
# define load_le16 load_h16
# define load_le32 load_h32
# define load_le64 load_h64
# define load_be16(p) swap16(load_h16((p)))
# define load_be32(p) swap32(load_h32((p)))
# define load_be64(p) swap64(load_h64((p)))
# define store_le16 store_h16
# define store_le32 store_h32
# define store_le64 store_h64
# define store_be16(p,v) store_h16((p),swap16((v)))
# define store_be32(p,v) store_h32((p),swap32((v)))
# define store_be64(p,v) store_h64((p),swap64((v)))
#elif defined(ROCKBOX_BIG_ENDIAN)
# define load_le16(p) swap16(load_h16((p)))
# define load_le32(p) swap32(load_h32((p)))
# define load_le64(p) swap64(load_h64((p)))
# define load_be16 load_h16
# define load_be32 load_h32
# define load_be64 load_h64
# define store_le16(p,v) store_h16((p),swap16((v)))
# define store_le32(p,v) store_h32((p),swap32((v)))
# define store_le64(p,v) store_h64((p),swap64((v)))
# define store_be16 store_h16
# define store_be32 store_h32
# define store_be64 store_h64
#else
# error
#endif

#endif /* HAVE_UNALIGNED_LOAD_STORE */

/*
 * Aligned loads
 */

static inline uint16_t load_h16_aligned(const void* p)
{
    return *(const uint16_t*)p;
}

static inline uint32_t load_h32_aligned(const void* p)
{
    return *(const uint32_t*)p;
}

static inline uint64_t load_h64_aligned(const void* p)
{
    return *(const uint64_t*)p;
}

static inline void store_h16_aligned(void* p, uint16_t val)
{
    *(uint16_t*)p = val;
}

static inline void store_h32_aligned(void* p, uint32_t val)
{
    *(uint32_t*)p = val;
}

static inline void store_h64_aligned(void* p, uint64_t val)
{
    *(uint64_t*)p = val;
}

#if defined(ROCKBOX_LITTLE_ENDIAN)
# define load_le16_aligned load_h16_aligned
# define load_le32_aligned load_h32_aligned
# define load_le64_aligned load_h64_aligned
# define load_be16_aligned(p) swap16(load_h16_aligned((p)))
# define load_be32_aligned(p) swap32(load_h32_aligned((p)))
# define load_be64_aligned(p) swap64(load_h64_aligned((p)))
# define store_le16_aligned store_h16_aligned
# define store_le32_aligned store_h32_aligned
# define store_le64_aligned store_h64_aligned
# define store_be16_aligned(p,v) store_h16_aligned((p),swap16((v)))
# define store_be32_aligned(p,v) store_h32_aligned((p),swap32((v)))
# define store_be64_aligned(p,v) store_h64_aligned((p),swap64((v)))
#elif defined(ROCKBOX_BIG_ENDIAN)
# define load_le16_aligned(p) swap16(load_h16_aligned((p)))
# define load_le32_aligned(p) swap32(load_h32_aligned((p)))
# define load_le64_aligned(p) swap64(load_h64_aligned((p)))
# define load_be16_aligned load_h16_aligned
# define load_be32_aligned load_h32_aligned
# define load_be64_aligned load_h64_aligned
# define store_le16_aligned(p,v) store_h16_aligned((p),swap16((v)))
# define store_le32_aligned(p,v) store_h32_aligned((p),swap32((v)))
# define store_le64_aligned(p,v) store_h64_aligned((p),swap64((v)))
# define store_be16_aligned store_h16_aligned
# define store_be32_aligned store_h32_aligned
# define store_be64_aligned store_h64_aligned
#else
# error "Unknown endian!"
#endif

#endif /* _RBENDIAN_H_ */
