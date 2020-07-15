/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "config.h"
#include "cpu.h"
#ifdef HAVE_GDB_API
#include "gdb_api.h"
#endif

#ifdef DEBUG
static char debugmembuf[200];
#endif

#include "kernel.h"
#include "system.h"
#include "debug.h"

#ifdef DEBUG
void debug_init(void)
{
}

static inline void debug(char *msg)
{
    (void)msg;
}
#endif /* end of DEBUG section */

#ifdef __GNUC__
void debugf(const char *fmt, ...)
#endif
{
#ifdef DEBUG
    va_list ap;
    
    va_start(ap, fmt);
    vsnprintf(debugmembuf, sizeof(debugmembuf), fmt, ap);
    va_end(ap);
    debug(debugmembuf);
#else
    (void)fmt;
#endif
}

