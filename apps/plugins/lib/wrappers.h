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

#define open rb->open
#define close rb->close
#define read rb->read
#define lseek rb->lseek
#define memset rb->memset
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#define cpu_boost rb->cpu_boost
#endif
#define yield rb->yield

#endif

