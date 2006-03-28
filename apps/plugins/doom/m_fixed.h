// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
// Fixed point arithemtics, implementation.
//
//-----------------------------------------------------------------------------


#ifndef __M_FIXED__
#define __M_FIXED__


#ifdef __GNUG__
#pragma interface
#endif

#include "doomtype.h"
#include "rockmacros.h"

//
// Fixed point, 32bit as 16.16.
//
#define FRACBITS  16
#define FRACUNIT  (1<<FRACBITS)

#define D_abs(x) ({fixed_t _t = (x), _s = _t >> (8*sizeof _t-1); (_t^_s)-_s;})

typedef int fixed_t;

inline static int FixedMul( int a, int b )
{
#if defined(CPU_COLDFIRE) && !defined(SIMULATOR)
   // Code contributed by Thom Johansen
   register int result;
   asm volatile (
      "mac.l   %[x],%[y],%%acc0  \n"   /* multiply */
      "move.l  %[y],%%d2         \n"
      "mulu.l  %[x],%%d2         \n"   /* get lower half, avoid emac stall */
      "movclr.l %%acc0,%[result] \n"   /* get higher half */
      "moveq.l #15,%%d1          \n"
      "asl.l   %%d1,%[result]    \n"   /* hi <<= 15, plus one free */
      "moveq.l #16,%%d1          \n"
      "lsr.l   %%d1,%%d2         \n"   /* (unsigned)lo >>= 16 */
      "or.l    %%d2 ,%[result]   \n"   /* combine result */
   : /* outputs */
      [result]"=&d"(result)
   : /* inputs */
      [x] "d" (a),
      [y] "d" (b)
   : /* clobbers */
      "d1", "d2"
   );
   return result;
#else
   return (fixed_t)((long long) a*b >> FRACBITS);
#endif
}

inline static fixed_t FixedDiv( fixed_t  a, fixed_t  b )
{
   return (D_abs(a)>>14) >= D_abs(b) ? ((a^b)>>31) ^ MAXINT :
          (fixed_t)(((long long) a << FRACBITS) / b);
}

/* CPhipps -
 * FixedMod - returns a % b, guaranteeing 0<=a<b
 * (notice that the C standard for % does not guarantee this)
 */

inline static fixed_t FixedMod(fixed_t a, fixed_t b)
{
   if (b & (b-1)) {
      fixed_t r = a % b;
      return ((r<0) ? r+b : r);
   } else
      return (a & (b-1));
}

#endif
