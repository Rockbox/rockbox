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

#ifndef GOBAN_SGF_STORAGE_H
#define GOBAN_SGF_STORAGE_H

#include "types.h"

/* Must be called (and return true) before using anything in the SGF
   subsystem returns false on failure */
bool setup_sgf (void);

/* Do cleanup, call before exiting the plugin.  You must not use any SGF
   subsystem functions after calling this */
void cleanup_sgf (void);

/* Get ready for a new game (either loaded or blank) */
void clear_caches_sgf (void);

/* Clear the SGF tree and get it ready for a new game (loaded or blank) */
void free_tree_sgf (void);

/* Returns true if the Rockbox audio buffer has been stolen */
bool audio_stolen_sgf (void);

/* Returns a handle to a struct storage_t (NOT a pointer) < 0 handles are
   invalid */
int alloc_storage_sgf (void);

/* Free one storage location */
void free_storage_sgf (int handle);

/* Get a pointer to a node or property which corresponds to the given
 * storage handle
 */
struct node_t *get_node (int handle);
struct prop_t *get_prop (int handle);

#endif
