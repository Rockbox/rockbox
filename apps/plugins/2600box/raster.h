/*****************************************************************************

   This file is part of Virtual 2600, the Atari 2600 Emulator
   ==========================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: raster.h,v 1.5 1997/04/06 02:15:13 ahornby Exp $
******************************************************************************/

/*
  The raster prototypes.
  */

#ifndef RASTER_H
#define RASTER_H
/* Color lookup tables. Used to speed up rendering */
/* The current colour lookup table */
extern int *colour_lookup;  

/* Colour table */
#define P0M0_COLOUR 0
#define P1M1_COLOUR 1
#define PFBL_COLOUR 2
#define BK_COLOUR 3
extern unsigned int colour_table[4];

/* normal/alternate, not scores/scores*/
extern int norm_val, scores_val;
extern int *colour_ptrs[2][3];

void tv_raster(int line);

extern void 
init_raster(void);
#endif



