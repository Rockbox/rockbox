/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: limiter.h,v 1.4 1996/04/01 14:51:50 alex Exp $
******************************************************************************/

/*
  Prototypes for the speed limiter.
  Ready for that 500Mhz DEC Alpha.
  
  Inspired by Brad Pitzels C++ Timer class.
  */

#ifndef LIMITER_H
#define LIMITER_H

void limiter_setFrameRate( int f );
void limiter_init(void);
long limiter_sync(void);
int limiter_getFrameRate(void);

#endif





