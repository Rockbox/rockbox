/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2012 by Andrew Ryabinin
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
#ifndef __BUILD_AUTOCONF_H
#define __BUILD_AUTOCONF_H

/* assume little endian for now */
#define ROCKBOX_LITTLE_ENDIAN 1

#define ROCKBOX_DIR "../../"

/* Rename rockbox API functions which may conflict with system's functions.
   This will allow to link libmkrk27boot with rbutil safely. */
#define read(x,y,z) mkrk27_read(x,y,z)
#define write(x,y,z) mkrk27_write(x,y,z)

/* This should resolve confilict with ipodpatcher's filesize function. */
#define filesize(x) mkrk27_filesize(x)

#endif
