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
#define _ROCKCONF_H_ /* Protect against unwanted include */
#include "lua.h"

#if !defined(SIMULATOR)
int errno = 0;
#endif

char *strerror(int errnum)
{
    (void) errnum;

    DEBUGF("strerror(%d)\n", errnum);

    return NULL;
}

long rb_pow(long x, long n)
{
    long pow = 1;
    unsigned long u;

    if(n <= 0)
    {
        if(n == 0 || x == 1)
            return 1;

        if(x != -1)
            return x != 0 ? 1/x : 0;

        n = -n;
    }

    u = n;
    while(1)
    {
        if(u & 01)
            pow *= x;

        if(u >>= 1)
            x *= x;
        else
            break;
    }

    return pow;
}

int strcoll(const char * str1, const char * str2)
{
    return rb->strcmp(str1, str2);
}

const char* get_current_path(lua_State *L, int level)
{
    static char buffer[MAX_PATH];
    lua_Debug ar;

    if(lua_getstack(L, level, &ar))
    {
        /* Try determining the base path of the current Lua chunk
            and write it to dest. */
        lua_getinfo(L, "S", &ar);

        char* curfile = (char*) &ar.source[1];
        char* pos = rb->strrchr(curfile, '/');
        if(pos != NULL)
        {
            unsigned int len = (unsigned int)(pos - curfile);
            len = len + 1 > sizeof(buffer) ? sizeof(buffer) - 1 : len;

            if(len > 0)
                memcpy(buffer, curfile, len);

            buffer[len] = '/';
            buffer[len+1] = '\0';

            return buffer;
        }
    }

    return NULL;
}
