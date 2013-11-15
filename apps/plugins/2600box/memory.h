/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: memory.h,v 1.5 1996/11/24 16:55:40 ahornby Exp $
******************************************************************************/

/*
  Prototypes for the memory interface.
  */

#ifndef VCSMEMORY_H
#define VCSMEMORY_H

inline BYTE 
undecRead (ADDRESS a);

void 
decWrite ( ADDRESS a, BYTE b);

BYTE 
decRead (ADDRESS a);

BYTE 
dbgRead (ADDRESS a);


#endif
