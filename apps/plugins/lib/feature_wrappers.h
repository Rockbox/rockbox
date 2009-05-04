/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2009 by Andrew Mahone
*
* This defines wrappers for some features which are used differently depending
* on how the target was built, primarily because of core features being accesed
* via pluginlib on targets where they are missing from core.
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

#ifndef _LIB_FEATURE_WRAPPERS_H_
#define _LIB_FEATURE_WRAPPERS_H_

/* search_albumart_files is only available in core with HAVE_ALBUMART defined,
 * but can easily be implement in pluginlib as long as the database is
 * available.
 */
#if defined(HAVE_ALBUMART) && defined(HAVE_JPEG)
#define search_albumart_files rb->search_albumart_files
#endif

/* This should only be used when loading scaled bitmaps, or using custom output
 * plugins. A native output plugin for the scaler is available as format_native
 * on targets with LCD_DEPTH > 1
 */
#ifdef HAVE_BMP_SCALING
#define scaled_read_bmp_file rb->read_bmp_file
#define scaled_read_bmp_fd rb->read_bmp_fd
#else
#define scaled_read_bmp_file read_bmp_file
#define scaled_read_bmp_fd read_bmp_fd
#endif

#ifdef HAVE_JPEG
#define read_jpeg_file rb->read_jpeg_file
#define read_jpeg_fd rb->read_jpeg_fd
#else
#endif

#endif

