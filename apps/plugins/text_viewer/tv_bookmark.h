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
#ifndef PLUGIN_TEXT_VIEWER_BOOKMARK_H
#define PLUGIN_TEXT_VIEWER_BOOKMARK_H

#include "tv_screen_pos.h"

/* stuff for the bookmarking */

/* Maximum amount of register possible bookmarks */
#define TV_MAX_BOOKMARKS 16
#define SERIALIZE_BOOKMARK_SIZE 8

/*
 * initialize the bookmark module
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
bool tv_init_bookmark(unsigned char **buf, size_t *size);

/* finalize the bookmark module */
void tv_finalize_bookmark(void);

/*
 * get the positions which registered bookmarks
 *
 * [Out] pos_array
 *          the array which store positions of all bookmarks
 *
 * return
 *     bookmark count
 */
int tv_get_bookmark_positions(struct tv_screen_pos *pos_array);

/*
 * the function that a bookmark add when there is not a bookmark in the given position
 * or the bookmark remove when there exist a bookmark in the given position.
 */
void tv_toggle_bookmark(void);

/*
 * The menu that can select registered bookmarks is displayed, one is selected from
 * among them, and moves to the page which selected bookmarks.
 */
void tv_select_bookmark(void);

/* creates system bookmark */
void tv_create_system_bookmark(void);

/*
 * serialize the bookmark array
 *
 * [In] fd
 *        the file descripter which is stored the result.
 *
 * Return
 *     the size of the result
 */
int tv_serialize_bookmarks(unsigned char *buf);

/*
 * deserialize the bookmark array
 *
 * [In] fd
 *       the file descripter which is stored the serialization of the bookmark array.
 *
 * Return
 *     true  success
 *     false failure
 */
bool tv_deserialize_bookmarks(int fd);

#endif
