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
#ifndef PLUGIN_TEXT_VIEWER_PREFERENCE_H
#define PLUGIN_TEXT_VIEWER_PREFERENCE_H

struct viewer_preferences {
    enum {
        WRAP=0,
        CHOP,
    } word_mode;

    enum {
        NORMAL=0,
        JOIN,
        EXPAND,
        REFLOW, /* won't be set on charcell LCD, must be last */
    } line_mode;

    enum {
        NARROW=0,
        WIDE,
    } view_mode;

    enum codepages encoding;

    enum {
        SB_OFF=0,
        SB_ON,
    } scrollbar_mode;
    bool need_scrollbar;

    enum {
        NO_OVERLAP=0,
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
        PAGE=0,
        LINE,
    } scroll_mode;

    int autoscroll_speed;

    unsigned char font[MAX_PATH];
};

struct viewer_preferences *viewer_get_preference(void);
void viewer_load_settings(void);
bool viewer_save_settings(void);

#endif
