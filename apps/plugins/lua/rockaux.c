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

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
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

struct tm * gmtime(const time_t *timep)
{
    static struct tm time;
    return rb->gmtime_r(timep, &time);
}

int get_current_path(lua_State *L, int level)
{
    lua_Debug ar;

    if(lua_getstack(L, level, &ar))
    {
        /* Try determining the base path of the current Lua chunk
            and write it to dest. */
        lua_getinfo(L, "S", &ar);

        const char* curfile = &ar.source[1];
        const char* pos = rb->strrchr(curfile, '/');
        if(pos != NULL)
        {
            lua_pushlstring (L, curfile, pos - curfile + 1);
            return 1;
        }
    }

    lua_pushnil(L);    
    return 1;
}
