// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// $Log$
// Revision 1.8  2006/04/14 21:07:55  kkurbjun
// Start of profiling support for doom.
//
// Revision 1.7  2006-04-04 11:16:44  dave
// Correct the #ifdef logic for timer_unregister() and add a comment describing why we need to surround the use of the user timer with #ifdefs
//
// Revision 1.6  2006-04-03 17:32:46  dave
// Clean up the (incorrect) #ifdef spaghetti for the timer.  We now have a user timer on the ipods, so we use it.
//
// Revision 1.5  2006-04-03 17:11:42  kkurbjun
// Finishing touches
//
// Revision 1.4  2006-04-03 17:00:56  dave
// Doom can't use the user timer at the same time as using the grayscale lib.
//
// Revision 1.3  2006-04-02 12:45:29  amiconn
// Use TIMER_FREQ for timers in plugins. Fixes timer speed on iPod.
//
// Revision 1.2  2006-04-02 01:52:44  kkurbjun
// Update adds prboom's high resolution support, also makes the scaling for platforms w/ resolution less then 320x200 much nicer.  IDoom's lookup table code has been removed.  Also fixed a pallete bug.  Some graphic errors are present in menu and status bar.  Also updates some headers and output formatting.
//
// Revision 1.1  2006-03-28 15:44:01  dave
// Patch #2969 - Doom!  Currently only working on the H300.
//
//
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "m_misc.h"
#include "i_video.h"
#include "i_sound.h"

#include "d_net.h"
#include "g_game.h"
#include "z_zone.h"

#ifdef __GNUG__
#pragma implementation "i_system.h"
#endif
#include "i_system.h"

#include "rockmacros.h"

//
// I_GetTime
// returns time in 1/70th second tics
//

/* NOTE: 

   The user timer is used to generate a 70Hz tick for Doom.  But it
   is unavailable for the grayscale targets (it's used by the grayscale
   lib) and is not implemented in the simulator - so we have to
   approximate it using current_tick.
*/
#if defined(HAVE_LCD_COLOR) && !defined(SIMULATOR) && !defined(RB_PROFILE)
volatile unsigned int doomtimer=0;

void doomtime(void)
{
   doomtimer++;
}
#endif

int  I_GetTime (void)
{
#if defined(HAVE_LCD_COLOR) && !defined(SIMULATOR) && !defined(RB_PROFILE)
   return doomtimer;
#else
#if HZ==100
   return ((7*(*rb->current_tick))/20);
#else
   #error FIX - I assumed HZ was 100
#endif
#endif
}

//
// I_Init
//

// I was looking into this and comparing the speed versus Prboom
// Turns out they are running the game much slower then I thought the game was
// played.  This explains why run was unusable other then through straight stretches
// The game is much slower now (in terms of game speed).
void I_Init (void)
{
#if defined(HAVE_LCD_COLOR) && !defined(SIMULATOR) && !defined(RB_PROFILE)
   rb->timer_register(1, NULL, TIMER_FREQ/TICRATE, 1, doomtime);
#endif
   I_InitSound();
}

//
// I_Quit
//
extern boolean doomexit;
void I_Quit (void)
{
   I_ShutdownSound();
   I_ShutdownMusic();
   I_ShutdownGraphics();
#if defined(HAVE_LCD_COLOR) && !defined(SIMULATOR) && !defined(RB_PROFILE)
   rb->timer_unregister();
#endif
   doomexit=1;
}

void I_WaitVBL(int count)
{
   rb->sleep(count);
}

//
// I_Error
//
extern boolean demorecording;

void I_Error (char *error, ...)
{
   char p_buf[50];
   va_list ap;

   va_start(ap, error);
   vsnprintf(p_buf,sizeof(p_buf), error, ap);
   va_end(ap);

   printf("%s\n",p_buf);

   // Shutdown. Here might be other errors.
   if (demorecording)
      G_CheckDemoStatus();

   I_Quit();
   rb->sleep(HZ*2);
}
