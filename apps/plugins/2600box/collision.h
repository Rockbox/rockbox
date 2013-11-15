/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: collision.h,v 1.5 1996/08/29 16:03:24 ahornby Exp $
******************************************************************************/

/*
  Defines for the hardware collision detection.
  */

#ifndef COLLISION_H
#define COLLISION_H

#include "col_mask.h"

/* The collsion vector */
extern BYTE* colvect;

/* The collision lookup table */
extern unsigned short col_table[256];

/* The collision state */
extern unsigned short col_state;

extern void
init_collisions(void);

extern void 
reset_collisions(void);

extern int
set_collisions(BYTE b); 

#endif






