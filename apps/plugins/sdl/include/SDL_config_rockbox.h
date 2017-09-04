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

/* This is the minimal configuration that can be used to build SDL */

#include "plugin.h"
#include "lib/pluginlib_exit.h"

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <tlsf.h>

#define COMBINED_SDL

/* Enable the dummy audio driver (src/audio/dummy/\*.c) */
//#define SDL_AUDIO_DRIVER_DISK  1

#ifdef SIMULATOR
#define RB_SAMPR SAMPR_44
#else
#define RB_SAMPR SAMPR_16
#endif

/* woot */
#define SDL_AUDIO_DRIVER_ROCKBOX 1

/* Enable the stub cdrom driver (src/cdrom/dummy/\*.c) */
#define SDL_CDROM_DISABLED      1

/* Enable the stub joystick driver (src/joystick/dummy/\*.c) */
#define SDL_JOYSTICK_DISABLED   1

/* Enable the stub shared object loader (src/loadso/dummy/\*.c) */
#define SDL_LOADSO_DISABLED     1

/* Enable the stub thread support (src/thread/generic/\*.c) */
//#define SDL_THREADS_DISABLED    1

/* WIP */
#define SDL_THREAD_ROCKBOX     1

#define SDL_TIMER_ROCKBOX 1

#define SDL_VIDEO_DRIVER_ROCKBOX  1

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

#undef strdup

#define atan2 atan2_wrapper
#define atexit rb_atexit
#define atoi rb->atoi
#define calloc tlsf_calloc
#define SDL_calloc calloc
#define cos cos_wrapper
#define fmod fmod_wrapper
#define free tlsf_free
#define malloc tlsf_malloc
#define mkdir rb->mkdir
#define printf printf_wrapper
#define puts printf
#define rand rb->rand
#define rb_atexit rbsdl_atexit
#define realloc tlsf_realloc
#define remove rb->remove
#define sin sin_wrapper
#define sprintf sprintf_wrapper
#define sscanf SDL_sscanf
#define strcat strcat_wrapper
#define strcmp rb->strcmp
#define strcpy strcpy_wrapper
#define strdup strdup_wrapper
#define strlen rb->strlen
#define strncmp rb->strncmp
#define strrchr rb->strrchr
#define strstr SDL_strstr
#define strtol SDL_strtol
#define qsort rb->qsort
#define snprintf rb->snprintf
#define getenv SDL_getenv
#define strchr SDL_strchr
#define strpbrk strpbrk_wrapper
#define sqrt sqrt_wrapper
#define tan tan_wrapper
#define fabs fabs_wrapper
#define lseek rb->lseek
#define vsnprintf rb->vsnprintf
#define vsprintf vsprintf_wrapper
#define unlink remove
#define pow pow_wrapper
#define floor floor_wrapper
#define log rb_log
#define atan atan_wrapper

#define assert(x) (0)

#define M_PI 4.0

#define LOAD_XPM

#define FILENAME_MAX MAX_PATH

char *strcpy_wrapper(char *dest, const char *src);
double cos_wrapper(double);
double sin_wrapper(double);
float atan2_wrapper(float, float);
float fmod(float x, float y);
int mkdir_wrapepr(const char *path);
int printf_wrapper(const char*, ...);
int sprintf_wrapper(char*, const char*, ...);
void rbsdl_atexit(void (*)(void));
float sqrt_wrapper(float);
float tan_wrapper(float);
float fabs_wrapper(float);
float atan_wrapper(float x);
float pow_wrapper(float x, float y);
float rb_log(float x);
float floor_wrapper(float n);

/* HACK */
//#define Mix_Chunk void
//#define MIX_CHANNELS 1

#endif /* _SDL_config_rockbox_h */
