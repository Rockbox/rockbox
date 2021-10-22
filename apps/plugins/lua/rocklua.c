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
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "rocklib.h"
#include "rocklib_img.h"
#include "luadir.h"
#include "rocklib_events.h"

static lua_State *Ls = NULL;
static int lu_status = 0;

static const luaL_Reg lualibs[] = {
  {"",              luaopen_base},
  {LUA_LOADLIBNAME, luaopen_package},
  {LUA_TABLIBNAME,  luaopen_table},
  {LUA_STRLIBNAME,  luaopen_string},
  {LUA_BITLIBNAME,  luaopen_bit},
  {LUA_IOLIBNAME,   luaopen_io},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_OSLIBNAME,   luaopen_os},
  {LUA_ROCKLIBNAME, luaopen_rock},
  {LUA_ROCKLIBNAME, luaopen_rock_img},
  {LUA_ROCKEVENTSNAME, luaopen_rockevents},
  {LUA_DIRLIBNAME,  luaopen_luadir},
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
  else lua_pushliteral(L, "\n\n");
  lua_pushliteral(L, "stack traceback: ");
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
    lua_pushliteral(L, "\n\n\t");
    lua_getinfo(L1, "Snl", &ar);
    char* filename = rb->strrchr(ar.short_src, '/'); /* remove path */
    lua_pushfstring(L, "%s:", filename ? filename : ar.short_src);
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
  lua_pushfstring(L, "\n\nRam Used: %d Kb", lua_gc (L, LUA_GCCOUNT, 0));
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

static void lua_atexit(void);
static int lua_split_arguments(lua_State *L, const char *filename);

static int loadfile_newstate(lua_State **L, const char *filename)
{
  const char *file;
  int ret;

  *L = luaL_newstate();
  rb_atexit(lua_atexit);

  lua_gc(*L, LUA_GCSTOP, 0);  /* stop collector during initialization */
  rocklua_openlibs(*L);

  lua_split_arguments(*L, filename);
  lua_setglobal (*L, "_arguments");
  file = lua_tostring (*L, -1);
  lua_setglobal (*L, "_fullpath");
  /* lua manual -> no guarantee pointer valid after value is removed from stack */
  ret = luaL_loadfile(*L, file);
  lua_gc(*L, LUA_GCRESTART, 0);

  return ret;
}

static void lua_atexit(void)
{
  char *filename;
  int err_n;
  if(Ls && lua_gettop(Ls) > 1)
  {
    err_n = lua_tointeger(Ls, -1); /* os.exit? */
    if (Ls == lua_touserdata(Ls, -1)) /* signal from restart_lua */
    {
      filename = (char *) malloc((MAX_PATH * 2) + 1);

      if (filename) {/* out of memory? */
        filename[MAX_PATH * 2] = '\0';
        rb->strlcpy(filename, lua_tostring(Ls, -2), MAX_PATH * 2);
      } else {
        goto ERR_RUN;
      }
      lua_close(Ls); /* close old state */

      lu_status = loadfile_newstate(&Ls, filename);

      free(filename);
      plugin_start(NULL);
    }
    else if (err_n >= PLUGIN_USB_CONNECTED) /* INTERNAL PLUGIN RETVAL */
    {
      lua_close(Ls);
      _exit(err_n);  /* don't call exit handler */
    }
    else if (err_n != 0)
    {
ERR_RUN:
      lu_status = LUA_ERRRUN;
      lua_pop(Ls, 1); /* put exit string on top of stack */
      plugin_start(NULL);
    }
    else
      lua_close(Ls);
  }
  _exit(PLUGIN_OK); /* don't call exit handler */
}

/* split filename at argchar
 * remainder of filename pushed on stack (-1)
* argument string pushed on stack or nil if doesn't exist (-2)
 */
static int lua_split_arguments(lua_State *L, const char *filename)
{
  const char argchar = '?';
  const char* arguments = strchr(filename, argchar);
  if(arguments) {
    lua_pushstring(L, (arguments + 1));
  } else {
    arguments = strlen(filename) + filename;
    lua_pushnil(L);
  }
  lua_pushlstring(L, filename, arguments - filename);
  lua_insert(L, -2); /* swap filename and argument */
  return 2;
}

static void display_traceback(const char *errstr)
{
#if 1
  splash_scroller(HZ * 5, errstr); /*rockaux.c*/
#else
  rb->splash(10 * HZ, errstr);
#endif
}
/***************** Plugin Entry Point *****************/
enum plugin_status plugin_start(const void* parameter)
{
    const char* filename;

    if (parameter == NULL)
    {
      if (!Ls)
        rb->splash(HZ, "Play a .lua file!");
    }
    else
    {
        filename = (char*) parameter;
        lu_status = loadfile_newstate(&Ls, filename);
    }

    if (Ls)
    {
        if (!lu_status) {
            rb->lcd_scroll_stop(); /* rb doesn't like bg change while scroll */
            rb->lcd_clear_display();
            lu_status= docall(Ls);
        }

        if (lu_status) {
            DEBUGF("%s\n", lua_tostring(Ls, -1));
            display_traceback(lua_tostring(Ls, -1));
            //rb->splash(10 * HZ, lua_tostring(Ls, -1));
            /*lua_pop(Ls, 1);*/
        }
        lua_close(Ls);
    }
    else
      return PLUGIN_ERROR;

    return PLUGIN_OK;
}
