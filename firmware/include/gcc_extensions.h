/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright © 2010 Rafaël Carré
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

#ifndef _GCC_EXTENSIONS_H_
#define _GCC_EXTENSIONS_H_

/* Support for some GCC extensions */

/* Compile time check of format for printf/scanf like functions */
#ifdef __GNUC__
#define ATTRIBUTE_PRINTF(fmt, arg1) __attribute__( ( format( printf, fmt, arg1 ) ) )
#define ATTRIBUTE_SCANF(fmt, arg1) __attribute__( ( format( scanf, fmt, arg1 ) ) )
#else
#define ATTRIBUTE_PRINTF(fmt, arg1)
#define ATTRIBUTE_SCANF(fmt, arg1)
#endif


/* Use to give gcc hints on which branch is most likely taken */
#if defined(__GNUC__) && __GNUC__ >= 3
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif


#if defined(__GNUC__) && (__GNUC__ >= 3 || \
                    (__GNUC__ >= 2 && __GNUC_MINOR__ >= 5))
#define NORETURN_ATTR __attribute__((noreturn))
#else
#define NORETURN_ATTR
#endif


#if defined(__GNUC__)
#define FORCE_INLINE inline __attribute__((always_inline))
#else
#define FORCE_INLINE inline
#endif

#if defined(__GNUC__)
#define NO_INLINE __attribute__((noinline))
#else
#define NO_INLINE
#endif

/* Version information from http://ohse.de/uwe/articles/gcc-attributes.html */
#if defined(__GNUC__) && (__GNUC__ >= 4 || \
                    (__GNUC__ >= 3 && __GNUC_MINOR__ >= 1))
#define USED_ATTR __attribute__((used))
#else
#define USED_ATTR
#endif

#if defined(__GNUC__) && (__GNUC__ >= 3)
#define UNUSED_ATTR __attribute__((unused))
#else
#define UNUSED_ATTR
#endif


#endif /* _GCC_EXTENSIONS_H_ */
