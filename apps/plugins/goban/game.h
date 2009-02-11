/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007-2009 Joshua Simmons <mud at majidejima dot com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef GAME_GOBAN_H
#define GAME_GOBAN_H

#include "types.h"

/* Call whenever anything saveable in the SGF file might have changed */
void set_game_modified (void);

/* Setup a new game, clearing anything currently in the SGF tree Returns
   false on failure, in which case the old data is NOT guaranteed to be
   available anymore */
bool setup_game (int width, int height, int handicap, int komi);

/* Load a game from a file, clearing anything currently in the SGF tree
   Returns false on failure, in which case the old data is NOT guaranteed
   to be available anymore */
bool load_game (const char *filename);

/* Save the data in the SGF tree to a file. Returns false on failure */
bool save_game (const char *filename);

/* The number of the current move (starts at 0, first move is 1) */
extern int move_num;

/* The color of the current_player, either BLACK or WHITE */
extern unsigned char current_player;
/* Where should we save to if explicitly saved? */
extern char save_file[];
/* True if there are unsaved changes in the file */
extern bool game_dirty;

/* True if there are changes that have been neither autosaved nor saved */
extern bool autosave_dirty;

/* The game metadata */
extern struct header_t header;

#endif
