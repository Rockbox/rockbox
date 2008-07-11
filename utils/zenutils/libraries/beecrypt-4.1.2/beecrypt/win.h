/*
 * Copyright (c) 2000, 2001, 2002 Virtual Unlimited B.V.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*!\file win.h
 * \brief BeeCrypt API, windows headers.
 * \author Bob Deblier <bob.deblier@pandora.be>
 */

#ifndef _BEECRYPT_WIN_H
#define _BEECRYPT_WIN_H

#define _REENTRANT

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0400
#endif

#include <windows.h>

#if __MWERKS__
# if __INTEL__
#  define WORDS_BIGENDIAN		0
# else
#  error Unknown CPU type in MetroWerks CodeWarrior
# endif
#elif defined(_MSC_VER)
# if defined(_M_IX86)
#  define WORDS_BIGENDIAN		0
#  define ROTL32(x, s) _rotl(x, s)
#  define ROTR32(x, s) _rotr(x, s)
# else
#  error Unknown CPU type in Microsoft Visual C
# endif
#else
# error Unknown compiler for WIN32
#endif

#if defined(_MSC_VER) || __MWERKS__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HAVE_ERRNO_H			1
#define HAVE_CTYPE_H			1
#define HAVE_FCNTL_H			1
#define HAVE_TIME_H				1

#define HAVE_SYS_TYPES_H		0
#define HAVE_SYS_TIME_H			0

#define HAVE_THREAD_H			0
#define HAVE_SYNCH_H			0
#define HAVE_PTHREAD_H			0
#define HAVE_SEMAPHORE_H		0

#define HAVE_TERMIO_H			0
#define HAVE_SYS_AUDIOIO_H		0
#define HAVE_SYS_IOCTL_H		0
#define HAVE_SYS_SOUNDCARD_H	0

#define HAVE_GETTIMEOFDAY		0
#define HAVE_GETHRTIME			0

#define HAVE_DEV_TTY			0
#define HAVE_DEV_AUDIO			0
#define HAVE_DEV_DSP			0
#define HAVE_DEV_RANDOM			0
#define HAVE_DEV_URANDOM		0
#define HAVE_DEV_TTY			0

#else
#error Not set up for this compiler
#endif

#if __MWERKS__
#define HAVE_SYS_STAT_H			0

#define HAVE_LONG_LONG			1
#define HAVE_UNSIGNED_LONG_LONG	1

#define HAVE_64_BIT_INT			1
#define HAVE_64_BIT_UINT		1

typedef char		int8_t;
typedef short		int16_t;
typedef long		int32_t;
typedef long long	int64_t;

typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned long		uint32_t;
typedef unsigned long long	uint64_t;

#elif defined(_MSC_VER)
#define HAVE_SYS_STAT_H			1

#define HAVE_LONG_LONG			0
#define HAVE_UNSIGNED_LONG_LONG	0

#define HAVE_64_BIT_INT			1
#define HAVE_64_BIT_UINT		1

typedef signed char	int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed __int64 int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;

typedef long off_t;

#endif

#define MP_WBITS	32U

typedef HANDLE bc_cond_t;
typedef HANDLE bc_mutex_t;
typedef HANDLE bc_thread_t;

#endif
