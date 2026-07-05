/* portab.h -- portability layer

   This file is part of the UCL data compression library.

   Copyright (C) 1996-2004 Markus Franz Xaver Johannes Oberhumer
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


#include <ucl/uclconf.h>

#if 1

#include "portab_a.h"

#else

/* INFO:
 *   The "portab_a.h" version above uses the ACC library to add
 *   support for ancient systems (like 16-bit DOS) and to provide
 *   some gimmicks like win32 high-resolution timers.
 *   Still, on any halfway modern machine you can also use the
 *   following pure ANSI-C code instead.
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#undef NDEBUG
#include <assert.h>

#define __acc_cdecl_main
#define ACC_UNUSED(var)         ((void) &var)

#if defined(WANT_UCL_MALLOC)
#  define ucl_malloc(a)         (malloc(a))
#  define ucl_free(a)           (free(a))
#endif
#if defined(WANT_UCL_FREAD)
#  define ucl_fread(f,b,s)      (fread(b,1,s,f))
#  define ucl_fwrite(f,b,s)     (fwrite(b,1,s,f))
#endif
#if defined(WANT_UCL_UCLOCK)
#  define ucl_uclock_handle_t   int
#  define ucl_uclock_t          double
#  define ucl_uclock_open(a)    ((void)(a))
#  define ucl_uclock_close(a)   ((void)(a))
#  define ucl_uclock_read(a,b)  *(b) = (clock() / (double)(CLOCKS_PER_SEC))
#  define ucl_uclock_get_elapsed(a,b,c) (*(c) - *(b))
#endif
#if defined(WANT_UCL_WILDARGV)
#  define ucl_wildargv(a,b)     ((void)0)
#endif

#endif


/*
vi:ts=4:et
*/

