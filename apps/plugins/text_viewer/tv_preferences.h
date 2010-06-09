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

enum scrollbar_mode {
    SB_OFF = 0,
    SB_ON,
};

struct tv_preferences {
    enum {
        WRAP = 0,
        CHOP,
    } word_mode;

    enum {
        NORMAL = 0,
        JOIN,
        EXPAND,
        REFLOW,
    } line_mode;

    enum {
        LEFT = 0,
        RIGHT,
    } alignment;

    enum codepages encoding;

    enum scrollbar_mode horizontal_scrollbar;
    enum scrollbar_mode vertical_scrollbar;

    enum {
        NO_OVERLAP = 0,
        OVERLAP,
    } page_mode;

    enum {
        HD_NONE = 0,
        HD_PATH,
        HD_SBAR,
        HD_BOTH,
    } header_mode;

    enum {
        FT_NONE = 0,
        FT_PAGE,
        FT_SBAR,
        FT_BOTH,
    } footer_mode;

    enum {
        SCREEN = 0,
        COLUMN,
    } horizontal_scroll_mode;

    enum {
        PAGE = 0,
        LINE,
    } vertical_scroll_mode;

    int autoscroll_speed;

    int windows;

    enum {
        NM_PAGE = 0,
        NM_TOP_BOTTOM,
    } narrow_mode;

    unsigned char font_name[MAX_PATH];
#ifdef HAVE_LCD_BITMAP
    struct font *font;
#endif
    unsigned char file_name[MAX_PATH];
};

/*
 * return the preferences
 *
 * return
 *     the pointer the preferences
 */
const struct tv_preferences *tv_get_preferences(void);

/*
 * change the preferences
 *
 * [In] new_prefs
 *          new preferences
 */
void tv_set_preferences(const struct tv_preferences *new_prefs);

/*
 * copy the preferences
 *
 * [Out] copy_prefs
 *          the preferences in copy destination
 */
void tv_copy_preferences(struct tv_preferences *copy_prefs);

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
void tv_add_preferences_change_listner(void (*listner)(const struct tv_preferences *oldp));

#endif
