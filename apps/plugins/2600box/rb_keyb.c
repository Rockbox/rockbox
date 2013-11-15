/*****************************************************************************

   This file is part of Virtual 2600, the Atari 2600 Emulator
   ==========================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: dos_keyb.c,v 1.1 1996/08/28 11:26:57 ahornby Exp $
******************************************************************************/

/* The keyboard interface. */
#include <stdio.h>

#include "rbconfig.h"
#include "options.h"
#include "types.h"
#include "address.h"
#include "vmachine.h"
#include "macro.h"
#include "extern.h"
#include "memory.h"
#include "display.h"
#include "kmap.h"

enum control {STICK, PADDLE, KEYPAD};

extern int keymap[103];

/* Update the current keyboard state. */
void
keybdrv_update (void)
{
}

/* Is this key presssed? */
int
keybdrv_pressed(int kval)
{
#if 0
  return (key[keymap[kval]]);
#endif
  return 0;
}

/* Defines the keyboard mappings */
void
keybdrv_setmap(void)
{
#if 0
  keymap[  kmapSYSREQ              ]=0;
  keymap[  kmapCAPSLOCK            ]=KEY_CAPSLOCK;
  keymap[  kmapNUMLOCK             ]=KEY_NUMLOCK;
  keymap[  kmapSCROLLLOCK          ]=0;
  keymap[  kmapLEFTCTRL            ]=KEY_CONTROL;
  keymap[  kmapLEFTALT             ]=KEY_ALT;
  keymap[  kmapLEFTSHIFT           ]=KEY_LSHIFT;
  keymap[  kmapRIGHTCTRL           ]=KEY_CONTROL;
  keymap[  kmapRIGHTALT            ]=KEY_ALT;
  keymap[  kmapRIGHTSHIFT          ]=KEY_RSHIFT;
  keymap[  kmapESC                 ]=KEY_ESC;
  keymap[  kmapBACKSPACE           ]=KEY_BACKSPACE;
  keymap[  kmapENTER               ]=KEY_ENTER;
  keymap[  kmapSPACE               ]=KEY_SPACE;
  keymap[  kmapTAB                 ]=KEY_TAB;
  keymap[  kmapF1                  ]=KEY_F1;
  keymap[  kmapF2                  ]=KEY_F2;
  keymap[  kmapF3                  ]=KEY_F3;
  keymap[  kmapF4                  ]=KEY_F4;
  keymap[  kmapF5                  ]=KEY_F5;
  keymap[  kmapF6                  ]=KEY_F6;
  keymap[  kmapF7                  ]=KEY_F7;
  keymap[  kmapF8                  ]=KEY_F8;
  keymap[  kmapF9                  ]=KEY_F9;
  keymap[  kmapF10                 ]=KEY_F10;
  keymap[  kmapF11                 ]=KEY_F11;
  keymap[  kmapF12                 ]=KEY_F12;
  keymap[  kmapA                   ]=KEY_A;
  keymap[  kmapB                   ]=KEY_B;
  keymap[  kmapC                   ]=KEY_C;
  keymap[  kmapD                   ]=KEY_D;
  keymap[  kmapE                   ]=KEY_E;
  keymap[  kmapF                   ]=KEY_F;
  keymap[  kmapG                   ]=KEY_G;
  keymap[  kmapH                   ]=KEY_H;
  keymap[  kmapI                   ]=KEY_I;
  keymap[  kmapJ                   ]=KEY_J;
  keymap[  kmapK                   ]=KEY_K;
  keymap[  kmapL                   ]=KEY_L;
  keymap[  kmapM                   ]=KEY_M;
  keymap[  kmapN                   ]=KEY_N;
  keymap[  kmapO                   ]=KEY_O;
  keymap[  kmapP                   ]=KEY_P;
  keymap[  kmapQ                   ]=KEY_Q;
  keymap[  kmapR                   ]=KEY_R;
  keymap[  kmapS                   ]=KEY_S;
  keymap[  kmapT                   ]=KEY_T;
  keymap[  kmapU                   ]=KEY_U;
  keymap[  kmapV                   ]=KEY_V;
  keymap[  kmapW                   ]=KEY_W ;
  keymap[  kmapX                   ]=KEY_X ;
  keymap[  kmapY                   ]=KEY_Y;
  keymap[  kmapZ                   ]=KEY_Z ;
  keymap[  kmap1                   ]=KEY_1;
  keymap[  kmap2                   ]=KEY_2;
  keymap[  kmap3                   ]=KEY_3;
  keymap[  kmap4                   ]=KEY_4;
  keymap[  kmap5                   ]=KEY_5;
  keymap[  kmap6                   ]=KEY_6;
  keymap[  kmap7                   ]=KEY_7;
  keymap[  kmap8                   ]=KEY_8;
  keymap[  kmap9                   ]=KEY_9;
  keymap[  kmap0                   ]=KEY_0;
  keymap[  kmapMINUS               ]=KEY_MINUS;
  keymap[  kmapEQUAL               ]=0;
  keymap[  kmapLBRACKET            ]=0x1A;
  keymap[  kmapRBRACKET            ]=0x1B;
  keymap[  kmapSEMICOLON           ]=0x27;
  keymap[  kmapTICK                ]=0x28;
  keymap[  kmapAPOSTROPHE          ]=0x29;
  keymap[  kmapBACKSLASH           ]=0x2B;
  keymap[  kmapCOMMA               ]=0x33;
  keymap[  kmapPERIOD              ]=0x34;
  keymap[  kmapSLASH               ]=0x35;
  keymap[  kmapINS                 ]=0xD2;
  keymap[  kmapDEL                 ]=0xD3;
  keymap[  kmapHOME                ]=0xC7;
  keymap[  kmapEND                 ]=0xCF;
  keymap[  kmapPGUP                ]=0xC9;
  keymap[  kmapPGDN                ]=0xD1;
  keymap[  kmapLARROW              ]=KEY_LEFT;
  keymap[  kmapRARROW              ]=KEY_RIGHT;
  keymap[  kmapUARROW              ]=KEY_UP;
  keymap[  kmapDARROW              ]=KEY_DOWN;
  keymap[  kmapKEYPAD0             ]=KEY_0;
  keymap[  kmapKEYPAD1             ]=KEY_1;
  keymap[  kmapKEYPAD2             ]=KEY_2;
  keymap[  kmapKEYPAD3             ]=KEY_3;
  keymap[  kmapKEYPAD4             ]=KEY_4;
  keymap[  kmapKEYPAD5             ]=KEY_5;
  keymap[  kmapKEYPAD6             ]=KEY_6;
  keymap[  kmapKEYPAD7             ]=KEY_7;
  keymap[  kmapKEYPAD8             ]=KEY_8;
  keymap[  kmapKEYPAD9             ]=KEY_9;
  keymap[  kmapKEYPADDEL           ]=0;
  keymap[  kmapKEYPADSTAR          ]=0;
  keymap[  kmapKEYPADMINUS         ]=KEY_MINUS;
  keymap[  kmapKEYPADPLUS          ]=0;
  keymap[  kmapKEYPADENTER         ]=KEY_ENTER;
  keymap[  kmapCTRLPRTSC           ]=0;
  keymap[  kmapSHIFTPRTSC          ]=0;
  keymap[  kmapKEYPADSLASH         ]=0;
#endif
}

/* Zeros the keyboard array, and installs event handlers. */
int
keybdrv_init (void)
{
  return 0;
}

/* Close the keyboard and tidy up */
void
keybdrv_close (void)
{
  
}







