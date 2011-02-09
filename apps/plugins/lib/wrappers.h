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
* This header redefines some core functions as calls via the plugin_api, to
* allow easy compilation of core source files in the pluginlib with different
* features from the version built for the core, or when a core object file is
* not built for a particular target.
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

#ifndef _LIB_WRAPPERS_H_
#define _LIB_WRAPPERS_H_

#define DEBUG_H
#define __SPRINTF_H__

#define open rb->open
#define close rb->close
#define read rb->read
#define lseek rb->lseek
#define memset rb->memset
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#ifdef CPU_BOOST_LOGGING
#define cpu_boost_ rb->cpu_boost_
#else
#define cpu_boost rb->cpu_boost
#endif
#endif
#define yield rb->yield
#define file_exists rb->file_exists
#define snprintf rb->snprintf
#define strcat rb->strcat
#define strchr rb->strchr
#define strcmp rb->strcmp
#define strcpy rb->strcpy
#define strip_extension rb->strip_extension
#define strlen rb->strlen
#define strlcpy rb->strlcpy
#define strrchr rb->strrchr
#define filesize rb->filesize

#endif

