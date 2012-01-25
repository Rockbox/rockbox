/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2002 Daniel Stenberg
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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
static unsigned old_cp;

void debug_exit(void)
{
    /* Reset console output codepage */
    SetConsoleOutputCP(old_cp);
}

void debug_init(void)
{
    old_cp = GetConsoleOutputCP();
    /* Set console output codepage to UTF8. Only works
     * correctly when the console uses a truetype font. */
    SetConsoleOutputCP(65001);
    atexit(debug_exit);
}
#else
void debug_init(void)
{
    /* nothing to be done */
}
#endif

void debugf(const char *fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
}

void ldebugf(const char* file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );
    fprintf( stderr, "%s:%d ", file, line );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
}
