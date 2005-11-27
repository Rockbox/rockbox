/* fake16.h -- fake the strict 16-bit memory model for test purposes

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


/*
 * NOTE:
 *   this file is *only* for testing the strict 16-bit memory model
 *   on a 32-bit machine. Because things like integral promotion,
 *   size_t and ptrdiff_t cannot be faked this is no real substitute
 *   for testing under a real 16-bit system.
 *
 *   See also <ucl/ucl16bit.h>
 *
 *   Usage: #include "src/fake16.h" at the top of <ucl/uclconf.h>
 */


#ifndef __UCLFAKE16BIT_H
#define __UCLFAKE16BIT_H

#ifdef __UCLCONF_H
#  error "include this file before uclconf.h"
#endif

#include <limits.h>

#if (USHRT_MAX == 0xffff)

#ifdef __cplusplus
extern "C" {
#endif

#define __UCL16BIT_H        /* do not use <ucl/ucl16bit.h> */

#define __UCL_STRICT_16BIT
#define __UCL_FAKE_STRICT_16BIT

#define UCL_99_UNSUPPORTED
#define UCL_999_UNSUPPORTED

typedef unsigned short      ucl_uint;
typedef short               ucl_int;
#define UCL_UINT_MAX        USHRT_MAX
#define UCL_INT_MAX         SHRT_MAX

#if 1
#define __UCL_NO_UNALIGNED
#define __UCL_NO_ALIGNED
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

#endif /* already included */

