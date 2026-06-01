/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

#include "menu.h"

enum onplay_custom_action {
    ONPLAY_NO_CUSTOMACTION,
    ONPLAY_CUSTOMACTION_SHUFFLE_SONGS,
    ONPLAY_CUSTOMACTION_FIRSTLETTER,
};

enum onplay_return_code {
    ONPLAY_MAINMENU = -1,
    ONPLAY_OK = 0,
    ONPLAY_RELOAD_DIR,
    ONPLAY_REVEAL_FILE,
    ONPLAY_START_PLAY,
    ONPLAY_PLAYLIST,
    ONPLAY_PLUGIN,
    ONPLAY_FUNC_RETURN, /* for use in hotkey_assignment only */
};

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
    HOTKEY_ALBUMART,
    HOTKEY_SHOW_IN_FILES,
    HOTKEY_CONTEXT_MENU = 0x3E, /* Last item shows / executes above actions in a menu */
    /* Note no more than 62 items */
};
enum hotkey_flags {
    HOTKEY_FLAG_NONE = 0x0,
    HOTKEY_FLAG_WPS = 0x1,
    HOTKEY_FLAG_TREE = 0x2,
    HOTKEY_FLAG_NOSBS = 0x4,
};

struct hotkey_assignment {
    int lang_id;            /* Language ID */
    struct menu_func_param func;  /* Function to run if this entry is selected */
    int8_t  return_code;    /* What to return after the function is run. */
    uint8_t flags;         /* Flags what context, display options */
                           /* (Pick ONPLAY_FUNC_RETURN to use function's return value) */
    uint8_t action;        /* hotkey_action */
    uint8_t icon;          /* context menu icon */
};

/* hotkey & wps context menu */
#define HK_CTX_ITEMS (5) /* 6 x 5 = 30 bits */
#define HK_CTX_MASK  (0x3F)
#define HK_CTX_BITS  (6) /* 6 bits, enough for 62 hotkey actions */
#define HK_CTX_SET(item, hotkey) ((hotkey & HK_CTX_MASK) << (item * HK_CTX_BITS))
#define HK_CTX_GET(item, hotkey) ((hotkey >> (item * HK_CTX_BITS)) & HK_CTX_MASK)

const struct hotkey_assignment *get_hotkey(int action);
int hotkey_run_menu(intptr_t flag, bool execute, int current_action);

int onplay(char* file, int attr, int from_context, bool hotkey, int customaction);
int get_onplay_context(void);

/* needed for the playlist viewer.. eventually clean this up */
void onplay_show_playlist_cat_menu(const char* track_name, int attr,
                                   void (*add_to_pl_cb));
void onplay_show_playlist_menu(const char* path, int attr, void (*playlist_insert_cb));

int wps_context_menu_do_setting(void *param);
#ifdef HAVE_HOTKEY
int tree_context_menu_do_setting(void *param);
#endif
void wps_context_menu_set_default(void* setting, void* defaultval);
char* wps_context_menu_write_to_cfg(void* setting, char*buf, int buf_len);
void wps_context_menu_load_from_cfg(void* setting, char *value);
bool wps_context_menu_is_changed(void* setting, void* defaultval);

#endif
