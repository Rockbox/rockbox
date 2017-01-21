//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
aint32_t with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

//****************************************************************************
//
// sounds.h
//
//****************************************************************************
#include "audiolib/fx_man.h"

#ifndef _sounds_public_
#define _sounds_public_

#define HIRESMUSICPATH "tunes"

extern int32_t FXDevice;
extern int32_t MusicDevice;
extern int32_t FXVolume;
extern int32_t MusicVolume;
extern fx_blaster_config BlasterConfig;
extern int32_t NumVoices;
extern int32_t NumChannels;
extern int32_t NumBits;
extern int32_t MixRate;
extern int32_t MidiPort;
extern int32_t ReverseStereo;

void SoundStartup( void );
void SoundShutdown( void );
void MusicStartup( void );
void MusicShutdown( void );

/* sounds.c */
void clearsoundlocks(void);

/* dunno where this came from; I added it. --ryan. */
void testcallback(uint32_t num);

#endif
