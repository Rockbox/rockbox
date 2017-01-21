/*
  Simple DirectMedia Layer
  Copyright (C) 2017 Franklin Wei

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

#ifndef _SDL_config_rockbox_h
#define _SDL_config_rockbox_h

#include "SDL_platform.h"

#include "tlsf.h"

/**
 *  \file SDL_config_rockbox.h
 *
 *  This is the Rockbox SDL config.
 */

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

/* Enable the stub shared object loader (src/loadso/dummy/\*.c) */
#define SDL_LOADSO_DISABLED 1

/* Enable the stub thread support (src/thread/generic/\*.c) */
#define SDL_THREADS_DISABLED    1

/* Enable the stub timer support (src/timer/dummy/\*.c) */
#define SDL_TIMERS_DISABLED 1

/* Enable the dummy video driver (src/video/dummy/\*.c) */
//#define SDL_VIDEO_DRIVER_DUMMY  1
#define SDL_VIDEO_DRIVER_DUMMY 1

/* Enable the dummy filesystem driver (src/filesystem/dummy/\*.c) */
#define SDL_FILESYSTEM_DUMMY  1

#define SDL_ATOMIC_DISABLED 1

#define SDL_CDROM_DUMMY 1

#define LACKS_UNISTD_H    1
#define LACKS_STDLIB_H    1
#define LACKS_STIDO_H     1
#define LACKS_MMAN_H      1
#define LACKS_SYS_PARAM_H 1
#define LACKS_SYS_MMAN_H  1

#define HAVE_MALLOC       1
#define HAVE_FREE         1
#define HAVE_REALLOC      1
#define malloc tlsf_malloc
#define calloc tlsf_calloc
#define realloc tlsf_realloc
#define free tlsf_free

#endif /* _SDL_config_rockbox_h */
