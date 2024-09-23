/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
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
#ifndef _ONPLAY_H_
#define _ONPLAY_H_

#ifdef HAVE_HOTKEY
#include "menu.h"
#endif

enum {
    ONPLAY_NO_CUSTOMACTION,
    ONPLAY_CUSTOMACTION_SHUFFLE_SONGS,
    ONPLAY_CUSTOMACTION_FIRSTLETTER,
};

int onplay(char* file, int attr, int from_context, bool hotkey, int customaction);
int get_onplay_context(void);

enum {
    ONPLAY_MAINMENU = -1,
    ONPLAY_OK = 0,
    ONPLAY_RELOAD_DIR,
    ONPLAY_START_PLAY,
    ONPLAY_PLAYLIST,
    ONPLAY_PLUGIN,
#ifdef HAVE_HOTKEY
    ONPLAY_FUNC_RETURN, /* for use in hotkey_assignment only */
#endif
};

#ifdef HAVE_HOTKEY

enum hotkey_action {
    HOTKEY_OFF = 0,
    HOTKEY_VIEW_PLAYLIST,
    HOTKEY_PROPERTIES,
    HOTKEY_PICTUREFLOW,
    HOTKEY_SHOW_TRACK_INFO,
    HOTKEY_PITCHSCREEN,
    HOTKEY_OPEN_WITH,
    HOTKEY_DELETE,
    HOTKEY_BOOKMARK,
    HOTKEY_PLUGIN,
    HOTKEY_INSERT,
    HOTKEY_INSERT_SHUFFLED,
    HOTKEY_BOOKMARK_LIST,
};
enum hotkey_flags {
    HOTKEY_FLAG_NONE = 0x0,
    HOTKEY_FLAG_WPS = 0x1,
    HOTKEY_FLAG_TREE = 0x2,
    HOTKEY_FLAG_NOSBS = 0x4,
};

struct hotkey_assignment {
    int action;             /* hotkey_action */
    int lang_id;            /* Language ID */
    struct menu_func_param func;  /* Function to run if this entry is selected */
    int16_t return_code;    /* What to return after the function is run. */
    uint16_t flags;         /* Flags what context, display options */
};                          /* (Pick ONPLAY_FUNC_RETURN to use function's return value) */

const struct hotkey_assignment *get_hotkey(int action);
#endif

/* needed for the playlist viewer.. eventually clean this up */
void onplay_show_playlist_cat_menu(const char* track_name, int attr,
                                   void (*add_to_pl_cb));
void onplay_show_playlist_menu(const char* path, int attr, void (*playlist_insert_cb));

#endif
