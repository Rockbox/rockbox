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
#ifndef PLUGIN_TEXT_VIEWER_PAGER_H
#define PLUGIN_TEXT_VIEWER_PAGER_H

#include "tv_screen_pos.h"

/* stuff for the paging */

/*
 * initialize the pager module
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
bool tv_init_pager(unsigned char **buf, size_t *bufsize);

/* finalize the pager module */
void tv_finalize_pager(void);

/* reset the stored line positions */
void tv_reset_line_positions(void);

/*
 * move to the next line
 *
 * [In] size
 *         the current line size
 */
void tv_move_next_line(int size);

/*
 * return the distance from the top of the current page
 *
 * [In] offset
 *         the difference between the current line
 *
 * return
 *        difference between the current line + offset from the top of the current page
 */
int tv_get_line_positions(int offset);

/* change the new page */
void tv_new_page(void);

/*
 * convert the given file position to the nearest the position (page, line)
 *
 * [In] fpos
 *         the file position which want to convert
 *
 * [Out] pos
 *         result
 */
void tv_convert_fpos(off_t fpos, struct tv_screen_pos *pos);

/*
 * move to the given page and line
 *
 * [In] page_offset
 *          page offset
 *
 * [In] line_offset
 *          line offset
 *
 * [In] whence
 *          SEEK_CUR  seek to the current page + offset page, the current line + offset line.
 *          SEEK_SET  seek to the offset page, offset line.
 *          SEEK_END  seek to the reading last page + offset page,
 *                            the last line of the reading last page + offset line.
 */
void tv_move_screen(int page_offset, int line_offset, int whence);

#endif
