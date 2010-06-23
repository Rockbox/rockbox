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
#ifndef PLUGIN_TEXT_VIEWER_READER_H
#define PLUGIN_TEXT_VIEWER_READER_H

/* stuff for the reading file */

/*
 * initialize the reader module
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
bool tv_init_reader(unsigned char **buf, size_t *bufsize);

/* finalize the reader module */
void tv_finalize_reader(void);

/*
 * return the file size
 *
 * return
 *     file size
 *
 * Note: when the file is UTF-8 file with BOM, if the encoding of the text viewer is UTF-8,
 * then file size decreases only BOM size.
 */
off_t tv_get_file_size(void);

/*
 * return the whether is the end of file or not
 *
 * return
 *     true  EOF
 *     false not EOF
 */
bool tv_is_eof(void);

/*
 * return the current file position
 *
 * return
 *     the current file position
 */
off_t tv_get_current_file_pos(void);

/*
 * return the bufer which store text data
 *
 * [Out] bufsize
 *          buffer size
 *
 * return
 *     the pointer of the buffer
 */
const unsigned char *tv_get_buffer(ssize_t *bufsize);

/*
 * seek to the given offset
 *
 * [In] offset
 *          offset size
 *
 * [In] whence
 *          SEEK_CUR  seek to the current position + offset.
 *          SEEK_SET  seek to the offset.
 *
 * Note: whence supports SEEK_CUR and SEEK_SET only.
 */
void tv_seek(off_t offset, int whence);

#endif
