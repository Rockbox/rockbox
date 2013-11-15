/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: display.h,v 1.13 1997/11/22 14:29:54 ahornby Exp $
******************************************************************************/

/*
  Defines for the X11 display code.
  */

#ifndef DISPLAY_H
#define DISPLAY_H
int fselLoad(void);
void rate_update(void);
void tv_off(void);
int tv_on(int argc, char **argv);
void tv_event(void);
void tv_display(void);
void tv_putpixel(int x, int y, BYTE value);
#if LCD_WIDTH >= 320
extern unsigned int tv_color(BYTE b);
#else
//extern BYTE tv_color(BYTE b);
extern unsigned short tv_color(BYTE b);
#endif

extern int redraw_flag;

extern BYTE *vscreen;
extern int vwidth,vheight,theight;
extern int tv_counter;
extern int tv_depth;
extern int tv_bytes_pp;
extern unsigned long tv_red_mask;
extern unsigned long tv_blue_mask;
extern unsigned long tv_green_mask;
#endif





