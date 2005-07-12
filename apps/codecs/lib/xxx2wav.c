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

/* Various "helper functions" common to all the xxx2wav decoder plugins  */

#if (CONFIG_HWCODEC == MASNONE)
/* software codec platforms, not for simulator */

#include "codecs.h"
#include "xxx2wav.h"

static struct codec_api* local_rb;

int mem_ptr;
int bufsize;
unsigned char* audiobuf;   /* The actual audio buffer from Rockbox */
unsigned char* mallocbuf;  /* 512K from the start of audio buffer */
unsigned char* filebuf;    /* The rest of the audio buffer */

void* codec_malloc(size_t size)
{
    void* x;

    x=&mallocbuf[mem_ptr];
    mem_ptr+=(size+3)&~3; // Keep memory 32-bit aligned (if it was already?)

    return(x);
}

void* codec_calloc(size_t nmemb, size_t size)
{
    void* x;
    x = codec_malloc(nmemb*size);
    local_rb->memset(x,0,nmemb*size);
    return(x);
}

void* codec_alloca(size_t size)
{
    void* x;
    x = codec_malloc(size);
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

void* memmove(const void *s1, const void *s2, size_t n)
{
    char* dest=(char*)s1;
    char* src=(char*)s2;
    size_t i;
    
    for (i=0;i<n;i++) { dest[i]=src[i]; }

    return(dest);
}

void qsort(void *base, size_t nmemb, size_t size,
           int(*compar)(const void *, const void *)) {
    local_rb->qsort(base,nmemb,size,compar);
}


void xxx2wav_set_api(struct codec_api* rb)
{
    local_rb = rb;
}

#endif /* CONFIG_HWCODEC == MASNONE */
