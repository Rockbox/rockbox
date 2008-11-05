/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2002    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: #ifdef jail to whip a few platforms into the UNIX ideal.

 ********************************************************************/
#include "config-tremor.h"

#ifndef _OS_TYPES_H
#define _OS_TYPES_H

#include <stdlib.h>
#include <codecs.h>

#ifdef _LOW_ACCURACY_
#  define X(n) (((((n)>>22)+1)>>1) - ((((n)>>22)+1)>>9))
#  define LOOKUP_T const unsigned char  
#else
#  define X(n) (n)
#  define LOOKUP_T const ogg_int32_t
#endif

/* make it easy on the folks that want to compile the libs with a
   different malloc than stdlib */

#define _ogg_malloc  ogg_malloc
#define _ogg_calloc  ogg_calloc
#define _ogg_realloc ogg_realloc
#define _ogg_free(x) do { } while(0)

void ogg_malloc_init(void);
void *ogg_malloc(size_t size);
void *ogg_tmpmalloc(size_t size);
void *ogg_calloc(size_t nmemb, size_t size);
void *ogg_tmpcalloc(size_t nmemb, size_t size);
void *ogg_realloc(void *ptr, size_t size);
long ogg_tmpmalloc_pos(void);
void ogg_tmpmalloc_free(long pos);

   typedef short ogg_int16_t;
   typedef int ogg_int32_t;
   typedef unsigned int ogg_uint32_t;
   typedef long long ogg_int64_t;

#endif  /* _OS_TYPES_H */
