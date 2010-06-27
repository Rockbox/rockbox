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

#include "plugin.h"
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


/* layout parts accessing functions */

#ifdef HAVE_LCD_BITMAP

/* show headaer */
void tv_show_header(void);

/*
 * show footer
 *
 * [In] pos
 *       the current position
 */
void tv_show_footer(const struct tv_screen_pos *pos);

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

/*
 * show horizontal/vertical scrollbar
 *
 * [In] window
 *          the current window
 *
 * [In] col
 *          the current column
 *
 * [In] cur_pos
 *          the current text position
 *
 * [In] size
 *          the size of text in displayed.
 */
void tv_show_scrollbar(int window, int col, off_t cur_pos, int size);

#endif

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

/* common display functons */

/* start the display processing  */
void tv_start_display(void);

/* end the display processing */
void tv_end_display(void);

/*update the display  */
void tv_update_display(void);

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

#endif
