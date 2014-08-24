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
#ifndef _ENDIAN_H_
#define _ENDIAN_H_

#ifndef _RBENDIAN_H_
/* this only defines what may be substituted in the native endian.h */
#error "Include rbendian.h instead."
#endif

#define __ENDIAN_H_NATIVE_RB

#ifdef NEED_GENERIC_BYTESWAPS
static inline uint16_t swap16_hw(uint16_t value)
    /*
     * result[15..8] = value[ 7..0];
     * result[ 7..0] = value[15..8];
     */
{
    return (value >> 8) | (value << 8);
}

static inline uint32_t swap32_hw(uint32_t value)
    /*
     * result[31..24] = value[ 7.. 0];
     * result[23..16] = value[15.. 8];
     * result[15.. 8] = value[23..16];
     * result[ 7.. 0] = value[31..24];
     */
{
    uint32_t hi = swap16_hw(value >> 16);
    uint32_t lo = swap16_hw(value & 0xffff);
    return (lo << 16) | hi;
}
#endif /* NEED_GENERIC_BYTESWAPS */

static FORCE_INLINE uint64_t swap64_hw(uint64_t x)
  { return swap32_hw((uint32_t)(x >> 32)) |
           ((uint64_t)swap32_hw((uint32_t)x) << 32); }

#endif /* _ENDIAN_H_ */
