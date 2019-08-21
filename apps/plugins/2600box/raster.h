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

#define VERSION_RASTER "2.3"

#include "rbconfig.h"
#include "rb_test.h"
#include "vmachine.h"


/* Colour table */
#define P0M0_COLOUR     0
#define P1M1_COLOUR     1
#define PFBL_COLOUR     2
#define BK_COLOUR       3

extern unsigned colour_table[4];

/* We have 7 mask bits (2 Players, 2 Missiles, Ball, Left and Right Playfield),
   but since left and right half of playfield cannot exist at the same time we
   have a maximum collision range of 0x60, not 0x80  */
#define MASK_MAX    0x60

/* The collision lookup table */
extern unsigned short col_table[MASK_MAX];

/* The collision state */
extern unsigned short collisions;
extern void init_raster(void);

int do_tia_write(int a, int b);
//int do_tia_write(unsigned a, unsigned b);
void do_raster(int xmin, int xmax);

/* delay for tia write commands. Negative for commands that don't need to be rendered */
extern void (*tia_call_tab[0x40])(unsigned, unsigned);

#endif
