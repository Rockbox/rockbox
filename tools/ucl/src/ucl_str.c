/* ucl_str.c -- string functions for the the UCL library

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

#undef ucl_memcmp
#undef ucl_memcpy
#undef ucl_memmove
#undef ucl_memset


/***********************************************************************
// slow but portable <string.h> stuff, only used in assertions
************************************************************************/

UCL_PUBLIC(int)
ucl_memcmp(const ucl_voidp s1, const ucl_voidp s2, ucl_uint len)
{
#if (UCL_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMCMP)
    return memcmp(s1,s2,len);
#else
    const ucl_byte *p1 = (const ucl_byte *) s1;
    const ucl_byte *p2 = (const ucl_byte *) s2;
    int d;

    if (len > 0) do
    {
        d = *p1 - *p2;
        if (d != 0)
            return d;
        p1++;
        p2++;
    }
    while (--len > 0);
    return 0;
#endif
}


UCL_PUBLIC(ucl_voidp)
ucl_memcpy(ucl_voidp dest, const ucl_voidp src, ucl_uint len)
{
#if (UCL_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMCPY)
    return memcpy(dest,src,len);
#else
    ucl_byte *p1 = (ucl_byte *) dest;
    const ucl_byte *p2 = (const ucl_byte *) src;

    if (len <= 0 || p1 == p2)
        return dest;
    do
        *p1++ = *p2++;
    while (--len > 0);
    return dest;
#endif
}


UCL_PUBLIC(ucl_voidp)
ucl_memmove(ucl_voidp dest, const ucl_voidp src, ucl_uint len)
{
#if (UCL_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMMOVE)
    return memmove(dest,src,len);
#else
    ucl_byte *p1 = (ucl_byte *) dest;
    const ucl_byte *p2 = (const ucl_byte *) src;

    if (len <= 0 || p1 == p2)
        return dest;

    if (p1 < p2)
    {
        do
            *p1++ = *p2++;
        while (--len > 0);
    }
    else
    {
        p1 += len;
        p2 += len;
        do
            *--p1 = *--p2;
        while (--len > 0);
    }
    return dest;
#endif
}


UCL_PUBLIC(ucl_voidp)
ucl_memset(ucl_voidp s, int c, ucl_uint len)
{
#if (UCL_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMSET)
    return memset(s,c,len);
#else
    ucl_byte *p = (ucl_byte *) s;

    if (len > 0) do
        *p++ = UCL_BYTE(c);
    while (--len > 0);
    return s;
#endif
}


/*
vi:ts=4:et
*/
