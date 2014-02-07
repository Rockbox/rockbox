/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright © 2013 by Thomas Martitz
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

/* hosted storage driver
 *
 * Small functions to support storage_* api on hosted targets
 * where we don't use raw access to storage devices but run on OS-mounted
 * file systems */

#include <stdbool.h>
#ifdef __unix__
#include <unistd.h>
#endif
#include "config.h"

extern void hstorage_init(void);

static inline int hstorage_flush(void)
{
#ifdef __unix__
    sync();
#endif
    return 0;
}

#ifdef HAVE_HOTSWAP
extern bool hstorage_removable(int drive);
extern bool hstorage_present(int drive);
#endif
