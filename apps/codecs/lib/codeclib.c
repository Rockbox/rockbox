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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* "helper functions" common to all codecs  */

#include "codecs.h"
#include "dsp.h"
#include "codeclib.h"
#include "id3.h"

struct codec_api *local_rb;

long mem_ptr;
long bufsize;
unsigned char* mp3buf;     // The actual MP3 buffer from Rockbox
unsigned char* mallocbuf;  // 512K from the start of MP3 buffer
unsigned char* filebuf;    // The rest of the MP3 buffer

int codec_init(struct codec_api* rb)
{
    local_rb = rb;
    mem_ptr = 0;
    mallocbuf = (unsigned char *)rb->get_codec_memory((size_t *)&bufsize);
  
    return 0;
}

void codec_set_replaygain(struct mp3entry* id3)
{
    local_rb->configure(DSP_SET_TRACK_GAIN, (long *) id3->track_gain);
    local_rb->configure(DSP_SET_ALBUM_GAIN, (long *) id3->album_gain);
    local_rb->configure(DSP_SET_TRACK_PEAK, (long *) id3->track_peak);
    local_rb->configure(DSP_SET_ALBUM_PEAK, (long *) id3->album_peak);
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
    local_rb->memset(x,0,nmemb*size);
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
    return(local_rb->strlen(s));
}

char *strcpy(char *dest, const char *src)
{
    return(local_rb->strcpy(dest,src));
}

char *strcat(char *dest, const char *src)
{
    return(local_rb->strcat(dest,src));
}

int strcmp(const char *s1, const char *s2)
{
    return(local_rb->strcmp(s1,s2));
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    return(local_rb->strncasecmp(s1,s2,n));
}

void *memcpy(void *dest, const void *src, size_t n)
{
    return(local_rb->memcpy(dest,src,n));
}

void *memset(void *s, int c, size_t n)
{
    return(local_rb->memset(s,c,n));
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    return(local_rb->memcmp(s1,s2,n));
}

void* memchr(const void *s, int c, size_t n)
{
    return(local_rb->memchr(s,c,n));
}

void *memmove(void *dest, const void *src, size_t n)
{
    return(local_rb->memmove(dest,src,n));
}

void qsort(void *base, size_t nmemb, size_t size,
           int(*compar)(const void *, const void *))
{
    local_rb->qsort(base,nmemb,size,compar);
}

#ifdef RB_PROFILE
void __cyg_profile_func_enter(void *this_fn, void *call_site) {
#ifdef CPU_COLDFIRE
    (void)call_site;
    local_rb->profile_func_enter(this_fn, __builtin_return_address(1));
#else
    local_rb->profile_func_enter(this_fn, call_site);
#endif
}

void __cyg_profile_func_exit(void *this_fn, void *call_site) {
    local_rb->profile_func_exit(this_fn,call_site);
}
#endif
