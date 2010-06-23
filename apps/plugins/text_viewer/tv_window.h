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
#ifndef PLUGIN_TEXT_VIEWER_WINDOW_H
#define PLUGIN_TEXT_VIEWER_WINDOW_H

/*
 * initialize the window module
 *
 * [In/Out] buf
 *          the start pointer of the buffer
 *
 * [In/Out] size
 *          enabled buffer size
 *
 * return
 *     true  initialize success
 *     false initialize failure
 */
bool tv_init_window(unsigned char **buf, size_t *bufsize);

/* finalize the window module */
void tv_finalize_window(void);

/* draw the display */
void tv_draw_window(void);

/* traverse all lines of the current page */
bool tv_traverse_lines(void);

/*
 * move to the window
 *
 * new position
 *     new window: the current window + window_delta
 *     new column: the current column + column_delta
 *
 * [In] window_delta
 *          variation of the window
 *
 * [In] column_delta
 *          variation of the column
 *
 */
void tv_move_window(int window_delta, int column_delta);

#endif
