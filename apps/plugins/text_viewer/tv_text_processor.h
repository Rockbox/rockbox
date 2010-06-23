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
#ifndef PLUGIN_TEXT_VIEWER_TEXT_PROCESSOR_H
#define PLUGIN_TEXT_VIEWER_TEXT_PROCESSOR_H

/*
 * initialize the text processor module
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
bool tv_init_text_processor(unsigned char **buf, size_t *bufsize);

/*
 * set the processing conditions
 *
 * [In] blocks
 *          total block count
 *
 * [In] width
 *          the block width
 */
void tv_set_creation_conditions(int blocks, int width);

/*
 * create the displayed text
 *
 * [In] src
 *          the start pointer of the buffer
 *
 * [In] bufsize
 *          buffer size
 *
 * [In] block
 *          the index of block to read text
 *
 * [In] is_multi
 *          true  read 2 blocks
 *          false read 1 block
 *
 * [Out] dst
 *          the pointer of the pointer which store the text
 *
 * return
 *     the size of the text
 */
int tv_create_formed_text(const unsigned char *src, ssize_t bufsize,
                              int block, bool is_multi, const unsigned char **dst);

#endif
