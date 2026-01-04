/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Daniel Stenberg
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
#define RB_FILESYSTEM_OS
#include <stdio.h>
#include "config.h"
#include "system.h"
#include "file.h"
#include "load_code.h"
#include "rbpaths.h"
#include "debug.h"

void * lc_open(const char *filename, unsigned char *buf, size_t buf_size)
{
    char osfilename[SIM_TMPBUF_MAX_PATH];
    if (sim_get_os_path(osfilename, filename, sizeof (osfilename)) < 0)
        return NULL;

    return os_lc_open(osfilename);
    (void)buf; (void)buf_size;
}
