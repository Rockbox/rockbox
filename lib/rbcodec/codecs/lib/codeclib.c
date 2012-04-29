/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* "helper functions" common to all codecs  */

#include <string.h>
#include "codecs.h"
#include "codeclib.h"
#include "metadata.h"
#include "dsp_proc_entry.h"

/* The following variables are used by codec_malloc() to make use of free RAM
 * within the statically allocated codec buffer. */
static size_t mem_ptr = 0;
static size_t bufsize = 0;
static unsigned char* mallocbuf = NULL;

int codec_init(void)
{
    /* codec_get_buffer() aligns the resulting point to CACHEALIGN_SIZE. */
    mem_ptr = 0;
    mallocbuf = (unsigned char *)ci->codec_get_buffer((size_t *)&bufsize);
  
    return 0;
}

void codec_set_replaygain(const struct mp3entry *id3)
{
    struct dsp_replay_gains gains =
    {
        .track_gain = id3->track_gain,
        .album_gain = id3->album_gain,
        .track_peak = id3->track_peak,
        .album_peak = id3->album_peak,
    };

    ci->configure(REPLAYGAIN_SET_GAINS, (intptr_t)&gains);
}

/* Various "helper functions" common to all the xxx2wav decoder plugins  */


void* codec_malloc(size_t size)
{
    void* x;

    if (mem_ptr + (long)size > bufsize)
        return NULL;
    
    x=&mallocbuf[mem_ptr];
    
    /* Keep memory aligned to CACHEALIGN_SIZE. */
    mem_ptr += (size + (CACHEALIGN_SIZE-1)) & ~(CACHEALIGN_SIZE-1);

    return(x);
}

void* codec_calloc(size_t nmemb, size_t size)
{
    void* x;
    x = codec_malloc(nmemb*size);
    if (x == NULL)
        return NULL;
    ci->memset(x,0,nmemb*size);
    return(x);
}

void codec_free(void* ptr) {
    (void)ptr;
}

void* codec_realloc(void* ptr, size_t size)
{
    void* x;
    (void)ptr;
    x = codec_malloc(size);
    return(x);
}

size_t strlen(const char *s)
{
    return(ci->strlen(s));
}

char *strcpy(char *dest, const char *src)
{
    return(ci->strcpy(dest,src));
}

char *strcat(char *dest, const char *src)
{
    return(ci->strcat(dest,src));
}

int strcmp(const char *s1, const char *s2)
{
    return(ci->strcmp(s1,s2));
}

void *memcpy(void *dest, const void *src, size_t n)
{
    return(ci->memcpy(dest,src,n));
}

void *memset(void *s, int c, size_t n)
{
    return(ci->memset(s,c,n));
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    return(ci->memcmp(s1,s2,n));
}

void* memchr(const void *s, int c, size_t n)
{
    return(ci->memchr(s,c,n));
}

void *memmove(void *dest, const void *src, size_t n)
{
    return(ci->memmove(dest,src,n));
}

void qsort(void *base, size_t nmemb, size_t size,
           int(*compar)(const void *, const void *))
{
    ci->qsort(base,nmemb,size,compar);
}

/* From ffmpeg - libavutil/common.h */
const uint8_t bs_log2_tab[256] ICONST_ATTR = {
    0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

const uint8_t bs_clz_tab[256] ICONST_ATTR = {
    8,7,6,6,5,5,5,5,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

#ifdef RB_PROFILE
void __cyg_profile_func_enter(void *this_fn, void *call_site) {
/* This workaround is required for coldfire gcc 3.4 but is broken for 4.4
   and 4.5, but for those the other way works. */
#if defined(CPU_COLDFIRE) && defined(__GNUC__) && __GNUC__ < 4
    (void)call_site;
    ci->profile_func_enter(this_fn, __builtin_return_address(1));
#else
    ci->profile_func_enter(this_fn, call_site);
#endif
}

void __cyg_profile_func_exit(void *this_fn, void *call_site) {
    ci->profile_func_exit(this_fn,call_site);
}
#endif
