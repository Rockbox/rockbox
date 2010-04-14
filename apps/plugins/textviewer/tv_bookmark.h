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

/* stuff for the bookmarking */
struct bookmark_info {
    long file_position;
    int  page;
    int  line;
    unsigned char flag;
};

int viewer_find_bookmark(int page, int line);
void viewer_add_remove_bookmark(int page, int line);
void viewer_add_bookmark(int page, int line);
void viewer_remove_last_read_bookmark(void);
void viewer_select_bookmark(int initval);

bool viewer_read_bookmark_infos(int fd);
bool viewer_write_bookmark_infos(int fd);
bool copy_bookmark_file(int sfd, int dfd, off_t start, off_t size);

#endif
