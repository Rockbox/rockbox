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
#ifndef PLUGIN_TEXT_VIEWER_SCREEN_POS_H
#define PLUGIN_TEXT_VIEWER_SCREEN_POS_H

struct tv_screen_pos {
    off_t file_pos;
    int page;
    int line;
};

/*
 * return the current position
 *
 * return
 *     the pointer which is stored the current position
 */
const struct tv_screen_pos *tv_get_screen_pos(void);

/*
 * set the current position
 *
 * [In] pos
 *         the pointer which is stored the current position
 */
void tv_set_screen_pos(const struct tv_screen_pos *pos);

/*
 * the current position set to the given structure
 *
 * [Out] pos
 *         the pointer in order to store the current position
 */
void tv_copy_screen_pos(struct tv_screen_pos *pos);

#endif
