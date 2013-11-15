
/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: no_ui.c,v 1.3 1996/11/24 16:55:40 ahornby Exp $
******************************************************************************/

/*
   User Interface (UI) prototypes for those without FWF etc.
 */
#include "rbconfig.h"

#if HAVE_LIBXT
#include <X11/Intrinsic.h>
#else 
typedef int Widget;
#endif

void 
fancy_widgets (Widget parent)
{
}

int
fselLoad (void)
{
  return 0;
}
