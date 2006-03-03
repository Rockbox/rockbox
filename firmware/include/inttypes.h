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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __INTTYPES_H__ 
#define __INTTYPES_H__ 

#include <limits.h>

/* 8 bit */
#define int8_t signed char
#define uint8_t unsigned char

/* 16 bit */
#if USHRT_MAX == 0xffff
#define int16_t short
#define uint16_t unsigned short
#endif

/* 32 bit */
#if ULONG_MAX == 0xfffffffful
#define int32_t long
#define uint32_t unsigned long
#elif UINT_MAX == 0xffffffffu
#define int32_t int
#define uint32_t unsigned int
#endif

/* 64 bit */
#if ULONG_MAX == 0xffffffffffffffffull
#define int64_t long
#define uint64_t unsigned long
#else
#define int64_t long long
#define uint64_t unsigned long long
#endif

#endif /* __INTTYPES_H__ */
