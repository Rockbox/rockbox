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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#ifndef SPEEX_ROCKBOX_H
#define SPEEX_ROCKBOX_H

#include "../codec.h"
#include "../lib/codeclib.h"

#if defined(DEBUG) || defined(SIMULATOR)
#undef DEBUGF
#define DEBUGF ci->debugf
#else
#define DEBUGF(...)
#endif

#ifdef ROCKBOX_HAS_LOGF
#undef LOGF
#define LOGF ci->logf
#else
#define LOGF(...)
#endif

extern struct codec_api* ci;

static inline void *speex_alloc (int size)
{
    return codec_calloc(size, 1);
}

static inline void *speex_alloc_scratch (int size)
{
    return codec_calloc(size,1);
}

static inline void *speex_realloc (void *ptr, int size)
{
    return codec_realloc(ptr, size);
}

static inline void speex_free (void *ptr)
{
    codec_free(ptr);
}

static inline void speex_free_scratch (void *ptr)
{
    codec_free(ptr);
}

static inline void *speex_move (void *dest, void *src, int n)
{
   return memmove(dest,src,n);
}

static inline void _speex_fatal(const char *str, const char *file, int line)
{
    DEBUGF("Fatal error: %s\n", str);
   //exit(1);
}

static inline void speex_warning(const char *str)
{
    DEBUGF("warning: %s\n", str);
}

static inline void speex_warning_int(const char *str, int val)
{
    DEBUGF("warning: %s %d\n", str, val);
}

static inline void speex_notify(const char *str)
{
    DEBUGF("notice: %s\n", str);
}

static inline void _speex_putc(int ch, void *file)
{
    //FILE *f = (FILE *)file;
    //printf("%c", ch);
}

#endif

