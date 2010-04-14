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
#ifndef PLUGIN_TEXT_VIEWER_SCREEN_H
#define PLUGIN_TEXT_VIEWER_SCREEN_H

/* scroll mode */
enum
{
    VIEWER_SCROLL_LINE,   /* up/down one line */
    VIEWER_SCROLL_PAGE,   /* up/down one page */
    VIEWER_SCROLL_COLUMN, /* left/right one column */
    VIEWER_SCROLL_SCREEN, /* left/right one screen */
    VIEWER_SCROLL_PREFS,  /*up/down/left/right follows the settings. */
};

void viewer_init_screen(void);
void viewer_finalize_screen(void);

void viewer_draw(void);

void viewer_top(void);
void viewer_bottom(void);

void viewer_scroll_up(int mode);
void viewer_scroll_down(int mode);
void viewer_scroll_left(int mode);
void viewer_scroll_right(int mode);

void change_font(unsigned char *font);

#endif
