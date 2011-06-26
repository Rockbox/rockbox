/**************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2007 Dan Everton
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#ifndef SPEEX_ROCKBOX_H
#define SPEEX_ROCKBOX_H

/* We don't want all this stuff if we're building encoder */
#ifndef ROCKBOX_VOICE_ENCODER

#include "codeclib.h"
#include "debug.h"

#if !defined(ROCKBOX_VOICE_CODEC)

#if defined(DEBUG) || defined(SIMULATOR)
#undef DEBUGF
#define DEBUGF ci->debugf
#endif


#ifdef ROCKBOX_HAS_LOGF
#undef LOGF
#define LOGF ci->logf
#endif

#endif /* ROCKBOX_VOICE_CODEC */

#define OVERRIDE_SPEEX_ALLOC 1
static inline void *speex_alloc (int size)
{
    return codec_calloc(size, 1);
}

#define OVERRIDE_SPEEX_ALLOC_SCRATCH 1
static inline void *speex_alloc_scratch (int size)
{
    return codec_calloc(size,1);
}

#define OVERRIDE_SPEEX_REALLOC 1
static inline void *speex_realloc (void *ptr, int size)
{
    return codec_realloc(ptr, size);
}

#define OVERRIDE_SPEEX_FREE 1
static inline void speex_free (void *ptr)
{
    codec_free(ptr);
}

#define OVERRIDE_SPEEX_FREE_SCRATCH 1
static inline void speex_free_scratch (void *ptr)
{
    codec_free(ptr);
}

#define OVERRIDE_SPEEX_FATAL 1
static inline void _speex_fatal(const char *str, const char *file, int line)
{
    (void)str;
    (void)file;
    (void)line;
    DEBUGF("Fatal error: %s\n", str);
   //exit(1);
}

#define OVERRIDE_SPEEX_WARNING 1
static inline void speex_warning(const char *str)
{
    (void)str;
    DEBUGF("warning: %s\n", str);
}

#define OVERRIDE_SPEEX_WARNING_INT 1
static inline void speex_warning_int(const char *str, int val)
{
    (void)str;
    (void)val;
    DEBUGF("warning: %s %d\n", str, val);
}

#define OVERRIDE_SPEEX_NOTIFY 1
static inline void speex_notify(const char *str)
{
    (void)str;
    DEBUGF("notice: %s\n", str);
}

#define OVERRIDE_SPEEX_PUTC 1
static inline void _speex_putc(int ch, void *file)
{
    (void)ch;
    (void)file;
    //FILE *f = (FILE *)file;
    //printf("%c", ch);
}

#endif /* ROCKBOX_VOICE_ENCODER */

#endif

