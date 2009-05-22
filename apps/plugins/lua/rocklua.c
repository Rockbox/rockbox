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
#include "lib/pluginlib_exit.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "rocklib.h"
#include "rockmalloc.h"

PLUGIN_HEADER

static const luaL_Reg lualibs[] = {
  {"", luaopen_base},
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_ROCKLIBNAME, luaopen_rock},
  {NULL, NULL}
};

static void rocklua_openlibs(lua_State *L) {
  const luaL_Reg *lib = lualibs;
  for (; lib->func; lib++) {
    lua_pushcfunction(L, lib->func);
    lua_pushstring(L, lib->name);
    lua_call(L, 1, 0);
  }
}

char curpath[MAX_PATH];
static void fill_curpath(const char* filename)
{
    char* pos = rb->strrchr(filename, '/');

    if(pos != NULL)
    {
        int len = (int)(pos - filename);

        if(len > 0)
            memcpy(curpath, filename, len);

        curpath[len] = '\0';
    }
}


/***************** Plugin Entry Point *****************/
enum plugin_status plugin_start(const void* parameter)
{
    const char* filename;
    int status;

    PLUGINLIB_EXIT_INIT

    if (parameter == NULL)
    {
        rb->splash(HZ, "Play a .lua file!");
        return PLUGIN_ERROR;
    }
    else
    {
        filename = (char*) parameter;
        fill_curpath(filename);

        lua_State *L = luaL_newstate();

        rocklua_openlibs(L);
        status = luaL_loadfile(L, filename);
        if (!status) {
            rb->lcd_clear_display();
            status = lua_pcall(L, 0, 0, 0);
        }

        dlmalloc_stats();

        if (status) {
            DEBUGF("%s\n", lua_tostring(L, -1));
            rb->splashf(5 * HZ, "%s", lua_tostring(L, -1));
            lua_pop(L, 1);
        }

        lua_close(L);
    }

    return PLUGIN_OK;
}

