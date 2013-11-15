/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: keyboard.h,v 1.4 1996/08/26 15:05:03 ahornby Exp $
******************************************************************************/

/*
  Keyboard prototypes.
  */

#ifndef KEYBOARD_H
#define KEYBOARD_H

enum control {STICK, PADDLE, KEYPAD};

int
mouse_position(void);

int
mouse_button(void);

void
read_trigger(void);

void
read_stick(void);

void 
read_keypad(int pad);

void
read_console(void);

void
init_keyboard(void);

void
close_keyboard(void);

void
read_keyboard(void);

#endif
