/*****************************************************************************

   This file is part of Virtual 2600, the Atari 2600 Emulator
   ==========================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: keyboard.c,v 1.18 1997/11/22 14:27:47 ahornby Exp $
******************************************************************************/

/* The device independant keyboard and mouse interface. */

#include <stdio.h>

#include "rbconfig.h"
#include "options.h"
#include "types.h"
#include "address.h"
#include "vmachine.h"
#include "macro.h"
#include "extern.h"
#include "memory.h"
#include "kmap.h"
#include "realjoy.h"
#include "display.h"
#include "keybdrv.h"
#include "moudrv.h"

enum control {STICK, PADDLE, KEYPAD};

enum joydirs {JLEFT, JRIGHT, JUP, JDOWN, JFIRE};
int joykeys[2][5]={ 
  {kmapLARROW, kmapRARROW, kmapUARROW, kmapDARROW, kmapSPACE},
  {kmapA, kmapS, kmapW, kmapZ, kmapLEFTALT}};

int padkeys[2][4][3]={ { {kmap1,kmap2,kmap3}, 
			 {kmapQ, kmapW, kmapE},
			 {kmapA, kmapS, kmapD},
			 {kmapZ, kmapX, kmapD}},
		       { {kmap5,kmap6,kmap7},
			 {kmapT, kmapY, kmapU},
			 {kmapG, kmapH, kmapJ},
			 {kmapB, kmapN, kmapM}}};

int moudrv_x, moudrv_y, moudrv_but;

static int raw;

/* Stores the keycodes */
int keymap[103];

/* Returns mouse position */
int
mouse_position (void)
{
  int res;
  moudrv_read();
  
  if(base_opts.mousey==1)
    res=(((moudrv_y/base_opts.magstep)*320)/200);
  else
    res=moudrv_x/base_opts.magstep;
  
  return res;
}

/* Returns mouse button state */
int
mouse_button (void)
{
  moudrv_read();
  return moudrv_but;
}

/* Read from the keypad */
void 
read_keypad(int pad) 
{
  int i,row,col;
  
  for(i=0; i<4; i++) 
    keypad[pad][i]=0xff;

  for(row=0;row<4;row++)
    {
      for(col=0;col<3;col++)
	{
	  if (keybdrv_pressed(padkeys[pad][row][col])) 
	    keypad[pad][row]=col;
	}
    }
}


/* Read from the emulated joystick */
void
read_stick (void)
{
  BYTE v[2];
  int i,jres;

  v[0] = v[1] = 0x0f;

  int buttons = rb->button_status();
      if (buttons & JOY_LEFT)
          v[0] &= 0x0B;
      if (buttons & JOY_RIGHT)
          v[0] &= 0x07;
      if (buttons & JOY_UP)
          v[0] &= 0x0E;
      if (buttons & JOY_DOWN)
          v[0] &= 0x0D;

  for(i=0;i<2;i++)
    {
      jres = get_realjoy (i);
      if (keybdrv_pressed(joykeys[i][JUP]) || (jres & UJOYMASK))
	v[i] &= 0x0E;
      else if (keybdrv_pressed(joykeys[i][JDOWN]) || (jres & DJOYMASK))
	v[i] &= 0x0D;
      if (keybdrv_pressed(joykeys[i][JLEFT]) || (jres & LJOYMASK))
	v[i] &= 0x0B;
      else if (keybdrv_pressed(joykeys[i][JRIGHT]) || (jres & RJOYMASK))
	v[i] &= 0x07;
    }

  /* Swap if necessary */
  if (base_opts.swap)
    riotRead[SWCHA] = (v[1] << 4) | v[0];
  else
    riotRead[SWCHA] = (v[0] << 4) | v[1];
}

/* Read from emulated joystick trigger */
void
read_trigger (void)
{
  int kr = 0x80, kl = 0x80, temp;
  int jres;

  int buttons = rb->button_status();
    if (buttons & JOY_TRIGGER)
        kr = 0x00;

  /* keyboard_update(); */
  /* update_realjoy(); */

  /* Left Player */
  jres = get_realjoy (1);
  if (keybdrv_pressed(joykeys[1][JFIRE]) || (jres & B1JOYMASK))
    kl = 0x00;

  /* Right Player */
  jres = get_realjoy (0);
  if (keybdrv_pressed(joykeys[0][JFIRE]) || (jres & B1JOYMASK))
    kr = 0x00;

  /* Swap the buttons if necessary */
  if (base_opts.swap)
    {
      temp = kl;
      kl = kr;
      kr = temp;
    }

  /* The normal non-latched mode */
  if (!(tiaWrite[VBLANK] & 0x40))
    {
      tiaRead[INPT4] = kr;
      tiaRead[INPT5] = kl;
    }
  /* The latched mode */
  else
    {
      if (kr == 0)
	tiaRead[INPT4] = 0x00;
      if (kl == 0)
	tiaRead[INPT5] = 0x00;
    }
}

/* Reads the keyboard */
void
read_keyboard (void)
{
  if (raw)
    {
      keybdrv_update ();
      moudrv_update ();
    }
  //if (keybdrv_pressed(kmapF10))
    //exit (0);
}

/* Read console switches */
void
read_console (void)
{
  riotRead[SWCHB] |= 0x03;
  int buttons = rb->button_status();
    if (buttons & CONSOLE_OPTION)
        riotRead[SWCHB] &= 0xFD;
    if (buttons & CONSOLE_RESET)
        riotRead[SWCHB] &= 0xFE;

  if (keybdrv_pressed(kmapF2))
    riotRead[SWCHB] &= 0xFE; /* Reset */
  if (keybdrv_pressed(kmapF3))
    riotRead[SWCHB] &= 0xFD; /* Select */

  if (keybdrv_pressed(kmapF4))
    {
      if (riotRead[SWCHB] & 0x08)
	/* BW */
	riotRead[SWCHB] &= 0xF7;
      else
	/* Color */
	riotRead[SWCHB] |= 0x08;
      while (keybdrv_pressed(kmapF4))
	keybdrv_update ();
    }

  if (keybdrv_pressed(kmapF5))
    riotRead[SWCHB] &= 0xBF;	/* P0 amateur */
  else if (keybdrv_pressed(kmapF6))
    riotRead[SWCHB] |= 0x40;	/* P0 pro */
  if (keybdrv_pressed(kmapF7))
    riotRead[SWCHB] &= 0x7f;	/* P1 amateur */
  else if (keybdrv_pressed(kmapF8))
    riotRead[SWCHB] |= 0x80;	/* P1 pro */
}

/* Close the keyboard */
void
close_keyboard (void)
{
  moudrv_close();
  keybdrv_close ();
  raw=0;
}

/* Initialises the keyboard */
void
init_keyboard (void)
{
  keybdrv_init ();
  keybdrv_setmap();
  moudrv_init();
  raw=1;
}

