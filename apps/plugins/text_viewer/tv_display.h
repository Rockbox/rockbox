/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Yoshihisa Uchida
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
#ifndef PLUGIN_TEXT_VIEWER_DISPLAY_H
#define PLUGIN_TEXT_VIEWER_DISPLAY_H

#include "tv_screen_pos.h"

/* stuff for the screen access */

/*
 * initialize the display module
 *
 * [In/Out] buf
 *          the start pointer of the buffer
 *
 * [In/Out] size
 *          buffer size
 *
 * return
 *     true  initialize success
 *     false initialize failure
 */
bool tv_init_display(unsigned char **buf, size_t *size);

/* finalize the display module */
void tv_finalize_display(void);

/*
 * draw the text
 *
 * [In] row
 *          the row that displays the text
 *
 * [In] text
 *          text
 *
 * [In] offset
 *          display the text that is since offset columns
 */
void tv_draw_text(int row, const unsigned char *text, int offset);

/*
 * show bookmark
 *
 * [In] rows
 *          the array of row where the bookmark
 *
 * [In] count
 *          want to show bookmark count
 */
void tv_show_bookmarks(const int *rows, int count);

/*
 * update extra parts (header, footer, scrollbar, etc.)
 *
 * [In] window
 *          current window
 *
 * [In] col
 *          current column
 *
 * [In] pos
 *          current screen position (file position, page, line)
 *
 * [In] size
 *          the size of text which is displayed.
 */
void tv_update_extra(int window, int col, const struct tv_screen_pos *pos, int size);

/*
 * initialize the scrollbar
 *
 * [In] total
 *          total text size
 *
 * [In] show_scrollbar
 *          true:  show the vertical scrollbar
 *          false: does not show the vertical scrollbar
 */
void tv_init_scrollbar(off_t total, bool show_scrollbar);

/* start the display processing  */
void tv_start_display(void);

/* end the display processing */
void tv_end_display(void);

/*change color scheme*/
void tv_night_mode(void);


/* layout functions */

/*
 * set the layout
 *
 * [In] show_scrollbar
 *          true:  show the vertical scrollbar
 *          false: does not show the vertical scrollbar
 */
void tv_set_layout(bool show_scrollbar);

/*
 * get the draw area info
 *
 * [Out] width
 *          width of the draw area
 *
 * [Out] cols
 *          column count of the draw area
 *
 * [Out] width
 *          row count of the draw area
 */
void tv_get_drawarea_info(int *width, int *cols, int *rows);

/*
 * whether exist scrollbar
 *
 * return
 *     true  exist scrollbar
 *     false does not exist scrollbar
 */
bool tv_exist_scrollbar(void);

#endif
