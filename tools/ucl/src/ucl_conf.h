/* ucl_conf.h -- main internal configuration file for the the UCL library

   This file is part of the UCL data compression library.

   Copyright (C) 1996-2002 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   The UCL library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The UCL library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the UCL library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
 */


/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the library and is subject
   to change.
 */


#ifndef __UCL_CONF_H
#define __UCL_CONF_H

#if !defined(__UCL_IN_MINIUCL)
#  ifndef __UCLCONF_H
#    include <ucl/uclconf.h>
#  endif
#endif


/***********************************************************************
// memory checkers
************************************************************************/

#if defined(__BOUNDS_CHECKING_ON)
#  include <unchecked.h>
#else
#  define BOUNDS_CHECKING_OFF_DURING(stmt)      stmt
#  define BOUNDS_CHECKING_OFF_IN_EXPR(expr)     (expr)
#endif


/***********************************************************************
// autoconf section
************************************************************************/

#if !defined(UCL_HAVE_CONFIG_H)
#  include <stddef.h>           /* ptrdiff_t, size_t */
#  include <string.h>           /* memcpy, memmove, memcmp, memset */
#  if !defined(NO_STDLIB_H)
#    include <stdlib.h>
#  endif
#  define HAVE_MEMCMP
#  define HAVE_MEMCPY
#  define HAVE_MEMMOVE
#  define HAVE_MEMSET
#else
#  include <sys/types.h>
#  if defined(STDC_HEADERS)
#    include <string.h>
#    include <stdlib.h>
#  endif
#  if defined(HAVE_STDDEF_H)
#    include <stddef.h>
#  endif
#  if defined(HAVE_MEMORY_H)
#    include <memory.h>
#  endif
#endif

#if defined(__UCL_DOS16) || defined(__UCL_WIN16)
#  define HAVE_MALLOC_H
#  define HAVE_HALLOC
#endif


#undef NDEBUG
#if !defined(UCL_DEBUG)
#  define NDEBUG
#endif
#if 1 || defined(UCL_DEBUG) || !defined(NDEBUG)
#  if !defined(NO_STDIO_H)
#    include <stdio.h>
#  endif
#endif
#include <assert.h>


#if !defined(UCL_UNUSED)
#  define UCL_UNUSED(parm)  (parm = parm)
#endif


#if !defined(__inline__) && !defined(__GNUC__)
#  if defined(__cplusplus)
#    define __inline__      inline
#  else
#    define __inline__      /* nothing */
#  endif
#endif


/***********************************************************************
//
************************************************************************/

#if 1
#  define UCL_BYTE(x)       ((unsigned char) (x))
#else
#  define UCL_BYTE(x)       ((unsigned char) ((x) & 0xff))
#endif
#if 0
#  define UCL_USHORT(x)     ((unsigned short) (x))
#else
#  define UCL_USHORT(x)     ((unsigned short) ((x) & 0xffff))
#endif

#define UCL_MAX(a,b)        ((a) >= (b) ? (a) : (b))
#define UCL_MIN(a,b)        ((a) <= (b) ? (a) : (b))
#define UCL_MAX3(a,b,c)     ((a) >= (b) ? UCL_MAX(a,c) : UCL_MAX(b,c))
#define UCL_MIN3(a,b,c)     ((a) <= (b) ? UCL_MIN(a,c) : UCL_MIN(b,c))

#define ucl_sizeof(type)    ((ucl_uint) (sizeof(type)))

#define UCL_HIGH(array)     ((ucl_uint) (sizeof(array)/sizeof(*(array))))

/* this always fits into 16 bits */
#define UCL_SIZE(bits)      (1u << (bits))
#define UCL_MASK(bits)      (UCL_SIZE(bits) - 1)

#define UCL_LSIZE(bits)     (1ul << (bits))
#define UCL_LMASK(bits)     (UCL_LSIZE(bits) - 1)

#define UCL_USIZE(bits)     ((ucl_uint) 1 << (bits))
#define UCL_UMASK(bits)     (UCL_USIZE(bits) - 1)

/* Maximum value of a signed/unsigned type.
   Do not use casts, avoid overflows ! */
#define UCL_STYPE_MAX(b)    (((1l  << (8*(b)-2)) - 1l)  + (1l  << (8*(b)-2)))
#define UCL_UTYPE_MAX(b)    (((1ul << (8*(b)-1)) - 1ul) + (1ul << (8*(b)-1)))


/***********************************************************************
//
************************************************************************/

#if !defined(SIZEOF_UNSIGNED)
#  if (UINT_MAX == 0xffff)
#    define SIZEOF_UNSIGNED         2
#  elif (UINT_MAX == UCL_0xffffffffL)
#    define SIZEOF_UNSIGNED         4
#  elif (UINT_MAX >= UCL_0xffffffffL)
#    define SIZEOF_UNSIGNED         8
#  else
#    error "SIZEOF_UNSIGNED"
#  endif
#endif

#if !defined(SIZEOF_UNSIGNED_LONG)
#  if (ULONG_MAX == UCL_0xffffffffL)
#    define SIZEOF_UNSIGNED_LONG    4
#  elif (ULONG_MAX >= UCL_0xffffffffL)
#    define SIZEOF_UNSIGNED_LONG    8
#  else
#    error "SIZEOF_UNSIGNED_LONG"
#  endif
#endif


#if !defined(SIZEOF_SIZE_T)
#  define SIZEOF_SIZE_T             SIZEOF_UNSIGNED
#endif
#if !defined(SIZE_T_MAX)
#  define SIZE_T_MAX                UCL_UTYPE_MAX(SIZEOF_SIZE_T)
#endif


/***********************************************************************
// <string.h> section
************************************************************************/

#if defined(NO_MEMCMP)
#  undef HAVE_MEMCMP
#endif

#if (UCL_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMCMP)
#  define ucl_memcmp                memcmp
#endif
#if (UCL_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMCPY)
#  define ucl_memcpy                memcpy
#endif
#if (UCL_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMMOVE)
#  define ucl_memmove               memmove
#endif
#if (UCL_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMSET)
#  define ucl_memset                memset
#endif

#if !defined(HAVE_MEMCMP)
#  undef memcmp
#  define memcmp                    ucl_memcmp
#endif
#if !defined(HAVE_MEMCPY)
#  undef memcpy
#  define memcpy                    ucl_memcpy
#endif
#if !defined(HAVE_MEMMOVE)
#  undef memmove
#  define memmove                   ucl_memmove
#endif
#if !defined(HAVE_MEMSET)
#  undef memset
#  define memset                    ucl_memset
#endif


/***********************************************************************
// compiler and architecture specific stuff
************************************************************************/

/* Some defines that indicate if memory can be accessed at unaligned
 * memory addresses. You should also test that this is actually faster
 * even if it is allowed by your system.
 */

#if 1 && defined(__UCL_i386) && (UINT_MAX == UCL_0xffffffffL)
#  if !defined(UCL_UNALIGNED_OK_2) && (USHRT_MAX == 0xffff)
#    define UCL_UNALIGNED_OK_2
#  endif
#  if !defined(UCL_UNALIGNED_OK_4) && (UCL_UINT32_MAX == UCL_0xffffffffL)
#    define UCL_UNALIGNED_OK_4
#  endif
#endif

#if defined(UCL_UNALIGNED_OK_2) || defined(UCL_UNALIGNED_OK_4)
#  if !defined(UCL_UNALIGNED_OK)
#    define UCL_UNALIGNED_OK
#  endif
#endif

#if defined(__UCL_NO_UNALIGNED)
#  undef UCL_UNALIGNED_OK
#  undef UCL_UNALIGNED_OK_2
#  undef UCL_UNALIGNED_OK_4
#endif

#if defined(UCL_UNALIGNED_OK_2) && (USHRT_MAX != 0xffff)
#  error "UCL_UNALIGNED_OK_2 must not be defined on this system"
#endif
#if defined(UCL_UNALIGNED_OK_4) && (UCL_UINT32_MAX != UCL_0xffffffffL)
#  error "UCL_UNALIGNED_OK_4 must not be defined on this system"
#endif


/* Many modern processors can transfer 32bit words much faster than
 * bytes - this can significantly speed decompression.
 */

#if defined(__UCL_NO_ALIGNED)
#  undef UCL_ALIGNED_OK_4
#endif

#if defined(UCL_ALIGNED_OK_4) && (UCL_UINT32_MAX != UCL_0xffffffffL)
#  error "UCL_ALIGNED_OK_4 must not be defined on this system"
#endif


/* Definitions for byte order, according to significance of bytes, from low
 * addresses to high addresses. The value is what you get by putting '4'
 * in the most significant byte, '3' in the second most significant byte,
 * '2' in the second least significant byte, and '1' in the least
 * significant byte.
 * The byte order is only needed if we use UCL_UNALIGNED_OK.
 */

#define UCL_LITTLE_ENDIAN       1234
#define UCL_BIG_ENDIAN          4321
#define UCL_PDP_ENDIAN          3412

#if !defined(UCL_BYTE_ORDER)
#  if defined(MFX_BYTE_ORDER)
#    define UCL_BYTE_ORDER      MFX_BYTE_ORDER
#  elif defined(__UCL_i386)
#    define UCL_BYTE_ORDER      UCL_LITTLE_ENDIAN
#  elif defined(BYTE_ORDER)
#    define UCL_BYTE_ORDER      BYTE_ORDER
#  elif defined(__BYTE_ORDER)
#    define UCL_BYTE_ORDER      __BYTE_ORDER
#  endif
#endif

#if defined(UCL_BYTE_ORDER)
#  if (UCL_BYTE_ORDER != UCL_LITTLE_ENDIAN) && \
      (UCL_BYTE_ORDER != UCL_BIG_ENDIAN)
#    error "invalid UCL_BYTE_ORDER"
#  endif
#endif

#if defined(UCL_UNALIGNED_OK) && !defined(UCL_BYTE_ORDER)
#  error "UCL_BYTE_ORDER is not defined"
#endif


/***********************************************************************
// some globals
************************************************************************/

__UCL_EXTERN_C int __ucl_init_done;
__UCL_EXTERN_C const ucl_byte __ucl_copyright[];
UCL_EXTERN(const ucl_byte *) ucl_copyright(void);
__UCL_EXTERN_C const ucl_uint32 _ucl_crc32_table[256];


/***********************************************************************
// ANSI C preprocessor macros
************************************************************************/

#define _UCL_STRINGIZE(x)           #x
#define _UCL_MEXPAND(x)             _UCL_STRINGIZE(x)

/* concatenate */
#define _UCL_CONCAT2(a,b)           a ## b
#define _UCL_CONCAT3(a,b,c)         a ## b ## c
#define _UCL_CONCAT4(a,b,c,d)       a ## b ## c ## d
#define _UCL_CONCAT5(a,b,c,d,e)     a ## b ## c ## d ## e

/* expand and concatenate (by using one level of indirection) */
#define _UCL_ECONCAT2(a,b)          _UCL_CONCAT2(a,b)
#define _UCL_ECONCAT3(a,b,c)        _UCL_CONCAT3(a,b,c)
#define _UCL_ECONCAT4(a,b,c,d)      _UCL_CONCAT4(a,b,c,d)
#define _UCL_ECONCAT5(a,b,c,d,e)    _UCL_CONCAT5(a,b,c,d,e)


/***********************************************************************
//
************************************************************************/

#include "ucl_ptr.h"


#endif /* already included */

/*
vi:ts=4:et
*/

