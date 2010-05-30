/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Dave Chapman
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
 
#ifndef __INTTYPES_H__
#define __INTTYPES_H__

#include <stdint.h>

/* could possibly have (f)scanf format specifiers here */

/* 8 bit */
#define PRId8   "d"
#define PRIi8   "d"
#define PRIu8   "u"
#define PRIx8   "x"
#define PRIX8   "X"

/* 16 bit */
#if USHRT_MAX == 0xffff

#define PRId16  "d"
#define PRIi16  "d"
#define PRIu16  "u"
#define PRIx16  "x"
#define PRIX16  "X"

#endif

/* 32 bit */
#if ULONG_MAX == 0xfffffffful

#define PRId32  "ld"
#define PRIi32  "ld"
#define PRIu32  "lu"
#define PRIx32  "lx"
#define PRIX32  "lX"
#define PRIdPTR "ld"
#define PRIiPTR "ld"
#define PRIuPTR "lu"
#define PRIxPTR "lx"
#define PRIXPTR "lX"

#elif UINT_MAX == 0xffffffffu

#define PRId32  "d"
#define PRIi32  "d"
#define PRIu32  "u"
#define PRIx32  "x"
#define PRIX32  "X"

#endif

/* 64 bit */
#if ULONG_MAX == 0xffffffffffffffffull

#define PRId64  "ld"
#define PRIi64  "ld"
#define PRIu64  "lu"
#define PRIx64  "lx"
#define PRIX64  "lX"
#define PRIdPTR "ld"
#define PRIiPTR "ld"
#define PRIuPTR "lu"
#define PRIxPTR "lx"
#define PRIXPTR "lX"


#else

#define PRId64  "lld"
#define PRIi64  "lld"
#define PRIu64  "llu"
#define PRIx64  "llx"
#define PRIX64  "llX"

#endif

#endif /* __INTTYPES_H__ */
