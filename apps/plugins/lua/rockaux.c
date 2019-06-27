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
 * Copyright (C) 2018 William Wilgus
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
#include "lib/pluginlib_actions.h"

extern long strtol(const char *nptr, char **endptr, int base);

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

/* filetol()
   reads long int from an open file, skips preceding
   whitespaces returns -1 if error, 1 on success.
   *num set to LONG_MAX or LONG_MIN on overflow.
   If number of digits is > than LUAI_MAXNUMBER2STR
   filepointer will continue till the next non digit
   but buffer will stop being filled with characters.
   Preceding zero is ignored
*/
int filetol(int fd, long *num)
{
    static char buffer[LUAI_MAXNUMBER2STR];
    int    retn = -1;
    char   chbuf = 0;
    size_t count = 0;
    bool   neg   = false;
    long   val;

    while (rb->read(fd, &chbuf, 1) == 1)
    {
        if(!isspace(chbuf) || retn == 1)
        {
            if(chbuf == '0') /* strip preceeding zeros */
            {
                *num = 0;
                retn = 1;
            }
            else if(chbuf == '-' && retn != 1)
                neg = true;
            else
            {
                rb->lseek(fd, -1, SEEK_CUR);
                break;
            }
        }
    }

    while (rb->read(fd, &chbuf, 1) == 1)
    {
        if(!isdigit(chbuf))
        {
            rb->lseek(fd, -1, SEEK_CUR);
            break;
        }
        else if (count < LUAI_MAXNUMBER2STR - 2)
            buffer[count++] = chbuf;
    }

    if(count)
    {
        buffer[count] = '\0';
        val = strtol(buffer, NULL, 10);
        *num = (neg)? -val:val;
        retn = 1;
    }

    return retn;
}

int get_plugin_action(int timeout, bool with_remote)
{
    static const struct button_mapping *m1[] = { pla_main_ctx };
    int btn;

#ifndef HAVE_REMOTE_LCD
    (void) with_remote;
#else
    static const struct button_mapping *m2[] = { pla_main_ctx, pla_remote_ctx };

    if (with_remote)
        btn = pluginlib_getaction(timeout, m2, 2);
    else
#endif
    btn = pluginlib_getaction(timeout, m1, 1);

    return btn;
}
