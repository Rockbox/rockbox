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
#define SDL_AUDIO_DRIVER_DUMMY  1

/* Enable the stub cdrom driver (src/cdrom/dummy/\*.c) */
#define SDL_CDROM_DISABLED      1

/* Enable the stub joystick driver (src/joystick/dummy/\*.c) */
#define SDL_JOYSTICK_DISABLED   1

/* Enable the stub shared object loader (src/loadso/dummy/\*.c) */
#define SDL_LOADSO_DISABLED     1

/* Enable the stub thread support (src/thread/generic/\*.c) */
#define SDL_THREADS_DISABLED    1

/* WIP */
//#define SDL_THREAD_ROCKBOX     1

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

#define malloc tlsf_malloc
#define calloc tlsf_calloc
#define realloc tlsf_realloc
#define free tlsf_free
#define strrchr rb->strrchr
#define atexit rb_atexit
#define strncmp rb->strncmp
#define cos cos_wrapper
#define sin sin_wrapper
#define printf printf_wrapper
#define rand rb->rand
#define atoi rb->atoi
#define strcmp rb->strcmp
#define rb_atexit rbsdl_atexit
#define strtol SDL_strtol
#define sscanf SDL_sscanf
#define sprintf sprintf_wrapper
#define mkdir rb->mkdir
#define remove rb->remove
#define strdup strdup_wrapper
#define strlen rb->strlen
#define strcpy strcpy_wrapper
#define strstr strstr_wrapper
#define strcat strcat_wrapper

#define LOAD_XPM

double sin_wrapper(double);
double cos_wrapper(double);
int printf_wrapper(const char*, ...);
void rbsdl_atexit(void (*)(void));
int sprintf_wrapper(char*, const char*, ...);
int mkdir_wrapepr(const char *path);

#endif /* _SDL_config_rockbox_h */
