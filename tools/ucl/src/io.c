/* io.c -- io functions

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

#if !defined(NO_STDIO_H)

#include <stdio.h>
#include <ucl/uclutil.h>

#undef ucl_fread
#undef ucl_fwrite


/***********************************************************************
//
************************************************************************/

UCL_PUBLIC(ucl_uint)
ucl_fread(FILE *f, ucl_voidp s, ucl_uint len)
{
#if 1 && (UCL_UINT_MAX <= SIZE_T_MAX)
    return fread(s,1,len,f);
#else
    ucl_byte *p = (ucl_byte *) s;
    ucl_uint l = 0;
    size_t k;
    unsigned char *b;
    unsigned char buf[512];

    while (l < len)
    {
        k = len - l > sizeof(buf) ? sizeof(buf) : (size_t) (len - l);
        k = fread(buf,1,k,f);
        if (k <= 0)
            break;
        l += k;
        b = buf; do *p++ = *b++; while (--k > 0);
    }
    return l;
#endif
}


/***********************************************************************
//
************************************************************************/

UCL_PUBLIC(ucl_uint)
ucl_fwrite(FILE *f, const ucl_voidp s, ucl_uint len)
{
#if 1 && (UCL_UINT_MAX <= SIZE_T_MAX)
    return fwrite(s,1,len,f);
#else
    const ucl_byte *p = (const ucl_byte *) s;
    ucl_uint l = 0;
    size_t k, n;
    unsigned char *b;
    unsigned char buf[512];

    while (l < len)
    {
        k = len - l > sizeof(buf) ? sizeof(buf) : (size_t) (len - l);
        b = buf; n = k; do *b++ = *p++; while (--n > 0);
        k = fwrite(buf,1,k,f);
        if (k <= 0)
            break;
        l += k;
    }
    return l;
#endif
}


#endif /* !defined(NO_STDIO_H) */


/*
vi:ts=4:et
*/
