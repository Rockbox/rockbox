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

#include "../codec.h"
#include "../lib/codeclib.h"

extern struct codec_api* ci;

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

void *speex_alloc (int size)
{
    return codec_calloc(size, 1);
}

void *speex_alloc_scratch (int size)
{
    return codec_calloc(size,1);
}

void *speex_realloc (void *ptr, int size)
{
    return codec_realloc(ptr, size);
}

void speex_free (void *ptr)
{
    codec_free(ptr);
}

void speex_free_scratch (void *ptr)
{
    codec_free(ptr);
}

void *speex_move (void *dest, void *src, int n)
{
   return memmove(dest,src,n);
}

void speex_error(const char *str)
{
    DEBUGF("Fatal error: %s\n", str);
   //exit(1);
}

void speex_warning(const char *str)
{
    DEBUGF("warning: %s\n", str);
}

void speex_warning_int(const char *str, int val)
{
    DEBUGF("warning: %s %d\n", str, val);
}

void _speex_putc(int ch, void *file)
{
    //FILE *f = (FILE *)file;
    //printf("%c", ch);
}

float floor(float x) {
    return ((float)(((int)x)));
} 

//Placeholders (not fixed point, only used when encoding):
float pow(float a, float b) {
	return 0;
}

float log(float l) {
	return 0;
}

float fabs(float a) {
	return 0;
}

float sin(float a) {
	return 0;
}

float cos(float a) {
	return 0;
}

float sqrt(float a) {
	return 0;
}

float exp(float a) {
	return 0;
}
