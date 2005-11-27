/* alloc.c -- memory allocation

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

#if defined(HAVE_MALLOC_H)
#  include <malloc.h>
#endif
#if defined(__palmos__)
#  include <System/MemoryMgr.h>
#endif


#undef ucl_alloc_hook
#undef ucl_free_hook
#undef ucl_alloc
#undef ucl_malloc
#undef ucl_free


/***********************************************************************
// implementation
************************************************************************/

UCL_PRIVATE(ucl_voidp)
ucl_alloc_internal(ucl_uint nelems, ucl_uint size)
{
    ucl_voidp p = NULL;
    unsigned long s = (unsigned long) nelems * size;

    if (nelems <= 0 || size <= 0 || s < nelems || s < size)
        return NULL;

#if defined(__palmos__)
    p = (ucl_voidp) MemPtrNew(s);
#elif (UCL_UINT_MAX <= SIZE_T_MAX)
    if (s < SIZE_T_MAX)
        p = (ucl_voidp) malloc((size_t)s);
#elif defined(HAVE_HALLOC) && defined(__DMC__)
    if (size < SIZE_T_MAX)
        p = (ucl_voidp) _halloc(nelems,(size_t)size);
#elif defined(HAVE_HALLOC)
    if (size < SIZE_T_MAX)
        p = (ucl_voidp) halloc(nelems,(size_t)size);
#else
    if (s < SIZE_T_MAX)
        p = (ucl_voidp) malloc((size_t)s);
#endif

    return p;
}


UCL_PRIVATE(void)
ucl_free_internal(ucl_voidp p)
{
    if (!p)
        return;

#if defined(__palmos__)
    MemPtrFree(p);
#elif (UCL_UINT_MAX <= SIZE_T_MAX)
    free(p);
#elif defined(HAVE_HALLOC) && defined(__DMC__)
    _hfree(p);
#elif defined(HAVE_HALLOC)
    hfree(p);
#else
    free(p);
#endif
}


/***********************************************************************
// public interface using the global hooks
************************************************************************/

/* global allocator hooks */
ucl_alloc_hook_t ucl_alloc_hook = ucl_alloc_internal;
ucl_free_hook_t ucl_free_hook = ucl_free_internal;


UCL_PUBLIC(ucl_voidp)
ucl_alloc(ucl_uint nelems, ucl_uint size)
{
    if (!ucl_alloc_hook)
        return NULL;

    return ucl_alloc_hook(nelems,size);
}


UCL_PUBLIC(ucl_voidp)
ucl_malloc(ucl_uint size)
{
    if (!ucl_alloc_hook)
        return NULL;

#if defined(__palmos__)
    return ucl_alloc_hook(size,1);
#elif (UCL_UINT_MAX <= SIZE_T_MAX)
    return ucl_alloc_hook(size,1);
#elif defined(HAVE_HALLOC)
    /* use segment granularity by default */
    if (size + 15 > size)   /* avoid overflow */
        return ucl_alloc_hook((size+15)/16,16);
    return ucl_alloc_hook(size,1);
#else
    return ucl_alloc_hook(size,1);
#endif
}


UCL_PUBLIC(void)
ucl_free(ucl_voidp p)
{
    if (!ucl_free_hook)
        return;

    ucl_free_hook(p);
}


/*
vi:ts=4:et
*/
