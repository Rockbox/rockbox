/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================

   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.

   See the file COPYING for details.

   $Id: options.c,v 1.7 1996/11/24 16:55:40 ahornby Exp $
******************************************************************************/

/* Command Line Option Parser */
#include "rbconfig.h"
#include <stdio.h>
#include <stdlib.h>

//#include "version.h"
/* *INDENT-OFF* */
/* Options common to all ports of x2600 */
struct BaseOptions {
  int rr;
  int tvtype;
  int lcon;
  int rcon;
  int bank;
  int magstep;
  char filename[80];
  int sound;
  int swap;
  int realjoy;
  int limit;
  int mousey;
  int mitshm;
  int dbg_level;
} base_opts={1,0,0,0,0,1,"",0,0,0,0,0,1,0};

#if 0
static void
copyright(void)
{

}
#endif

#if 0
static void
base_usage(void)
{

}

static void
usage(void)
{
    base_usage();
}
#endif

#if 0
int
parse_options(char *file)
{
    rb->strcpy ( &base_opts.filename[0] , file );
    return 0;
}
#endif





