/*
 * Copyright (c) 2000, 2002, 2003 Virtual Unlimited B.V.
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

/*!\file blowfishopt.h
 * \brief Blowfish block cipher, assembler-optimized routines, headers.
 * \author Bob Deblier <bob.deblier@pandora.be>
 * \ingroup BC_blowfish_m
 */

#ifndef _BLOWFISHOPT_H
#define _BLOWFISHOPT_H

#include "beecrypt/beecrypt.h"
#include "beecrypt/blowfish.h"

#ifdef __cplusplus
extern "C" {
#endif

#if WIN32
# if defined(_MSC_VER) && defined(_M_IX86)
#  define ASM_BLOWFISHENCRYPT
#  define ASM_BLOWFISHDECRYPT
# elif __INTEL__ && __MWERKS__
#  define ASM_BLOWFISHENCRYPT
#  define ASM_BLOWFISHDECRYPT
# endif
#endif

#if defined(__GNUC__)
# if defined(OPTIMIZE_I586) || defined(OPTIMIZE_I686)
#  define ASM_BLOWFISHENCRYPT
#  define ASM_BLOWFISHDECRYPT
# endif
# if defined(OPTIMIZE_POWERPC)
#  define ASM_BLOWFISHENCRYPT
#  define ASM_BLOWFISHDECRYPT
# endif
#endif

#if defined(__IBMC__)
# if defined(OPTIMIZE_POWERPC)
#  define ASM_BLOWFISHENCRYPT
#  define ASM_BLOWFISHDECRYPT
# endif
#endif

#if defined(__INTEL_COMPILER)
# if defined(OPTIMIZE_I586) || defined(OPTIMIZE_I686)
#  define ASM_BLOWFISHENCRYPT
#  define ASM_BLOWFISHDECRYPT
# endif
#endif

#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
/* nothing here yet */
#endif

#undef ASM_BLOWFISHENCRYPT
#undef ASM_BLOWFISHDECRYPT

#ifdef __cplusplus
}
#endif

#endif
