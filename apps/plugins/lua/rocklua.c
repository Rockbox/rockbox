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
  {"",              luaopen_base},
  {LUA_TABLIBNAME,  luaopen_table},
  {LUA_STRLIBNAME,  luaopen_string},
  {LUA_OSLIBNAME,   luaopen_os},
  {LUA_ROCKLIBNAME, luaopen_rock},
  {LUA_BITLIBNAME,  luaopen_bit},
  {LUA_IOLIBNAME,   luaopen_io},
  {LUA_LOADLIBNAME, luaopen_package},
  {LUA_MATHLIBNAME, luaopen_math},
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

/* ldlib.c */
static lua_State *getthread (lua_State *L, int *arg) {
  if (lua_isthread(L, 1)) {
    *arg = 1;
    return lua_tothread(L, 1);
  }
  else {
    *arg = 0;
    return L;
  }
}

#define LEVELS1 12      /* size of the first part of the stack */
#define LEVELS2 10      /* size of the second part of the stack */

static int db_errorfb (lua_State *L) {
  int level;
  int firstpart = 1;  /* still before eventual `...' */
  int arg;
  lua_State *L1 = getthread(L, &arg);
  lua_Debug ar;
  if (lua_isnumber(L, arg+2)) {
    level = (int)lua_tointeger(L, arg+2);
    lua_pop(L, 1);
  }
  else
    level = (L == L1) ? 1 : 0;  /* level 0 may be this own function */
  if (lua_gettop(L) == arg)
    lua_pushliteral(L, "");
  else if (!lua_isstring(L, arg+1)) return 1;  /* message is not a string */
  else lua_pushliteral(L, "\n");
  lua_pushliteral(L, "stack traceback:");
  while (lua_getstack(L1, level++, &ar)) {
    if (level > LEVELS1 && firstpart) {
      /* no more than `LEVELS2' more levels? */
      if (!lua_getstack(L1, level+LEVELS2, &ar))
        level--;  /* keep going */
      else {
        lua_pushliteral(L, "\n\t...");  /* too many levels */
        while (lua_getstack(L1, level+LEVELS2, &ar))  /* find last levels */
          level++;
      }
      firstpart = 0;
      continue;
    }
    lua_pushliteral(L, "\n\t");
    lua_getinfo(L1, "Snl", &ar);
    lua_pushfstring(L, "%s:", ar.short_src);
    if (ar.currentline > 0)
      lua_pushfstring(L, "%d:", ar.currentline);
    if (*ar.namewhat != '\0')  /* is there a name? */
        lua_pushfstring(L, " in function " LUA_QS, ar.name);
    else {
      if (*ar.what == 'm')  /* main? */
        lua_pushfstring(L, " in main chunk");
      else if (*ar.what == 'C' || *ar.what == 't')
        lua_pushliteral(L, " ?");  /* C function or tail call */
      else
        lua_pushfstring(L, " in function <%s:%d>",
                           ar.short_src, ar.linedefined);
    }
    lua_concat(L, lua_gettop(L) - arg);
  }
  lua_concat(L, lua_gettop(L) - arg);
  return 1;
}

/* lua.c */
static int traceback (lua_State *L) {
  lua_pushcfunction(L, db_errorfb);
  lua_pushvalue(L, 1);  /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}

static int docall (lua_State *L) {
  int status;
  int base = lua_gettop(L);  /* function index */
  lua_pushcfunction(L, traceback);  /* push traceback function */
  lua_insert(L, base);  /* put it under chunk and args */
  status = lua_pcall(L, 0, 0, base);
  lua_remove(L, base);  /* remove traceback function */
  /* force a complete garbage collection in case of errors */
  if (status != 0) lua_gc(L, LUA_GCCOLLECT, 0);
  return status;
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

        lua_State *L = luaL_newstate();

        rocklua_openlibs(L);
        status = luaL_loadfile(L, filename);
        if (!status) {
            rb->lcd_clear_display();
            status = docall(L);
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

