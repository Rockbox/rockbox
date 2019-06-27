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

#ifndef _ROCKLIB_H_
#define _ROCKLIB_H_

#define LUA_ROCKLIBNAME	"rb"

#ifndef ERR_IDX_RANGE
#define ERR_IDX_RANGE "index out of range"
#endif

#ifndef ERR_DATA_OVF
#define ERR_DATA_OVF  "data overflow"
#endif

#define RB_CONSTANT(x)        {#x, x}
#define RB_STRING_CONSTANT(x) {#x, x}

struct lua_int_reg {
  char const* name;
  const int   value;
};

struct lua_str_reg {
  char const* name;
  char const* value;
};

LUALIB_API int (luaopen_rock) (lua_State *L) __attribute__((aligned(0x8)));
/* in rockaux.c */
int get_current_path(lua_State *L, int level);
int filetol(int fd, long *num);
int get_plugin_action(int timeout, bool with_remote);

#endif /* _ROCKLIB_H_ */

