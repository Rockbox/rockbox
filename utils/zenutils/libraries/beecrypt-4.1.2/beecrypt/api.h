/*
 * Copyright (c) 2001, 2002, 2004 Beeyond Software Holding BV
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

/*!\file api.h
 * \brief BeeCrypt API, portability headers.
 * \author Bob Deblier <bob.deblier@pandora.be>
 */

#ifndef _BEECRYPT_API_H
#define _BEECRYPT_API_H

#if defined(_WIN32) && !defined(WIN32)
# define WIN32 1
#endif

#if WIN32
# if !__CYGWIN32__
#  include "beecrypt/win.h"
# else
#  include "beecrypt/gnu.h"
# endif
# ifdef BEECRYPT_DLL_EXPORT
#  define BEECRYPTAPI //__declspec(dllexport)
# else
#  define BEECRYPTAPI //__declspec(dllimport)
# endif
# ifdef BEECRYPT_CXX_DLL_EXPORT
#  define BEECRYPTCXXAPI //__declspec(dllexport)
#  define BEECRYPTCXXTEMPLATE
# else
#  define BEECRYPTCXXAPI //__declspec(dllimport)
#  define BEECRYPTCXXTEMPLATE extern
# endif
#else
# include "beecrypt/gnu.h"
# define BEECRYPTAPI
# define BEECRYPTCXXAPI
#endif

#ifndef ROTL32
# define ROTL32(x, s) (((x) << (s)) | ((x) >> (32 - (s))))
#endif
#ifndef ROTR32
# define ROTR32(x, s) (((x) >> (s)) | ((x) << (32 - (s))))
#endif
#ifndef ROTR64
# define ROTR64(x, s) (((x) >> (s)) | ((x) << (64 - (s))))
#endif

typedef uint8_t		byte;

typedef int8_t		javabyte;
typedef int16_t		javashort;
typedef int32_t		javaint;
typedef int64_t		javalong;

typedef uint16_t	javachar;

#if (MP_WBITS == 64)
typedef uint64_t	mpw;
typedef uint32_t	mphw;
#elif (MP_WBITS == 32)
# if HAVE_UINT64_T
#  define HAVE_MPDW 1
typedef uint64_t	mpdw;
# endif
typedef uint32_t	mpw;
typedef uint16_t	mphw;
#else
# error
#endif

#endif
