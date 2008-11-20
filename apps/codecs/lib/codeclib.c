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
#include "dsp.h"
#include "codeclib.h"
#include "metadata.h"

size_t mem_ptr;
size_t bufsize;
unsigned char* mp3buf;     // The actual MP3 buffer from Rockbox
unsigned char* mallocbuf;  // 512K from the start of MP3 buffer
unsigned char* filebuf;    // The rest of the MP3 buffer

int codec_init(void)
{
    mem_ptr = 0;
    mallocbuf = (unsigned char *)ci->codec_get_buffer((size_t *)&bufsize);
  
    return 0;
}

void codec_set_replaygain(struct mp3entry* id3)
{
    ci->configure(DSP_SET_TRACK_GAIN, id3->track_gain);
    ci->configure(DSP_SET_ALBUM_GAIN, id3->album_gain);
    ci->configure(DSP_SET_TRACK_PEAK, id3->track_peak);
    ci->configure(DSP_SET_ALBUM_PEAK, id3->album_peak);
}

/* Various "helper functions" common to all the xxx2wav decoder plugins  */


void* codec_malloc(size_t size)
{
    void* x;

    if (mem_ptr + (long)size > bufsize)
        return NULL;
    
    x=&mallocbuf[mem_ptr];
    mem_ptr+=(size+3)&~3; /* Keep memory 32-bit aligned */

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

#ifdef RB_PROFILE
void __cyg_profile_func_enter(void *this_fn, void *call_site) {
#ifdef CPU_COLDFIRE
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
