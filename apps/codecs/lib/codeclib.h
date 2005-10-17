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

#include "config.h"
#include "codecs.h"
#include "system.h"
#include <sys/types.h>

#define MALLOC_BUFSIZE (512*1024)

extern int mem_ptr;
extern int bufsize;
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
void *memmove(const void *s1, const void *s2, size_t n);

size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);
int strcmp(const char *, const char *);
int strcasecmp(const char *, const char *);

void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *));

#define abs(x) ((x)>0?(x):-(x))
#define labs(x) abs(x)

/* Various codec helper functions */

int codec_init(struct codec_api* rb);
void codec_set_replaygain(struct mp3entry* id3);

