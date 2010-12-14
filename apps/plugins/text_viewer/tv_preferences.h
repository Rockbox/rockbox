/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Gilles Roux
 *               2003 Garrett Derner
 *               2010 Yoshihisa Uchida
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
#ifndef PLUGIN_TEXT_VIEWER_PREFERENCES_H
#define PLUGIN_TEXT_VIEWER_PREFERENCES_H

enum {
    TV_CALLBACK_OK,
    TV_CALLBACK_STOP,
    TV_CALLBACK_ERROR,
};

/* word_mode */
enum {
    WM_WRAP = 0,
    WM_CHOP,
};

/* line_mode */
enum {
    LM_NORMAL = 0,
    LM_JOIN,
    LM_EXPAND,
    LM_REFLOW,
};

/* alignment */
enum {
    AL_LEFT = 0,
    AL_RIGHT,
};

/* horizontal_scroll_mode */
enum {
    HS_SCREEN = 0,
    HS_COLUMN,
};

/* vertical_scroll_mode */
enum {
    VS_PAGE = 0,
    VS_LINE,
};

/* narrow_mode */
enum {
    NM_PAGE = 0,
    NM_TOP_BOTTOM,
};

struct tv_preferences {
    unsigned word_mode;
    unsigned line_mode;
    unsigned alignment;

    unsigned encoding;

    bool horizontal_scrollbar;
    bool vertical_scrollbar;

    bool overlap_page_mode;
    bool header_mode;
    bool footer_mode;
    unsigned horizontal_scroll_mode;
    unsigned vertical_scroll_mode;

    int autoscroll_speed;

    int windows;

    unsigned narrow_mode;

    unsigned indent_spaces;

    bool statusbar;

#ifdef HAVE_LCD_BITMAP
    unsigned char font_name[MAX_PATH];
    struct font *font;
#endif
    unsigned char file_name[MAX_PATH];
};

/*
 *     global pointer to the preferences (read-only)
 */
extern const struct tv_preferences * const preferences;
extern bool preferences_changed;

/*
 * change the preferences
 *
 * [In] new_prefs
 *          new preferences
 *
 * return
 *     true  success
 *     false error
 */
bool tv_set_preferences(const struct tv_preferences *new_prefs);

/*
 * copy the preferences
 *
 * [Out] copy_prefs
 *          the preferences in copy destination
 */
void tv_copy_preferences(struct tv_preferences *copy_prefs);

/*
 * compare the preferences structs (binary)
 *
 * return
 *     true  differs
 *     false identical
 */
bool tv_compare_preferences(struct tv_preferences *copy_prefs);

/*
 * set the default settings
 *
 * [Out] p
 *          the preferences which store the default settings
 */
void tv_set_default_preferences(struct tv_preferences *p);

/*
 * register the function to be executed when the current preferences is changed
 *
 * [In] listner
 *          the function to be executed when the current preferences is changed
 */
void tv_add_preferences_change_listner(int (*listner)(const struct tv_preferences *oldp));

#endif
