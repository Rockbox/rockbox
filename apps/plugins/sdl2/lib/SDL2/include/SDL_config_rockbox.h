/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2020 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef SDL_config_rockbox_h_
#define SDL_config_rockbox_h_
#define SDL_config_h_

#include "SDL_platform.h"

/**
 *  \file SDL_config_rockbox.h
 *
 *  Rockbox SDL config.
 */

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



#define HAVE_STDARG_H   1
#define HAVE_STDDEF_H   1

/* Most everything except Visual Studio 2008 and earlier has stdint.h now */
#if defined(_MSC_VER) && (_MSC_VER < 1600)
/* Here are some reasonable defaults */
typedef unsigned int size_t;
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned long uintptr_t;
#else
#define HAVE_STDINT_H 1
#endif /* Visual Studio 2008 */

#ifdef __GNUC__
#define HAVE_GCC_SYNC_LOCK_TEST_AND_SET 1
#endif

/* Enable the dummy audio driver (src/audio/dummy/\*.c) */
#define SDL_AUDIO_DRIVER_DUMMY  1

/* Enable the stub joystick driver (src/joystick/dummy/\*.c) */
#define SDL_JOYSTICK_DISABLED   1

/* Enable the stub haptic driver (src/haptic/dummy/\*.c) */
#define SDL_HAPTIC_DISABLED 1

/* Enable the stub sensor driver (src/sensor/dummy/\*.c) */
#define SDL_SENSOR_DISABLED 1

/* Enable the stub shared object loader (src/loadso/dummy/\*.c) */
#define SDL_LOADSO_DISABLED 1

/* Enable the stub thread support (src/thread/generic/\*.c) */
#define SDL_THREADS_DISABLED    0
#define SDL_THREAD_ROCKBOX      1

#define SDL_TIMERS_DISABLED      0
#define SDL_TIMER_ROCKBOX        1

/* Enable the dummy video driver (src/video/dummy/\*.c) */
#define SDL_VIDEO_DRIVER_DUMMY  1

/* Enable the dummy filesystem driver (src/filesystem/dummy/\*.c) */
#define SDL_FILESYSTEM_DUMMY  1

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

#define  HAVE_CEIL                       1
#define  HAVE_COS                        1
#define  HAVE_EXP                        1
#define  HAVE_FABS                       1
#define  HAVE_FLOOR                      1
#define  HAVE_FMOD                       1
#define  HAVE_LOG                        1
#define  HAVE_POW                        1
#define  HAVE_SIN                        1
#define  HAVE_SQRT                       1
#define  HAVE_TAN                        1
#define  HAVE_ABS                        1
#define  HAVE_CTYPE_H                    1
#define  HAVE_LIBC                       1

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

#define M_PI 3.14159265358979323846
#define EOF (-1)

#define LOAD_XPM
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

#endif /* SDL_config_minimal_h_ */
