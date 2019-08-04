/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifndef _SDL_config_rockbox_h
#define _SDL_config_rockbox_h

#include "SDL_platform.h"

/* Rockbox SDL config header */

#include "plugin.h"
#include "lib/pluginlib_exit.h"

#ifdef _WIN32
#define __int64 long long
#endif

#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <tlsf.h>

//#define COMBINED_SDL

/* "recommended" sample rate for Rockbox. Games should use this by
 * default unless necessary to do otherwise. */
#ifdef SIMULATOR
#define RB_SAMPR SAMPR_44
#else
#define RB_SAMPR HW_SAMPR_MIN_GE_22 /* Min HW rate at least 22KHz */
#endif

/* Enable the stub cdrom driver (src/cdrom/dummy/\*.c) */
#define SDL_CDROM_DISABLED      1

/* Enable the stub joystick driver (src/joystick/dummy/\*.c) */
#define SDL_JOYSTICK_DISABLED   1

/* Enable the stub shared object loader (src/loadso/dummy/\*.c) */
#define SDL_LOADSO_DISABLED     1

/* woot */
#define SDL_AUDIO_DRIVER_ROCKBOX 1
#define SDL_THREAD_ROCKBOX       1
#undef  SDL_THREAD_PTHREAD
#define SDL_TIMER_ROCKBOX        1
#define SDL_VIDEO_DRIVER_ROCKBOX 1

#ifndef ROCKBOX_BIG_ENDIAN
#define SDL_BYTEORDER SDL_LIL_ENDIAN
#else
#define SDL_BYTEORDER SDL_BIG_ENDIAN
#endif

#define SDL_HAS_64BIT_TYPE 1
#define LACKS_UNISTD_H    1
#define LACKS_STDLIB_H    1
#define LACKS_MMAN_H      1
#define LACKS_SYS_PARAM_H 1
#define LACKS_SYS_MMAN_H  1

#define HAVE_STDIO_H      1
#define HAVE_MALLOC       1
#define HAVE_FREE         1
#define HAVE_REALLOC      1
#define HAVE_QSORT        1
#define HAVE_STRLEN       1
#define HAVE_STRLCPY      1
#define HAVE_STRCMP       1
#define HAVE_STRNCMP      1

#undef strdup

/* clock() wraps current_tick */
#ifdef CLOCKS_PER_SEC
#undef CLOCKS_PER_SEC
#endif
#define CLOCKS_PER_SEC HZ

/*
  copied from firmware/assert.h
*/

#undef assert

#ifdef NDEBUG           /* required by ANSI standard */
#define assert(p)       ((void)0)
#else

#define assert(e)       ((e) ? (void)0 : fatal("assertion failed %s:%d", __FILE__, __LINE__))

#endif /* NDEBUG */

#define SDL_calloc calloc
#define atan atan_wrapper
#define atan2 atan2_wrapper
#define atexit rb_atexit
#define atof atof_wrapper
#define atoi rb->atoi
#define atol atoi
#define calloc tlsf_calloc
#define ceil ceil_wrapper
#define clock() (*rb->current_tick)
#define closedir rb->closedir
#define cos cos_wrapper
#define exit rb_exit
#define exp(x) pow(2.71828182845, (x)) /* HACK */
#define fabs fabs_wrapper
#define floor floor_wrapper
#define fmod fmod_wrapper
#define free tlsf_free
#define fscanf fscanf_wrapper
#define getchar() rb->sleep(2*HZ)
#define getenv SDL_getenv
#define log rb_log
#define lseek rb->lseek
#define malloc tlsf_malloc
#define mkdir rb->mkdir
#define opendir rb->opendir
#define pow pow_wrapper
#define printf printf_wrapper
#define putenv(x) /* nothing */
#define puts printf
#define qsort rb->qsort
#define rand rb->rand
#define rb_atexit rbsdl_atexit
#define readdir rb->readdir
#define realloc tlsf_realloc
#define remove rb->remove
#define sin sin_wrapper
#define snprintf rb->snprintf
#define sprintf sprintf_wrapper
#define sqrt sqrt_wrapper
#define srand rb->srand
#define sscanf SDL_sscanf
#define strcasecmp rb->strcasecmp
#define strcat strcat_wrapper
#define strchr rb->strchr
#define strcmp rb->strcmp
#define strcpy strcpy_wrapper
#define strdup strdup_wrapper
#define strerror(x) "error"
#define strlcpy rb->strlcpy
#define strlen rb->strlen
#define strncasecmp rb->strncasecmp
#ifndef strncat
#define strncat rb->strlcat /* hack */
#endif
#define strncmp rb->strncmp
#ifndef strncat
#define strpbrk strpbrk_wrapper
#endif
#define strrchr rb->strrchr
#define strstr SDL_strstr
#define strtok strtok_wrapper
#define strtok_r rb->strtok_r
#define strtol SDL_strtol
#define tan tan_wrapper
#define time(x) (*rb->current_tick/HZ)
#define unlink remove
#define vprintf vprintf_wrapper
#define vsnprintf rb->vsnprintf
#define vsprintf vsprintf_wrapper

// use Rockbox's string routines
#define SDL_memcpy memcpy
#define SDL_memcmp memcmp

#define M_PI 3.14159265358979323846
#define EOF (-1)

#define LOAD_XPM

#define WAV_MUSIC
#define MID_MUSIC
#define USE_TIMIDITY_MIDI

#define FILENAME_MAX MAX_PATH

char *strcat_wrapper(char *dest, const char *src);
char *strcpy_wrapper(char *dest, const char *src);
char *strtok_wrapper(char *str, const char *delim);
double ceil_wrapper(double x);
double cos_wrapper(double);
double floor_wrapper(double n);
double sin_wrapper(double);
float atan2_wrapper(float, float);
float atan_wrapper(float x);
float fabs_wrapper(float);
float fmod(float x, float y);
float pow_wrapper(float x, float y);
float rb_log(float x);
float sqrt_wrapper(float);
float tan_wrapper(float);
int mkdir_wrapepr(const char *path);
int printf_wrapper(const char*, ...);
int sprintf_wrapper(char*, const char*, ...);
int vprintf(const char *fmt, va_list ap);
int fscanf_wrapper(FILE *f, const char *fmt, ...);
void fatal(char *fmt, ...);
void rb_exit(int rc);
void rbsdl_atexit(void (*)(void));
float atof_wrapper (char *str);

/* speed */
static inline uint16_t readLE16(void *addr)
{
    uint8_t *ptr = addr;
    return (*(ptr+1) << 8) | *ptr;
}

static inline uint32_t readLE32(void *addr)
{
    uint8_t *ptr = addr;
    return (*(ptr+3) << 24) |(*(ptr+2) << 16) | (*(ptr+1) << 8) | *ptr;
}

#endif /* _SDL_config_rockbox_h */
