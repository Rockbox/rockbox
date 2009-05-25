/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Dan Everton (safetydan)
 * String function implementations taken from dietlibc 0.31 (GPLv2 License)
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

#include "plugin.h"

#if !defined(SIMULATOR) || defined(__MINGW32__) || defined(__CYGWIN__)
int errno = 0;
#endif

char *strerror(int errnum)
{
    (void) errnum;
    
    DEBUGF("strerror()\n");
    
    return NULL;
}

long floor(long x)
{
    (void) x;
    
    DEBUGF("floor()\n");
    
    return 0;
}

long pow(long x, long y)
{
    (void) x;
    (void) y;
    
    DEBUGF("pow()\n");
    
    return 0;
}

int strcoll(const char * str1, const char * str2)
{
    return rb->strcmp(str1, str2);
}

