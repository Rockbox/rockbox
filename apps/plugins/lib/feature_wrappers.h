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
#ifdef HAVE_ALBUMART
#define search_albumart_files rb->search_albumart_files
#endif

/* This should only be used when loading scaled bitmaps, or using custom output
 * plugins. The pluginlib loader does not support loading bitmaps unscaled in
 * native format, so rb->read_bmp_file should always be used directly to load
 * such images.
 */
#if LCD_DEPTH > 1
#define scaled_read_bmp_file rb->read_bmp_file
#else
#define scaled_read_bmp_file read_bmp_file
#endif

#endif

