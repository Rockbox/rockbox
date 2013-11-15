/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: exmacro.h,v 1.5 1996/11/24 16:55:40 ahornby Exp $
******************************************************************************/

/*
  Defines inline functions that would otherwise be macros.
  */

#ifndef EXMACRO_H
#define EXMACRO_H

inline ADDRESS load_abs_addr(void);
inline int pagetest( ADDRESS a, BYTE b);
inline int brtest( BYTE a);
inline int toBCD( int a);
inline int fromBCD( int a);

#endif

