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
#ifndef PLUGIN_TEXT_VIEWER_TEXT_READER_H
#define PLUGIN_TEXT_VIEWER_TEXT_READER_H

/*
 * initialize the text reader module
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
bool tv_init_text_reader(unsigned char **buf, size_t *bufsize);

/* finalize the text reader module */
void tv_finalize_text_reader(void);

/*
 * set the read conditions
 *
 * [In] blocks
 *          block count
 *
 * [In] width
 *          block width
 */
void tv_set_read_conditions(int blocks, int width);

/*
 * return the total text size
 *
 * return
 *     the total text size
 */
off_t tv_get_total_text_size(void);

/*
 * get the text of the next line
 *
 * [Out] buf
 *           the pointer of the pointer which store the text
 *
 * return
 *     true   next line exists
 *     false  next line does not exist
 */
bool tv_get_next_line(const unsigned char **buf);

/*
 * start to read lines
 *
 * [In] block
 *          the index of block to read text
 *
 * [In] is_multi
 *          true  read 2 blocks
 *          false read 1 block
 */
void tv_read_start(int block, bool is_multi);

/*
 * end to read lines
 *
 * return
 *     read text size
 */
int tv_read_end(void);

/* seek to the head of the file */
void tv_seek_top(void);

#endif
