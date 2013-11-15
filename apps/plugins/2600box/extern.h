/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: extern.h,v 1.3 1996/04/01 14:51:50 alex Exp $
******************************************************************************/

/* 
   Defines the external variables needed by most hardware access code.
   */

#ifndef VCSEXTERN_H
#define VCSEXTERN_H


#include "types.h"	/* for BYTE, ADDRESS, etc. types and structures */

extern int   clength[];

/* Processor registers */
extern BYTE  accumulator;
extern BYTE  x_register;
extern BYTE  y_register;
extern BYTE  stack_pointer;
extern BYTE  status_register;
extern ADDRESS program_counter;
extern CLOCK clk;

/* Processor flags */
extern int   zero_flag;
extern int   sign_flag;
extern int   overflow_flag;
extern int   break_flag;
extern int   decimal_flag;
extern int   interrupt_flag;
extern int   carry_flag;

/* Debugging */
extern int   hexflg;
extern int   verflg;
extern int   traceflg;
extern int   debugflg;
extern int   runflg;

#endif






