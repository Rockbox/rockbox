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
//
// DESCRIPTION:
// System interface, sound.
//
//-----------------------------------------------------------------------------

#ifndef __I_SOUND__
#define __I_SOUND__

#include "doomdef.h"

// UNIX hack, to be removed.
#ifdef SNDSERV
extern FILE* sndserver;
extern char* sndserver_filename;
#endif

#include "doomstat.h"
#include "sounds.h"



// Init at program start...
void I_InitSound(void);

// ... update sound buffer and audio device at runtime...
void I_UpdateSound(void);
void I_SubmitSound(void);

// ... shut down and relase at program termination.
void I_ShutdownSound(void);


//
//  SFX I/O
//

// Initialize channels?
void I_SetChannels(void);

// Get raw data lump index for sound descriptor.
int I_GetSfxLumpNum (sfxinfo_t* sfxinfo );


// Starts a sound in a particular sound channel.
int
I_StartSound
( int  id,
  int  vol,
   int cnum,
  int  sep,
  int  pitch,
  int  priority );


// Stops a sound channel.
void I_StopSound(int handle);

// Called by S_*() functions
//  to see if a channel is still playing.
// Returns 0 if no longer playing, 1 if playing.
int I_SoundIsPlaying(int handle);

// Updates the volume, separation,
//  and pitch of a sound channel.
void
I_UpdateSoundParams
( int  handle,
  int  vol,
  int  sep,
  int  pitch );


//
//  MUSIC I/O
//
void I_InitMusic(void);
void I_ShutdownMusic(void);
// Volume.
void I_SetMusicVolume(int volume);
// PAUSE game handling.
void I_PauseSong(int handle);
void I_ResumeSong(int handle);
// Registers a song handle to song data.
int I_RegisterSong(const void *data);
// Called by anything that wishes to start music.
//  plays a song, and when the song is done,
//  starts playing it again in an endless loop.
// Horrible thing to do, considering.
void
I_PlaySong
( int  handle,
  int  looping );
// Stops a song over 3 seconds.
void I_StopSong(int handle);
// See above (register), then think backwards
void I_UnRegisterSong(int handle);



#endif
//-----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2006/03/28 15:44:01  dave
// Patch #2969 - Doom!  Currently only working on the H300.
//
//
//-----------------------------------------------------------------------------
