/*
 * Based on LuaFileSystem : http://www.keplerproject.org/luafilesystem
 *
 * Copyright © 2003 Kepler Project.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "plugin.h"
#include "rocklibc.h"

#include "lauxlib.h"
#include "luadir.h"

#define DIR_METATABLE "directory metatable"
typedef struct dir_data {
    int  closed;
    DIR *dir;
} dir_data;

static int make_dir (lua_State *L) {
    const char *path = luaL_checkstring (L, 1);
    lua_pushboolean (L, !rb->mkdir(path));
    return 1;
}

static int remove_dir (lua_State *L) {
    const char *path = luaL_checkstring (L, 1);
    lua_pushboolean (L, !rb->rmdir (path));
    return 1;
}


/*
** Directory iterator
*/
static int dir_iter (lua_State *L) {
    struct dirent *entry;
    dir_data *d = (dir_data *)luaL_checkudata (L, 1, DIR_METATABLE);

    luaL_argcheck (L, !d->closed, 1, "closed directory");

    if ((entry = rb->readdir (d->dir)) != NULL) {
        struct dirinfo info = rb->dir_get_info(d->dir, entry);
        lua_pushstring (L, entry->d_name);
        lua_pushboolean (L, info.attribute & ATTR_DIRECTORY);
        if (lua_toboolean (L, lua_upvalueindex(1))) {
            lua_createtable(L, 0, 3);
            lua_pushnumber (L, info.attribute);
            lua_setfield (L, -2, "attribute");
            lua_pushnumber (L, info.size);
            lua_setfield (L, -2, "size");
            lua_pushnumber (L, info.mtime);
            lua_setfield (L, -2, "time");
        }
        else
        {
            lua_pushnil(L);
        }
        return 3;
    } else {
        /* no more entries => close directory */
        rb->closedir (d->dir);
        d->closed = 1;
        return 0;
    }
}


/*
** Closes directory iterators
*/
static int dir_close (lua_State *L) {
    dir_data *d = (dir_data *)lua_touserdata (L, 1);

    if (!d->closed && d->dir) {
        rb->closedir (d->dir);
        d->closed = 1;
    }
    return 0;
}


/*
** Factory of directory iterators
*/
static int dir_iter_factory (lua_State *L) {
    const char *path = luaL_checkstring (L, 1);
    dir_data *d;
    lua_settop(L, 2); /* index 2 (bool) return attribute table */
    lua_pushcclosure(L, &dir_iter, 1);
    d = (dir_data *) lua_newuserdata (L, sizeof(dir_data));
    d->closed = 0;

    luaL_getmetatable (L, DIR_METATABLE);
    lua_setmetatable (L, -2);
    d->dir = rb->opendir (path);
    if (d->dir == NULL)
    {
        luaL_error (L, "cannot open %s: %d", path, errno);
    }
    return 2;
}


/*
** Creates directory metatable.
*/
static int dir_create_meta (lua_State *L) {
    luaL_newmetatable (L, DIR_METATABLE);
    lua_createtable(L, 0, 2);
    lua_pushcfunction (L, dir_iter);
    lua_setfield (L, -2, "next");
    lua_pushcfunction (L, dir_close);
    lua_setfield (L, -2, "close");
    /* set its __index field */
    lua_setfield (L, -2, "__index");
    /* set its __gc field */
    lua_pushcfunction (L, dir_close);
    lua_setfield (L, -2, "__gc");
    return 1;
}

static const struct luaL_reg fslib[] = {
    {"dir", dir_iter_factory},
    {"mkdir", make_dir},
    {"rmdir", remove_dir},
    {NULL, NULL},
};

int luaopen_luadir (lua_State *L) {
    dir_create_meta (L);
    luaL_register (L, LUA_DIRLIBNAME, fslib);
    return 1;
}
