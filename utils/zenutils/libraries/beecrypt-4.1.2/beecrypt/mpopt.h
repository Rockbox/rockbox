/*
 * Copyright (c) 2003 Bob Deblier
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

/*!\file mpopt.h
 * \brief Multi-precision integer optimization definitions.
 * \author Bob Deblier <bob.deblier@pandora.be>
 * \ingroup MP_m
 */

#ifndef _MPOPT_H
#define _MPOPT_H

#if WIN32
# if __MWERKS__ && __INTEL__
# elif defined(_MSC_VER) && defined(_M_IX86)
#  define ASM_MPZERO
#  define ASM_MPFILL
#  define ASM_MPEVEN
#  define ASM_MPODD
#  define ASM_MPADDW
#  define ASM_MPSUBW
#  define ASM_MPADD
#  define ASM_MPSUB
#  define ASM_MPMULTWO
#  define ASM_MPDIVTWO
#  define ASM_MPSETMUL
#  define ASM_MPADDMUL
#  define ASM_MPADDSQRTRC
# endif
#endif

#if defined(__DECC)
# if defined(OPTIMIZE_ALPHA)
#  define ASM_MPADD
#  define ASM_MPSUB
#  define ASM_MPSETMUL
#  define ASM_MPADDMUL
#  define ASM_MPADDSQRTRC
# endif
#endif

#if defined(__GNUC__)
# if defined(OPTIMIZE_ALPHA)
#  define ASM_MPADD
#  define ASM_MPSUB
#  define ASM_MPSETMUL
#  define ASM_MPADDMUL
#  define ASM_MPADDSQRTRC
# elif defined(OPTIMIZE_ARM)
#  define ASM_MPSETMUL
#  define ASM_MPADDMUL
#  define ASM_MPADDSQRTRC
# elif defined(OPTIMIZE_I386) || defined(OPTIMIZE_I486) || defined(OPTIMIZE_I586) || defined(OPTIMIZE_I686)
#  define ASM_MPZERO
#  define ASM_MPFILL
#  define ASM_MPEVEN
#  define ASM_MPODD
#  define ASM_MPADD
#  define ASM_MPADDW
#  define ASM_MPSUB
#  define ASM_MPSUBW
#  define ASM_MPMULTWO
#  define ASM_MPDIVTWO
#  define ASM_MPSETMUL
#  define ASM_MPADDMUL
#  define ASM_MPADDSQRTRC
#  define ASM_MPPNDIV
# elif defined(OPTIMIZE_IA64)
#  define ASM_MPZERO
#  define ASM_MPCOPY
#  define ASM_MPADD
#  define ASM_MPSUB
#  define ASM_MPMULTWO
#  define ASM_MPSETMUL
#  define ASM_MPADDMUL
# elif defined(OPTIMIZE_M68K)
#  define ASM_MPADD
#  define ASM_MPSUB
#  define ASM_MPSETMUL
#  define ASM_MPADDMUL
#  define ASM_MPADDSQRTRC
# elif defined(OPTIMIZE_POWERPC) || defined(OPTIMIZE_POWERPC64)
#  define ASM_MPSETMUL
#  define ASM_MPADD
#  define ASM_MPADDW
#  define ASM_MPSUB
#  define ASM_MPSUBW
#  define ASM_MPMULTWO
#  define ASM_MPADDMUL
#  define ASM_MPADDSQRTRC
# elif defined(OPTIMIZE_S390X)
#  define ASM_MPSETMUL
#  define ASM_MPADDMUL
#  define ASM_MPADDSQRTRC
# elif defined(OPTIMIZE_SPARCV8)
#  define ASM_MPSETMUL
#  define ASM_MPADDMUL
#  define ASM_MPADDSQRTRC
# elif defined(OPTIMIZE_SPARCV8PLUS)
#  define ASM_MPADDW
#  define ASM_MPSUBW
#  define ASM_MPADD
#  define ASM_MPSUB
#  define ASM_MPMULTWO
#  define ASM_MPSETMUL
#  define ASM_MPADDMUL
#  define ASM_MPADDSQRTRC
# elif defined(OPTIMIZE_X86_64)
#  define ASM_MPZERO
#  define ASM_MPFILL
#  define ASM_MPEVEN
#  define ASM_MPODD
#  define ASM_MPADD
#  define ASM_MPADDW
#  define ASM_MPSUB
#  define ASM_MPSUBW
#  define ASM_MPDIVTWO
#  define ASM_MPMULTWO
#  define ASM_MPSETMUL
#  define ASM_MPADDMUL
#  define ASM_MPADDSQRTRC
# endif
#endif

#if defined(__IBMC__)
# if defined(OPTIMIZE_POWERPC) || defined(OPTIMIZE_POWERPC64)
#  define ASM_MPSETMUL
#  define ASM_MPADDW
#  define ASM_MPSUBW
#  define ASM_MPADD
#  define ASM_MPSUB
#  define ASM_MPMULTWO
#  define ASM_MPADDMUL
#  define ASM_MPADDSQRTRC
# endif
#endif

#if defined(__INTEL_COMPILER)
# if defined(OPTIMIZE_I386) || defined(OPTIMIZE_I486) || defined(OPTIMIZE_I586) || defined(OPTIMIZE_I686)
#  define ASM_MPZERO
#  define ASM_MPFILL
#  define ASM_MPEVEN
#  define ASM_MPODD
#  define ASM_MPADDW
#  define ASM_MPSUBW
#  define ASM_MPADD
#  define ASM_MPSUB
#  define ASM_MPMULTWO
#  define ASM_MPDIVTWO
#  define ASM_MPSETMUL
#  define ASM_MPADDMUL
#  define ASM_MPADDSQRTRC
#  define ASM_MPPNDIV
# endif
#endif

#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
# if defined(OPTIMIZE_SPARCV8)
#  define ASM_MPSETMUL
#  define ASM_MPADDMUL
#  define ASM_MPADDSQRTRC
# elif defined(OPTIMIZE_SPARCV8PLUS)
#  define ASM_MPADDW
#  define ASM_MPSUBW
#  define ASM_MPADD
#  define ASM_MPSUB
#  define ASM_MPMULTWO
#  define ASM_MPSETMUL
#  define ASM_MPADDMUL
#  define ASM_MPADDSQRTRC
# endif
#endif

#undef ASM_MPZERO
#undef ASM_MPFILL
#undef ASM_MPEVEN
#undef ASM_MPODD
#undef ASM_MPADD
#undef ASM_MPADDW
#undef ASM_MPSUB
#undef ASM_MPSUBW
#undef ASM_MPMULTWO
#undef ASM_MPDIVTWO
#undef ASM_MPSETMUL
#undef ASM_MPADDMUL
#undef ASM_MPADDSQRTRC
#undef ASM_MPPNDIV

#endif
