/* portab_a.h -- advanced portability layer

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


/*************************************************************************
// use the ACC library for the hard work
**************************************************************************/

#if defined(UCL_HAVE_CONFIG_H)
#  define ACC_CONFIG_NO_HEADER 1
#endif
#include "acc/acc.h"

#if (ACC_CC_MSC && (_MSC_VER >= 1000 && _MSC_VER < 1200))
   /* avoid `-W4' warnings in system header files */
#  pragma warning(disable: 4201 4214 4514)
#endif
#if (ACC_CC_MSC && (_MSC_VER >= 1300))
   /* avoid `-Wall' warnings in system header files */
#  pragma warning(disable: 4163 4255 4820)
   /* avoid warnings about inlining */
#  pragma warning(disable: 4710 4711)
#endif
#if (ACC_CC_MSC && (_MSC_VER >= 1400))
   /* avoid warnings when using "deprecated" POSIX functions */
#  pragma warning(disable: 4996)
#endif

#include "acc/acc_incd.h"
#include "acc/acc_ince.h"
#if defined(__UCL_MMODEL_HUGE) || defined(WANT_UCL_UCLOCK) || defined(WANT_UCL_WILDARGV)
#  include "acc/acc_inci.h"
#  include "acc/acc_lib.h"
#endif

#if defined(WANT_UCL_MALLOC)
#  if defined(__UCL_MMODEL_HUGE)
#    include "acc/acclib/halloc.ch"
#  else
#    define acc_halloc(a)           (malloc(a))
#    define acc_hfree(a)            (free(a))
#  endif
#endif
#if defined(WANT_UCL_FREAD)
#  if defined(__UCL_MMODEL_HUGE)
#    include "acc/acclib/hfread.ch"
#  else
#    define acc_hfread(f,b,s)       (fread(b,1,s,f))
#    define acc_hfwrite(f,b,s)      (fwrite(b,1,s,f))
#  endif
#endif
#if defined(WANT_UCL_UCLOCK)
#  include "acc/acclib/uclock.ch"
#endif
#if defined(WANT_UCL_WILDARGV)
#  include "acc/acclib/wildargv.ch"
#endif
#if (__ACCLIB_REQUIRE_HMEMCPY_CH) && !defined(__ACCLIB_HMEMCPY_CH_INCLUDED)
#  include "acc/acclib/hmemcpy.ch"
#endif


/*************************************************************************
// misc
**************************************************************************/

/* turn on assertions */
#undef NDEBUG
#include <assert.h>

/* just in case */
#undef xmalloc
#undef xfree
#undef xread
#undef xwrite
#undef xputc
#undef xgetc
#undef xread32
#undef xwrite32


/*************************************************************************
// finally pull into the UCL namespace
**************************************************************************/

#undef ucl_malloc
#undef ucl_free
#undef ucl_fread
#undef ucl_fwrite
#if defined(WANT_UCL_MALLOC)
#  define ucl_malloc(a)         acc_halloc(a)
#  define ucl_free(a)           acc_hfree(a)
#endif
#if defined(WANT_UCL_FREAD)
#  define ucl_fread(f,b,s)      acc_hfread(f,b,s)
#  define ucl_fwrite(f,b,s)     acc_hfwrite(f,b,s)
#endif
#if defined(WANT_UCL_UCLOCK)
#  define ucl_uclock_handle_t   acc_uclock_handle_t
#  define ucl_uclock_t          acc_uclock_t
#  define ucl_uclock_open(a)    acc_uclock_open(a)
#  define ucl_uclock_close(a)   acc_uclock_close(a)
#  define ucl_uclock_read(a,b)  acc_uclock_read(a,b)
#  define ucl_uclock_get_elapsed(a,b,c) acc_uclock_get_elapsed(a,b,c)
#endif
#if defined(WANT_UCL_WILDARGV)
#  define ucl_wildargv(a,b)     acc_wildargv(a,b)
#endif


/*
vi:ts=4:et
*/

