/* ucl_ptr.h -- low-level pointer constructs

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


#ifndef __UCL_PTR_H
#define __UCL_PTR_H

#ifdef __cplusplus
extern "C" {
#endif


/* This is the lowest part of the UCL library.
 * It deals with pointer representations at bit level.
 */


/***********************************************************************
// Includes
************************************************************************/

#if defined(__UCL_DOS16) || defined(__UCL_WIN16)
#  include <dos.h>
#  if 1 && defined(__WATCOMC__)
#    include <i86.h>
     __UCL_EXTERN_C unsigned char _HShift;
#    define __UCL_HShift    _HShift
#  elif 1 && defined(_MSC_VER)
     __UCL_EXTERN_C unsigned short __near _AHSHIFT;
#    define __UCL_HShift    ((unsigned) &_AHSHIFT)
#  elif defined(__UCL_WIN16)
#    define __UCL_HShift    3
#  else
#    define __UCL_HShift    12
#  endif
#  if !defined(_FP_SEG) && defined(FP_SEG)
#    define _FP_SEG         FP_SEG
#  endif
#  if !defined(_FP_OFF) && defined(FP_OFF)
#    define _FP_OFF         FP_OFF
#  endif
#endif


/***********************************************************************
// Integral types
************************************************************************/

/* ptrdiff_t */
#if !defined(ucl_ptrdiff_t)
  #if (UINT_MAX >= UCL_0xffffffffL)
     typedef ptrdiff_t          ucl_ptrdiff_t;
  #else
     typedef long               ucl_ptrdiff_t;
  #endif
#endif


/* Unsigned type that has *exactly* the same number of bits as a ucl_voidp */
#if !defined(__UCL_HAVE_PTR_T)
#  if defined(ucl_ptr_t)
#    define __UCL_HAVE_PTR_T
#  endif
#endif
#if !defined(__UCL_HAVE_PTR_T)
#  if defined(SIZEOF_CHAR_P) && defined(SIZEOF_UNSIGNED_LONG)
#    if (SIZEOF_CHAR_P == SIZEOF_UNSIGNED_LONG)
       typedef unsigned long    ucl_ptr_t;
       typedef long             ucl_sptr_t;
#      define __UCL_HAVE_PTR_T
#    endif
#  endif
#endif
#if !defined(__UCL_HAVE_PTR_T)
#  if defined(SIZEOF_CHAR_P) && defined(SIZEOF_UNSIGNED)
#    if (SIZEOF_CHAR_P == SIZEOF_UNSIGNED)
       typedef unsigned int     ucl_ptr_t;
       typedef int              ucl_sptr_t;
#      define __UCL_HAVE_PTR_T
#    endif
#  endif
#endif
#if !defined(__UCL_HAVE_PTR_T)
#  if defined(SIZEOF_CHAR_P) && defined(SIZEOF_UNSIGNED_SHORT)
#    if (SIZEOF_CHAR_P == SIZEOF_UNSIGNED_SHORT)
       typedef unsigned short   ucl_ptr_t;
       typedef short            ucl_sptr_t;
#      define __UCL_HAVE_PTR_T
#    endif
#  endif
#endif
#if !defined(__UCL_HAVE_PTR_T)
#  if defined(UCL_HAVE_CONFIG_H) || defined(SIZEOF_CHAR_P)
#    error "no suitable type for ucl_ptr_t"
#  else
     typedef unsigned long      ucl_ptr_t;
     typedef long               ucl_sptr_t;
#    define __UCL_HAVE_PTR_T
#  endif
#endif


/***********************************************************************
//
************************************************************************/

/* Always use the safe (=integral) version for pointer-comparisions.
 * The compiler should optimize away the additional casts anyway.
 *
 * Note that this only works if the representation and ordering
 * of the pointer and the integral is the same (at bit level).
 *
 * Most 16-bit compilers have their own view about pointers -
 * fortunately they don't care about comparing pointers
 * that are pointing to Nirvana.
 */

#if defined(__UCL_DOS16) || defined(__UCL_WIN16)
#define PTR(a)              ((ucl_bytep) (a))
/* only need the low bits of the pointer -> offset is ok */
#define PTR_ALIGNED_4(a)    ((_FP_OFF(a) & 3) == 0)
#define PTR_ALIGNED2_4(a,b) (((_FP_OFF(a) | _FP_OFF(b)) & 3) == 0)
#else
#define PTR(a)              ((ucl_ptr_t) (a))
#define PTR_LINEAR(a)       PTR(a)
#define PTR_ALIGNED_4(a)    ((PTR_LINEAR(a) & 3) == 0)
#define PTR_ALIGNED_8(a)    ((PTR_LINEAR(a) & 7) == 0)
#define PTR_ALIGNED2_4(a,b) (((PTR_LINEAR(a) | PTR_LINEAR(b)) & 3) == 0)
#define PTR_ALIGNED2_8(a,b) (((PTR_LINEAR(a) | PTR_LINEAR(b)) & 7) == 0)
#endif

#define PTR_LT(a,b)         (PTR(a) < PTR(b))
#define PTR_GE(a,b)         (PTR(a) >= PTR(b))
#define PTR_DIFF(a,b)       ((ucl_ptrdiff_t) (PTR(a) - PTR(b)))


UCL_EXTERN(ucl_ptr_t)
__ucl_ptr_linear(const ucl_voidp ptr);


typedef union
{
    char            a_char;
    unsigned char   a_uchar;
    short           a_short;
    unsigned short  a_ushort;
    int             a_int;
    unsigned int    a_uint;
    long            a_long;
    unsigned long   a_ulong;
    ucl_int         a_ucl_int;
    ucl_uint        a_ucl_uint;
    ucl_int32       a_ucl_int32;
    ucl_uint32      a_ucl_uint32;
    ptrdiff_t       a_ptrdiff_t;
    ucl_ptrdiff_t   a_ucl_ptrdiff_t;
    ucl_ptr_t       a_ucl_ptr_t;
    ucl_voidp       a_ucl_voidp;
    void *          a_void_p;
    ucl_bytep       a_ucl_bytep;
    ucl_bytepp      a_ucl_bytepp;
    ucl_uintp       a_ucl_uintp;
    ucl_uint *      a_ucl_uint_p;
    ucl_uint32p     a_ucl_uint32p;
    ucl_uint32 *    a_ucl_uint32_p;
    unsigned char * a_uchar_p;
    char *          a_char_p;
}
ucl_align_t;



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */

/*
vi:ts=4:et
*/

