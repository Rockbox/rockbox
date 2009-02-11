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

#include "game.h"
#include "types.h"
#include "board.h"
#include "goban.h"
#include "sgf.h"
#include "sgf_output.h"
#include "sgf_parse.h"
#include "sgf_storage.h"
#include "display.h"

static void pre_game_setup (void);

char save_file[SAVE_FILE_LENGTH];
bool game_dirty = false;
bool autosave_dirty = false;

int move_num = 0;

unsigned char current_player = BLACK;

struct header_t header;

void
set_game_modified (void)
{
    game_dirty = true;
    autosave_dirty = true;
}

bool
load_game (const char *filename)
{
    rb->memset (&header, 0, sizeof (header));

    if (rb->strlen (filename) + 1 > SAVE_FILE_LENGTH)
    {
        DEBUGF ("file name too long\n");
        return false;
    }

    if (!rb->file_exists (filename))
    {
        DEBUGF ("file doesn't exist!\n");
        return false;
    }

    pre_game_setup ();

    rb->strcpy (save_file, filename);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost (true);
#endif

    rb->splash (0, "Loading...");

    bool parse_failed = false;
    if (!parse_sgf (save_file))
    {
        rb->splash (3 * HZ, "Unable to parse SGF file.  Will overwrite.");
        parse_failed = true;
    }

    game_dirty = false;
    autosave_dirty = false;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost (false);
#endif

    if (header.handicap >= 2)
    {
        current_player = WHITE;
    }

    post_game_setup_sgf ();

    if (!parse_failed)
    {
        draw_screen_display();
        if (rb->strcmp(filename, DEFAULT_SAVE))
        {
            metadata_summary();
        }
    }

    return true;
}


bool
save_game (const char *filename)
{
    if (rb->strlen (filename) + 1 > SAVE_FILE_LENGTH)
    {
        return false;
    }

    /* We only have to do something if the game is dirty, or we're being
       asked to save to a different location than we loaded from.

       If the game isn't dirty and we're being asked to save to default,
       we also don't have to do anything.*/
    if (!game_dirty &&
        (rb->strcmp (filename, save_file) == 0 ||
         rb->strcmp (filename, DEFAULT_SAVE) == 0))
    {
        return true;
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost (true);
#endif

    rb->splash (0, "Saving...");

    if (output_sgf (filename))
    {
        /* saving only "cleans" the game if it's not a save to default,
         * or if our save_file is actually default
         *
         * (so autosaves won't prevent legitimate saves to a Save As or
         * loaded file)
         */
        if (rb->strcmp (filename, DEFAULT_SAVE) ||
            rb->strcmp (save_file, DEFAULT_SAVE) == 0)
        {
            game_dirty = false;
        }

        /* but saving anywhere means that autosave isn't dirty */
        autosave_dirty = false;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost (false);
#endif
        /* The save succeeded.  Now, if we saved to an actual file (not to the
         * DEFAULT_SAVE), then we should delete the DEFAULT_SAVE file because
         * the changes stored in it are no longer unsaved.
         */
        if (rb->strcmp (filename, DEFAULT_SAVE))
        {
            rb->remove(DEFAULT_SAVE);
        }

        return true;
    }
    else
    {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost (false);
#endif
        return false;
    }
}


static void
pre_game_setup (void)
{
    rb->memset (&header, 0, sizeof (header));

    clear_caches_sgf ();
    free_tree_sgf ();

    set_size_board (MAX_BOARD_SIZE, MAX_BOARD_SIZE);

    clear_board ();

    game_dirty = true;
    move_num = 0;

    play_mode = MODE_PLAY;

    rb->strcpy (save_file, DEFAULT_SAVE);

    header_marked = false;
}


bool
setup_game (int width, int height, int handicap, int komi)
{
    pre_game_setup ();

    if (!set_size_board (width, height))
    {
        return false;
    }

    clear_board ();

    /* place handicap */
    if (handicap >= 2)
    {
        current_player = WHITE;
    }
    else if (handicap < 0)
    {
        return false;
    }
    else
    {
        current_player = BLACK;
    }

    header.handicap = handicap;
    setup_handicap_sgf ();

    header.komi = komi;

    post_game_setup_sgf ();

    return true;
}
