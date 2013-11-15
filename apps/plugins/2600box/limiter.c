

/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for Details.
   
   $Id: limiter.c,v 1.8 1997/11/22 14:27:47 ahornby Exp $
******************************************************************************/

/*
   Attempt at a speed limiter.
   (You'll have a fast computer to need it though!) 
 */

#include "rbconfig.h"
#include "options.h"
#include "types.h"
#include "vmachine.h"

/* Global variables. A C++ implementation would hide these */

/*
struct timeval limiter_lastClk;
struct timeval limiter_newClk;
long limiter_fTime = 0;
long limiter_last = 0;
long limiter_new = 0;
long limiter_endSleep = 0;
int limiter_frameRate = 0;
int limiter_vRate = 0;
*/

/* Get the currently set frame rate */
/* returns: currently set rate */
int
limiter_getFrameRate (void)
{
	/*
  if (limiter_vRate < 0)
    limiter_vRate = tv_hertz;
  return limiter_vRate;*/
	return 0;
}

/* Set the desired frame rate */
/* f: frame rate in frames per second */
void
limiter_setFrameRate (int f)
{
/*  limiter_frameRate = f;
  limiter_fTime = 1000000 / f;*/
}

/* Initialise the frame limiter */
void
limiter_init (void)
{/*
  gettimeofday (&limiter_lastClk, NULL);
  gettimeofday (&limiter_newClk, NULL);
  limiter_endSleep = limiter_lastClk.tv_usec;
  */
}

/* Perform the speed limiting syncronisation */
/* returns: number of microseconds since last call */
long
limiter_sync (void)
{
#if 0
  long delta = 0, wait = 0;

  /* Get the current time */
  gettimeofday (&limiter_newClk, NULL);

  if (limiter_newClk.tv_usec < limiter_endSleep)
    limiter_endSleep -= 1000000;

  /* how long to wait. Min is 0 usec, max is ftime. */
  wait = (limiter_fTime - limiter_newClk.tv_usec + limiter_endSleep);
  if (wait < 0)
    wait = 0;
  else if (wait > limiter_fTime)
    wait = limiter_fTime;

  /* Do the waiting */
#if HAVE_SELECT 
  if(base_opts.limit && wait>10)
    {
      struct timeval tv_wait={0, wait};
      select(0,0,0,0,&tv_wait);
    }
#endif

  limiter_endSleep = limiter_newClk.tv_usec + wait;
  delta = limiter_newClk.tv_usec - limiter_lastClk.tv_usec;

  gettimeofday (&limiter_lastClk, NULL);

  limiter_vRate = 1000000 / delta;
  return delta;
#endif 
  return 0;
}
