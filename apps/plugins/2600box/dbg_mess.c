/*****************************************************************************

   This file is part of Virtual 2600, the Atari 2600 Emulator
   ==========================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: dbg_mess.c,v 1.3 1997/11/22 14:27:47 ahornby Exp $
******************************************************************************/

/* Print debugging messages of the appropriate urgency */

#include "rbconfig.h"
#include <stdio.h>
#include <stdarg.h>

#include "types.h"
#include "options.h"

/*
  Levels with meanings
  --------------------
  
  None,  (No stdout)
  User level,  (User level messages)
  Normal programmers debug, (Programmer debug messages)
  Lots programmers debug,   (Every frame)
  Obscene programmers debug (Every line or more)
  
  */

enum dbg_level {DBG_NONE=0, DBG_USER=1, DBG_NORMAL=2, DBG_LOTS=3, DBG_OBSCENE=4};

void 
dbg_message(int level, const char *format, ...)
{
#if Verbose
  
  if( level <= base_opts.dbg_level )
    {
      va_list ap;
      
      va_start(ap, format);
      
      vfprintf(stderr, format, ap );
    }
#endif
}


