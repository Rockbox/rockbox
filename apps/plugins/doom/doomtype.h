/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *      Simple basic typedefs, isolated here to make it easier
 *       separating modules.
 *
 *-----------------------------------------------------------------------------*/
#ifndef __DOOMTYPE__
#define __DOOMTYPE__
#include "rockmacros.h"

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
// Fixed to use builtin bool type with C++.
#ifdef __cplusplus
typedef bool boolean;
#else
//typedef enum {false, true} boolean;
//#define boolean bool
typedef enum _boolean { FALSE, TRUE } boolean;
#endif
typedef unsigned char byte;
#endif

typedef signed long long int_64_t;
typedef unsigned long long uint_64_t;

#define MAXCHAR  ((char)0x7f)
#define MAXSHORT ((short)0x7fff)

// Max pos 32-bit int.
#define MAXINT  ((int)0x7fffffff)
#define MAXLONG  ((long)0x7fffffff)
#define MINCHAR  ((char)0x80)
#define MINSHORT ((short)0x8000)

// Max negative 32-bit integer.
#define MININT  ((int)0x80000000)
#define MINLONG  ((long)0x80000000)

/* cph - move compatibility levels here so we can use them in d_server.c */
typedef enum {
   doom_12_compatibility, /* Behave like early doom versions */
   doom_demo_compatibility, /* As compatible as possible for
           * playing original Doom demos */
   doom_compatibility,      /* Compatible with original Doom levels */
   boom_compatibility_compatibility,      /* Boom's compatibility mode */
   boom_201_compatibility,                /* Compatible with Boom v2.01 */
   boom_202_compatibility,                /* Compatible with Boom v2.01 */
   lxdoom_1_compatibility,                /* LxDoom v1.3.2+ */
   mbf_compatibility,                     /* MBF */
   prboom_1_compatibility,                /* PrBoom 2.03beta? */
   prboom_2_compatibility,                /* PrBoom 2.1.0-2.1.1 */
   prboom_3_compatibility,                /* Latest PrBoom */
   MAX_COMPATIBILITY_LEVEL,               /* Must be last entry */
   /* Aliases follow */
   boom_compatibility = boom_201_compatibility, /* Alias used by G_Compatibility */
   best_compatibility = prboom_3_compatibility,
} complevel_t;

#endif
