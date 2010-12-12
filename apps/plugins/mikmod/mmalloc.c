/*	MikMod sound library
	(c) 1998, 1999 Miodrag Vallat and others - see file AUTHORS for
	complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.
 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.
 
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id: mmalloc.c,v 1.3 2007/12/03 20:42:58 denis111 Exp $

  Dynamic memory routines

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

#define ALIGN_STRIDE 16

static void * align_pointer(char *ptr, size_t stride)
{
	char *pptr = ptr + sizeof(void*);
	char *fptr;
	size_t err = ((size_t)pptr)&(stride-1);
	if (err)
		fptr = pptr + (stride - err);
	else
		fptr = pptr;
	*(size_t*)(fptr - sizeof(void*)) = (size_t)ptr;
	return fptr;
}

static void *get_pointer(void *data)
{
	unsigned char *_pptr = (unsigned char*)data - sizeof(void*);
	size_t _ptr = *(size_t*)_pptr;
	return (void*)_ptr;
}


void* MikMod_realloc(void *data, size_t size)
{
	return realloc(data, size);
	
#if 0
	if (data)
	{
#if defined __MACH__
		void *d = realloc(data, size);
		if (d)
		{
			return d;
		}
		return 0;
#elif (defined _WIN32 || defined _WIN64) && !defined(_WIN32_WCE)
		return _aligned_realloc(data, size, ALIGN_STRIDE);
#else
		unsigned char *newPtr = (unsigned char *)realloc(get_pointer(data), size + ALIGN_STRIDE + sizeof(void*));
		return align_pointer(newPtr, ALIGN_STRIDE);
#endif
	}
	return MikMod_malloc(size);
#endif
}


/* Same as malloc, but sets error variable _mm_error when fails. Returns a 16-byte aligned pointer */
void* MikMod_malloc(size_t size)
{
	void *d;
	if(!(d=calloc(1,size))) {
		_mm_errno = MMERR_OUT_OF_MEMORY;
		if(_mm_errorhandler) _mm_errorhandler();
	}
	return d;
	
#if 0
#if defined __MACH__
	void *d = calloc(1, size);
	if (d)
	{
		return d;
	}
	return 0;
#elif (defined _WIN32 || defined _WIN64) && !defined(_WIN32_WCE)
	void * d = _aligned_malloc(size, ALIGN_STRIDE);
	if (d)
	{
		ZeroMemory(d, size);
		return d;
	}
	return 0;
#else
	void *d = calloc(1, size + ALIGN_STRIDE + sizeof(void*));

	if(!d) {
		_mm_errno = MMERR_OUT_OF_MEMORY;
		if(_mm_errorhandler) _mm_errorhandler();
	}
	return align_pointer(d, ALIGN_STRIDE);
#endif
#endif
}

/* Same as calloc, but sets error variable _mm_error when fails */
void* MikMod_calloc(size_t nitems,size_t size)
{
	void *d;
   
	if(!(d=calloc(nitems,size))) {
		_mm_errno = MMERR_OUT_OF_MEMORY;
		if(_mm_errorhandler) _mm_errorhandler();
	}
	return d;
	
#if 0
#if defined __MACH__
	void *d = calloc(nitems, size);
	if (d)
	{
		return d;
	}
	return 0;
#elif (defined _WIN32 || defined _WIN64) && !defined(_WIN32_WCE)
	void * d = _aligned_malloc(size * nitems, ALIGN_STRIDE);
	if (d)
	{
		ZeroMemory(d, size * nitems);
		return d;
	}
	return 0;
#else
	void *d = calloc(nitems, size + ALIGN_STRIDE + sizeof(void*));
   
	if(!d) {
		_mm_errno = MMERR_OUT_OF_MEMORY;
		if(_mm_errorhandler) _mm_errorhandler();
	}
	return align_pointer(d, ALIGN_STRIDE);
#endif
#endif
}

void MikMod_free(void *data)
{
	free(data);
	
#if 0
	if (data)
	{
#if defined __MACH__
		free(data);
#elif (defined _WIN32 || defined _WIN64) && !defined(_WIN32_WCE)
		_aligned_free(data);
#else
		free(get_pointer(data));
#endif
	}
#endif
}

/* ex:set ts=4: */
