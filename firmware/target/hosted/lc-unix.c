/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Thomas Martitz
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

#include <string.h> /* size_t */
#include "load_code.h"

/* unix specific because WIN32 wants UCS instead of UTF-8, so filenames
 * need to be converted */

/* plain wrappers , nothing to do */
void *lc_open(const char *filename, unsigned char *buf, size_t buf_size)
{
    return _lc_open(filename, buf, buf_size);
}

void *lc_get_header(void *handle)
{
    return _lc_get_header(handle);
}

void lc_close(void *handle)
{
    _lc_close(handle);
}

