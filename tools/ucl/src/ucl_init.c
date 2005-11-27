/* ucl_init.c -- initialization of the UCL library

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
   http://www.oberhumer.com/opensource/ucl/
 */


#include "ucl_conf.h"
#include "ucl_util.h"
#include <stdio.h>


#if 0
#  define IS_SIGNED(type)       (((type) (1ul << (8 * sizeof(type) - 1))) < 0)
#  define IS_UNSIGNED(type)     (((type) (1ul << (8 * sizeof(type) - 1))) > 0)
#else
#  define IS_SIGNED(type)       (((type) (-1)) < ((type) 0))
#  define IS_UNSIGNED(type)     (((type) (-1)) > ((type) 0))
#endif


/***********************************************************************
// Runtime check of the assumptions about the size of builtin types,
// memory model, byte order and other low-level constructs.
//
// We are really paranoid here - UCL should either fail (or crash)
// at startup or not at all.
//
// Because of inlining much of these functions evaluates to nothing.
************************************************************************/

static ucl_bool schedule_insns_bug(void);   /* avoid inlining */
static ucl_bool strength_reduce_bug(int *); /* avoid inlining */


#if 0 || defined(UCL_DEBUG)
static ucl_bool __ucl_assert_fail(const char *s, unsigned line)
{
#if defined(__palmos__)
    printf("UCL assertion failed in line %u: '%s'\n",line,s);
#else
    fprintf(stderr,"UCL assertion failed in line %u: '%s'\n",line,s);
#endif
    return 0;
}
#  define __ucl_assert(x)   ((x) ? 1 : __ucl_assert_fail(#x,__LINE__))
#else
#  define __ucl_assert(x)   ((x) ? 1 : 0)
#endif


/***********************************************************************
// The next two functions should get completely optimized out of existance.
// Some assertions are redundant - but included for clarity.
************************************************************************/

static ucl_bool basic_integral_check(void)
{
    ucl_bool r = 1;
    ucl_bool sanity;

    /* paranoia */
    r &= __ucl_assert(CHAR_BIT == 8);
    r &= __ucl_assert(sizeof(char) == 1);
    r &= __ucl_assert(sizeof(short) >= 2);
    r &= __ucl_assert(sizeof(long) >= 4);
    r &= __ucl_assert(sizeof(int) >= sizeof(short));
    r &= __ucl_assert(sizeof(long) >= sizeof(int));

    r &= __ucl_assert(sizeof(ucl_uint32) >= 4);
    r &= __ucl_assert(sizeof(ucl_uint32) >= sizeof(unsigned));
#if defined(__UCL_STRICT_16BIT)
    r &= __ucl_assert(sizeof(ucl_uint) == 2);
#else
    r &= __ucl_assert(sizeof(ucl_uint) >= 4);
    r &= __ucl_assert(sizeof(ucl_uint) >= sizeof(unsigned));
#endif

#if defined(SIZEOF_UNSIGNED)
    r &= __ucl_assert(SIZEOF_UNSIGNED == sizeof(unsigned));
#endif
#if defined(SIZEOF_UNSIGNED_LONG)
    r &= __ucl_assert(SIZEOF_UNSIGNED_LONG == sizeof(unsigned long));
#endif
#if defined(SIZEOF_UNSIGNED_SHORT)
    r &= __ucl_assert(SIZEOF_UNSIGNED_SHORT == sizeof(unsigned short));
#endif
#if !defined(__UCL_IN_MINIUCL)
#if defined(SIZEOF_SIZE_T)
    r &= __ucl_assert(SIZEOF_SIZE_T == sizeof(size_t));
#endif
#endif

    /* assert the signedness of our integral types */
    sanity = IS_UNSIGNED(unsigned short) && IS_UNSIGNED(unsigned) &&
             IS_UNSIGNED(unsigned long) &&
             IS_SIGNED(short) && IS_SIGNED(int) && IS_SIGNED(long);
    if (sanity)
    {
        r &= __ucl_assert(IS_UNSIGNED(ucl_uint32));
        r &= __ucl_assert(IS_UNSIGNED(ucl_uint));
        r &= __ucl_assert(IS_SIGNED(ucl_int32));
        r &= __ucl_assert(IS_SIGNED(ucl_int));

        r &= __ucl_assert(INT_MAX    == UCL_STYPE_MAX(sizeof(int)));
        r &= __ucl_assert(UINT_MAX   == UCL_UTYPE_MAX(sizeof(unsigned)));
        r &= __ucl_assert(LONG_MAX   == UCL_STYPE_MAX(sizeof(long)));
        r &= __ucl_assert(ULONG_MAX  == UCL_UTYPE_MAX(sizeof(unsigned long)));
        r &= __ucl_assert(SHRT_MAX   == UCL_STYPE_MAX(sizeof(short)));
        r &= __ucl_assert(USHRT_MAX  == UCL_UTYPE_MAX(sizeof(unsigned short)));
        r &= __ucl_assert(UCL_UINT32_MAX == UCL_UTYPE_MAX(sizeof(ucl_uint32)));
        r &= __ucl_assert(UCL_UINT_MAX   == UCL_UTYPE_MAX(sizeof(ucl_uint)));
#if !defined(__UCL_IN_MINIUCL)
        r &= __ucl_assert(SIZE_T_MAX     == UCL_UTYPE_MAX(sizeof(size_t)));
#endif
    }

    return r;
}


static ucl_bool basic_ptr_check(void)
{
    ucl_bool r = 1;
    ucl_bool sanity;

    r &= __ucl_assert(sizeof(char *) >= sizeof(int));
    r &= __ucl_assert(sizeof(ucl_byte *) >= sizeof(char *));

    r &= __ucl_assert(sizeof(ucl_voidp) == sizeof(ucl_byte *));
    r &= __ucl_assert(sizeof(ucl_voidp) == sizeof(ucl_voidpp));
    r &= __ucl_assert(sizeof(ucl_voidp) == sizeof(ucl_bytepp));
    r &= __ucl_assert(sizeof(ucl_voidp) >= sizeof(ucl_uint));

    r &= __ucl_assert(sizeof(ucl_ptr_t) == sizeof(ucl_voidp));
    r &= __ucl_assert(sizeof(ucl_ptr_t) >= sizeof(ucl_uint));

    r &= __ucl_assert(sizeof(ucl_ptrdiff_t) >= 4);
    r &= __ucl_assert(sizeof(ucl_ptrdiff_t) >= sizeof(ptrdiff_t));

#if defined(SIZEOF_CHAR_P)
    r &= __ucl_assert(SIZEOF_CHAR_P == sizeof(char *));
#endif
#if defined(SIZEOF_PTRDIFF_T)
    r &= __ucl_assert(SIZEOF_PTRDIFF_T == sizeof(ptrdiff_t));
#endif

    /* assert the signedness of our integral types */
    sanity = IS_UNSIGNED(unsigned short) && IS_UNSIGNED(unsigned) &&
             IS_UNSIGNED(unsigned long) &&
             IS_SIGNED(short) && IS_SIGNED(int) && IS_SIGNED(long);
    if (sanity)
    {
        r &= __ucl_assert(IS_UNSIGNED(ucl_ptr_t));
        r &= __ucl_assert(IS_SIGNED(ucl_ptrdiff_t));
        r &= __ucl_assert(IS_SIGNED(ucl_sptr_t));
    }

    return r;
}


/***********************************************************************
//
************************************************************************/

static ucl_bool ptr_check(void)
{
    ucl_bool r = 1;
    int i;
    char _wrkmem[10 * sizeof(ucl_byte *) + sizeof(ucl_align_t)];
    ucl_bytep wrkmem;
    ucl_bytepp dict;
    unsigned char x[4 * sizeof(ucl_align_t)];
    long d;
    ucl_align_t a;
    ucl_align_t u;

    for (i = 0; i < (int) sizeof(x); i++)
        x[i] = UCL_BYTE(i);

    wrkmem = UCL_PTR_ALIGN_UP((ucl_byte *)_wrkmem,sizeof(ucl_align_t));

#if 0
    dict = (ucl_bytepp) wrkmem;
#else
    /* Avoid a compiler warning on architectures that
     * do not allow unaligned access. */
    u.a_ucl_bytep = wrkmem; dict = u.a_ucl_bytepp;
#endif

    d = (long) ((const ucl_bytep) dict - (const ucl_bytep) _wrkmem);
    r &= __ucl_assert(d >= 0);
    r &= __ucl_assert(d < (long) sizeof(ucl_align_t));

    memset(&a,0xff,sizeof(a));
    r &= __ucl_assert(a.a_ushort == USHRT_MAX);
    r &= __ucl_assert(a.a_uint == UINT_MAX);
    r &= __ucl_assert(a.a_ulong == ULONG_MAX);
    r &= __ucl_assert(a.a_ucl_uint == UCL_UINT_MAX);

    /* sanity check of the memory model */
    if (r == 1)
    {
        for (i = 0; i < 8; i++)
            r &= __ucl_assert((const ucl_voidp) (&dict[i]) == (const ucl_voidp) (&wrkmem[i * sizeof(ucl_byte *)]));
    }

    /* check BZERO8_PTR and that NULL == 0 */
    memset(&a,0,sizeof(a));
    r &= __ucl_assert(a.a_char_p == NULL);
    r &= __ucl_assert(a.a_ucl_bytep == NULL);
    r &= __ucl_assert(NULL == (void*)0);
    if (r == 1)
    {
        for (i = 0; i < 10; i++)
            dict[i] = wrkmem;
        BZERO8_PTR(dict+1,sizeof(dict[0]),8);
        r &= __ucl_assert(dict[0] == wrkmem);
        for (i = 1; i < 9; i++)
            r &= __ucl_assert(dict[i] == NULL);
        r &= __ucl_assert(dict[9] == wrkmem);
    }

    /* check that the pointer constructs work as expected */
    if (r == 1)
    {
        unsigned k = 1;
        const unsigned n = (unsigned) sizeof(ucl_uint32);
        ucl_byte *p0;
        ucl_byte *p1;

        k += __ucl_align_gap(&x[k],n);
        p0 = (ucl_bytep) &x[k];
#if defined(PTR_LINEAR)
        r &= __ucl_assert((PTR_LINEAR(p0) & (n-1)) == 0);
#else
        r &= __ucl_assert(n == 4);
        r &= __ucl_assert(PTR_ALIGNED_4(p0));
#endif

        r &= __ucl_assert(k >= 1);
        p1 = (ucl_bytep) &x[1];
        r &= __ucl_assert(PTR_GE(p0,p1));

        r &= __ucl_assert(k < 1+n);
        p1 = (ucl_bytep) &x[1+n];
        r &= __ucl_assert(PTR_LT(p0,p1));

        /* now check that aligned memory access doesn't core dump */
        if (r == 1)
        {
            ucl_uint32 v0, v1;
#if 0
            v0 = * (ucl_uint32 *) &x[k];
            v1 = * (ucl_uint32 *) &x[k+n];
#else
            /* Avoid compiler warnings on architectures that
             * do not allow unaligned access. */
            u.a_uchar_p = &x[k];
            v0 = *u.a_ucl_uint32_p;
            u.a_uchar_p = &x[k+n];
            v1 = *u.a_ucl_uint32_p;
#endif
            r &= __ucl_assert(v0 > 0);
            r &= __ucl_assert(v1 > 0);
        }
    }

    return r;
}


/***********************************************************************
//
************************************************************************/

UCL_PUBLIC(int)
_ucl_config_check(void)
{
    ucl_bool r = 1;
    int i;
    union {
        ucl_uint32 a;
        unsigned short b;
        ucl_uint32 aa[4];
        unsigned char x[4*sizeof(ucl_align_t)];
    } u;

#if 0
    /* paranoia - the following is guaranteed by definition anyway */
    r &= __ucl_assert((const void *)&u == (const void *)&u.a);
    r &= __ucl_assert((const void *)&u == (const void *)&u.b);
    r &= __ucl_assert((const void *)&u == (const void *)&u.x[0]);
    r &= __ucl_assert((const void *)&u == (const void *)&u.aa[0]);
#endif

    r &= basic_integral_check();
    r &= basic_ptr_check();
    if (r != 1)
        return UCL_E_ERROR;

    u.a = 0; u.b = 0;
    for (i = 0; i < (int) sizeof(u.x); i++)
        u.x[i] = UCL_BYTE(i);

#if 0
    /* check if the compiler correctly casts signed to unsigned */
    r &= __ucl_assert( (int) (unsigned char) ((char) -1) == 255);
#endif

    /* check UCL_BYTE_ORDER */
#if defined(UCL_BYTE_ORDER)
    if (r == 1)
    {
#  if (UCL_BYTE_ORDER == UCL_LITTLE_ENDIAN)
        ucl_uint32 a = (ucl_uint32) (u.a & UCL_UINT32_C(0xffffffff));
        unsigned short b = (unsigned short) (u.b & 0xffff);
        r &= __ucl_assert(a == UCL_UINT32_C(0x03020100));
        r &= __ucl_assert(b == 0x0100);
#  elif (UCL_BYTE_ORDER == UCL_BIG_ENDIAN)
        ucl_uint32 a = u.a >> (8 * sizeof(u.a) - 32);
        unsigned short b = u.b >> (8 * sizeof(u.b) - 16);
        r &= __ucl_assert(a == UCL_UINT32_C(0x00010203));
        r &= __ucl_assert(b == 0x0001);
#  else
#    error "invalid UCL_BYTE_ORDER"
#  endif
    }
#endif

    /* check that unaligned memory access works as expected */
#if defined(UCL_UNALIGNED_OK_2)
    r &= __ucl_assert(sizeof(short) == 2);
    if (r == 1)
    {
        unsigned short b[4];

        for (i = 0; i < 4; i++)
            b[i] = * (const unsigned short *) &u.x[i];

#  if (UCL_BYTE_ORDER == UCL_LITTLE_ENDIAN)
        r &= __ucl_assert(b[0] == 0x0100);
        r &= __ucl_assert(b[1] == 0x0201);
        r &= __ucl_assert(b[2] == 0x0302);
        r &= __ucl_assert(b[3] == 0x0403);
#  elif (UCL_BYTE_ORDER == UCL_BIG_ENDIAN)
        r &= __ucl_assert(b[0] == 0x0001);
        r &= __ucl_assert(b[1] == 0x0102);
        r &= __ucl_assert(b[2] == 0x0203);
        r &= __ucl_assert(b[3] == 0x0304);
#  endif
    }
#endif

#if defined(UCL_UNALIGNED_OK_4)
    r &= __ucl_assert(sizeof(ucl_uint32) == 4);
    if (r == 1)
    {
        ucl_uint32 a[4];

        for (i = 0; i < 4; i++)
            a[i] = * (const ucl_uint32 *) &u.x[i];

#  if (UCL_BYTE_ORDER == UCL_LITTLE_ENDIAN)
        r &= __ucl_assert(a[0] == UCL_UINT32_C(0x03020100));
        r &= __ucl_assert(a[1] == UCL_UINT32_C(0x04030201));
        r &= __ucl_assert(a[2] == UCL_UINT32_C(0x05040302));
        r &= __ucl_assert(a[3] == UCL_UINT32_C(0x06050403));
#  elif (UCL_BYTE_ORDER == UCL_BIG_ENDIAN)
        r &= __ucl_assert(a[0] == UCL_UINT32_C(0x00010203));
        r &= __ucl_assert(a[1] == UCL_UINT32_C(0x01020304));
        r &= __ucl_assert(a[2] == UCL_UINT32_C(0x02030405));
        r &= __ucl_assert(a[3] == UCL_UINT32_C(0x03040506));
#  endif
    }
#endif

#if defined(UCL_ALIGNED_OK_4)
    r &= __ucl_assert(sizeof(ucl_uint32) == 4);
#endif

    /* check the ucl_adler32() function */
    if (r == 1)
    {
        ucl_uint32 adler;
        adler = ucl_adler32(0, NULL, 0);
        adler = ucl_adler32(adler, ucl_copyright(), 186);
        r &= __ucl_assert(adler == UCL_UINT32_C(0x47fb39fc));
    }

    /* check for the gcc schedule-insns optimization bug */
    if (r == 1)
    {
        r &= __ucl_assert(!schedule_insns_bug());
    }

    /* check for the gcc strength-reduce optimization bug */
    if (r == 1)
    {
        static int x[3];
        static unsigned xn = 3;
        register unsigned j;

        for (j = 0; j < xn; j++)
            x[j] = (int)j - 3;
        r &= __ucl_assert(!strength_reduce_bug(x));
    }

    /* now for the low-level pointer checks */
    if (r == 1)
    {
        r &= ptr_check();
    }

    return r == 1 ? UCL_E_OK : UCL_E_ERROR;
}


static ucl_bool schedule_insns_bug(void)
{
#if defined(__UCL_CHECKER)
    /* for some reason checker complains about uninitialized memory access */
    return 0;
#else
    const int clone[] = {1, 2, 0};
    const int *q;
    q = clone;
    return (*q) ? 0 : 1;
#endif
}


static ucl_bool strength_reduce_bug(int *x)
{
    return x[0] != -3 || x[1] != -2 || x[2] != -1;
}


/***********************************************************************
//
************************************************************************/

int __ucl_init_done = 0;

UCL_PUBLIC(int)
__ucl_init2(ucl_uint32 v, int s1, int s2, int s3, int s4, int s5,
                          int s6, int s7, int s8, int s9)
{
    int r;

    __ucl_init_done = 1;

    if (v == 0)
        return UCL_E_ERROR;

    r = (s1 == -1 || s1 == (int) sizeof(short)) &&
        (s2 == -1 || s2 == (int) sizeof(int)) &&
        (s3 == -1 || s3 == (int) sizeof(long)) &&
        (s4 == -1 || s4 == (int) sizeof(ucl_uint32)) &&
        (s5 == -1 || s5 == (int) sizeof(ucl_uint)) &&
        (s6 == -1 || s6 > 0) &&
        (s7 == -1 || s7 == (int) sizeof(char *)) &&
        (s8 == -1 || s8 == (int) sizeof(ucl_voidp)) &&
        (s9 == -1 || s9 == (int) sizeof(ucl_compress_t));
    if (!r)
        return UCL_E_ERROR;

    r = _ucl_config_check();
    if (r != UCL_E_OK)
        return r;

    return r;
}


/*
vi:ts=4:et
*/
