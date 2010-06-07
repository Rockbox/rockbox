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
#ifndef PLUGIN_TEXT_VIEWER_ACTION_H
#define PLUGIN_TEXT_VIEWER_ACTION_H

#include "tv_menu.h"

/* horizontal scroll mode */
enum tv_horizontal_scroll_mode
{
    TV_HORIZONTAL_SCROLL_COLUMN, /* left/right one column */
    TV_HORIZONTAL_SCROLL_SCREEN, /* left/right one screen */
    TV_HORIZONTAL_SCROLL_PREFS,  /* left/right follows the settings */
};

/*vertical scroll mode */
enum tv_vertical_scroll_mode
{
    TV_VERTICAL_SCROLL_LINE,   /* up/down one line */
    TV_VERTICAL_SCROLL_PAGE,   /* up/down one page */
    TV_VERTICAL_SCROLL_PREFS,  /* up/down follows the settings */
};

/*
 * initialize modules
 *
 * [In] file
 *          read file name
 *
 * return
 *     true  initialize success
 *     false initialize failure
 */
bool tv_init(const unsigned char *file);

/*
 * finalize modules
 *
 * [In] parameter
 *          this argument does not use
 */
void tv_exit(void *parameter);

/* draw the current page */
void tv_draw(void);

/*
 * scroll up
 *
 * [In] mode
 *          scroll mode
 */
void tv_scroll_up(enum tv_vertical_scroll_mode mode);

/*
 * scroll down
 *
 * [In] mode
 *          scroll mode
 */
void tv_scroll_down(enum tv_vertical_scroll_mode mode);

/*
 * scroll left
 *
 * [In] mode
 *          scroll mode
 */
void tv_scroll_left(enum tv_horizontal_scroll_mode mode);

/*
 * scroll right
 *
 * [In] mode
 *          scroll mode
 */
void tv_scroll_right(enum tv_horizontal_scroll_mode mode);

/* jump to the top */
void tv_top(void);

/* jump to the bottom */
void tv_bottom(void);

/*
 * display menu
 *
 * return
 *     the following value returns
 *         TV_MENU_RESULT_EXIT_MENU     menu exit and continue this plugin
 *         TV_MENU_RESULT_EXIT_PLUGIN   request to exit this plugin
 *         TV_MENU_RESULT_ATTACHED_USB  connect USB cable
 */
enum tv_menu_result tv_menu(void);

/* add or remove the bookmark to the current position */
void tv_add_or_remove_bookmark(void);

#endif
