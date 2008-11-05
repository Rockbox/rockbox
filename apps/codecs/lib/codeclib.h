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

#include "config.h"
#include "codecs.h"
#include "system.h"
#include <sys/types.h>

extern struct codec_api *ci;
extern long mem_ptr;
extern long bufsize;
extern unsigned char* mp3buf;     // The actual MP3 buffer from Rockbox
extern unsigned char* mallocbuf;  // 512K from the start of MP3 buffer
extern unsigned char* filebuf;    // The rest of the MP3 buffer

/* Standard library functions that are used by the codecs follow here */

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

#define abs(x) ((x)>0?(x):-(x))
#define labs(x) abs(x)

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

/* Various codec helper functions */

int codec_init(void);
void codec_set_replaygain(struct mp3entry* id3);

#ifdef RB_PROFILE
void __cyg_profile_func_enter(void *this_fn, void *call_site)
    NO_PROF_ATTR ICODE_ATTR;
void __cyg_profile_func_exit(void *this_fn, void *call_site)
    NO_PROF_ATTR ICODE_ATTR;
#endif
