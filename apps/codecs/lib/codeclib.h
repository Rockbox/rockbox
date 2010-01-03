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

#ifndef __CODECLIB_H__
#define __CODECLIB_H__

#include "config.h"
#include "codecs.h"
#include <sys/types.h>

extern struct codec_api *ci;
extern size_t mem_ptr;
extern size_t bufsize;
extern unsigned char* mp3buf;     /* The actual MP3 buffer from Rockbox                 */
extern unsigned char* mallocbuf;  /* The free space after the codec in the codec buffer */
extern unsigned char* filebuf;    /* The rest of the MP3 buffer                         */

/* Standard library functions that are used by the codecs follow here */

/* Get these functions 'out of the way' of the standard functions. Not doing
 * so confuses the cygwin linker, and maybe others. These functions need to
 * be implemented elsewhere */
#define malloc(x) codec_malloc(x)
#define calloc(x,y) codec_calloc(x,y)
#define realloc(x,y) codec_realloc(x,y)
#define free(x) codec_free(x)
#define alloca(x) __builtin_alloca(x)

void* codec_malloc(size_t size);
void* codec_calloc(size_t nmemb, size_t size);
void* codec_realloc(void* ptr, size_t size);
void codec_free(void* ptr);

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memmove(void *s1, const void *s2, size_t n);

size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);
int strcmp(const char *, const char *);

void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *));

/*MDCT library functions*/

extern void mdct_backward(int n, int32_t *in, int32_t *out);

#if defined(CPU_ARM) && (ARM_ARCH == 4)
/* optimised unsigned integer division for ARMv4, in IRAM */
unsigned udiv32_arm(unsigned a, unsigned b);
#define UDIV32(a, b) udiv32_arm(a, b)
#else
/* default */
#define UDIV32(a, b) (a / b)
#endif

#if !defined(CPU_ARM) || ARM_ARCH < 5
/* From libavutil/common.h */
extern const uint8_t bs_log2_tab[256] ICONST_ATTR;
extern const uint8_t bs_clz_tab[256] ICONST_ATTR;
#endif

#define BS_LOG2  0 /* default personality, equivalent floor(log2(x)) */
#define BS_CLZ   1 /* alternate personality, Count Leading Zeros */
#define BS_SHORT 2 /* input guaranteed not to exceed 16 bits */
#define BS_0_0   4 /* guarantee mapping of 0 input to 0 output */

/* Generic bit-scanning function, used to wrap platform CLZ instruction or
   scan-and-lookup code, and to provide control over output for 0 inputs. */
static inline unsigned int bs_generic(unsigned int v, int mode)
{
#if defined(CPU_ARM) && ARM_ARCH >= 5
    unsigned int r = __builtin_clz(v);
    if (mode & BS_CLZ)
    {
        if (mode & BS_0_0)
            r &= 31;
    } else {
        r = 31 - r;
    /* If mode is constant, this is a single conditional instruction */
        if (mode & BS_0_0 && (signed)r < 0) 
            r += 1;
    }
#else
    const uint8_t *bs_tab;
    unsigned int r;
    unsigned int n = v;
    int inc;
    /* Set up table, increment, and initial result value based on
       personality. */
    if (mode & BS_CLZ)
    {
        bs_tab = bs_clz_tab;
        r = 24;
        inc = -16;
    } else {
        bs_tab = bs_log2_tab;
        r = 0;
        inc = 16;
    }
    if (!(mode & BS_SHORT) && n >= 0x10000) {
        n >>= 16;
        r += inc;
    }
    if (n > 0xff) {
        n >>= 8;
        r += inc / 2;
    }
#ifdef CPU_COLDFIRE
    /* The high 24 bits of n are guaranteed empty after the above, so a
       superfluous ext.b instruction can be saved by loading the LUT value over
       n with asm */
    asm volatile (
        "move.b (%1,%0.l),%0"
        : "+d" (n)
        : "a" (bs_tab)
    );
#else
    n = bs_tab[n];
#endif
    r += n;
    if (mode & BS_CLZ && mode & BS_0_0 && v == 0)
        r = 0;
#endif
    return r;
}

/* TODO figure out if we really need to care about calculating
   av_log2(0) */
#define av_log2(v) bs_generic(v, BS_0_0)

/* Various codec helper functions */

int codec_init(void);
void codec_set_replaygain(struct mp3entry* id3);

#ifdef RB_PROFILE
void __cyg_profile_func_enter(void *this_fn, void *call_site)
    NO_PROF_ATTR ICODE_ATTR;
void __cyg_profile_func_exit(void *this_fn, void *call_site)
    NO_PROF_ATTR ICODE_ATTR;
#endif

#endif /* __CODECLIB_H__ */
