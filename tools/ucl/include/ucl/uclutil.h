/* uclutil.h -- utilities for the UCL real-time data compression library

   This file is part of the UCL data compression library.

   Copyright (C) 2002 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2001 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2000 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1999 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1998 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1997 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996 Markus Franz Xaver Johannes Oberhumer
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


#ifndef __UCLUTIL_H
#define __UCLUTIL_H

#ifndef __UCLCONF_H
#include <ucl/uclconf.h>
#endif

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
//
************************************************************************/

UCL_EXTERN(ucl_uint)
ucl_fread(FILE *f, ucl_voidp buf, ucl_uint size);
UCL_EXTERN(ucl_uint)
ucl_fwrite(FILE *f, const ucl_voidp buf, ucl_uint size);


#if (UCL_UINT_MAX <= UINT_MAX)
   /* avoid problems with Win32 DLLs */
#  define ucl_fread(f,b,s)      (fread(b,1,s,f))
#  define ucl_fwrite(f,b,s)     (fwrite(b,1,s,f))
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */

