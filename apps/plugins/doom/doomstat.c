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
// Revision 1.1  2006/03/28 15:44:01  dave
// Patch #2969 - Doom!  Currently only working on the H300.
//
//
// DESCRIPTION:
// Put all global tate variables here.
//
//-----------------------------------------------------------------------------

#ifdef __GNUG__
#pragma implementation "doomstat.h"
#endif
#include "doomstat.h"


// Game Mode - identify IWAD as shareware, retail etc.
GameMode_t gamemode IDATA_ATTR= indetermined;
GameMission_t gamemission IDATA_ATTR = doom;

// Language.
Language_t   language = english;

// Set if homebrew PWAD stuff has been added.
boolean modifiedgame;

//-----------------------------------------------------------------------------

// CPhipps - compatibility vars
complevel_t compatibility_level=prboom_3_compatibility, default_compatibility_level=prboom_3_compatibility;

int comp[COMP_TOTAL] IBSS_ATTR, default_comp[COMP_TOTAL];    // killough 10/98

// v1.1-like pitched sounds
int pitched_sounds=0;        // killough

int     default_translucency; // config file says           // phares
boolean general_translucency IBSS_ATTR; // true if translucency is ok // phares

int demo_insurance, default_demo_insurance;        // killough 1/16/98

int  allow_pushers IDATA_ATTR = 1;      // MT_PUSH Things              // phares 3/10/98
int  default_allow_pushers;  // killough 3/1/98: make local to each game

int  variable_friction IDATA_ATTR = 1;      // ice & mud               // phares 3/10/98
int  default_variable_friction;  // killough 3/1/98: make local to each game

int  weapon_recoil IBSS_ATTR;              // weapon recoil                   // phares
int  default_weapon_recoil;      // killough 3/1/98: make local to each game

int player_bobbing IBSS_ATTR;  // whether player bobs or not          // phares 2/25/98
int default_player_bobbing;  // killough 3/1/98: make local to each game

int monsters_remember IBSS_ATTR;          // killough 3/1/98
int default_monsters_remember;

int monster_infighting IDATA_ATTR=1;       // killough 7/19/98: monster<=>monster attacks
int default_monster_infighting=1;

int monster_friction IDATA_ATTR=1;       // killough 10/98: monsters affected by friction
int default_monster_friction=1;

#ifdef DOGS
int dogs, default_dogs;         // killough 7/19/98: Marine's best friend :)
int dog_jumping, default_dog_jumping;   // killough 10/98
#endif

// killough 8/8/98: distance friends tend to move towards players
int distfriend = 128, default_distfriend = 128;

// killough 9/8/98: whether monsters are allowed to strafe or retreat
int monster_backing IBSS_ATTR, default_monster_backing;

// killough 9/9/98: whether monsters are able to avoid hazards (e.g. crushers)
int monster_avoid_hazards IBSS_ATTR, default_monster_avoid_hazards;

// killough 9/9/98: whether monsters help friends
int help_friends IBSS_ATTR, default_help_friends;

int flashing_hom;     // killough 10/98

int doom_weapon_toggles; // killough 10/98

int monkeys IBSS_ATTR, default_monkeys;

boolean nosfxparm=0;
boolean rockblock=1;
