/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#include "hwstub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <lua.hpp>
#include <unistd.h>
#include "soc_desc.hpp"

#if LUA_VERSION_NUM < 502
#warning You need at least lua 5.2
#endif

/**
 * Global variables
 */
bool g_quiet = false;
bool g_exit = false;
struct hwstub_device_t *g_hwdev;
struct hwstub_version_desc_t g_hwdev_ver;
struct hwstub_layout_desc_t g_hwdev_layout;
struct hwstub_target_desc_t g_hwdev_target;
struct hwstub_stmp_desc_t g_hwdev_stmp;
struct hwstub_pp_desc_t g_hwdev_pp;
lua_State *g_lua;

/**
 * hw specific
 */

void print_log(struct hwstub_device_t *hwdev)
{
    do
    {
        char buffer[128];
        int length = hwstub_get_log(hwdev, buffer, sizeof(buffer) - 1);
        if(length <= 0)
            break;
        buffer[length] = 0;
        printf("%s", buffer);
    }while(1);
}

/**
 * Lua specific
 */
int my_lua_help(lua_State *state)
{
    bool has_sub = false;
    // implement help() in C so that we do not rely on the init to implement it
    // help can take optional arguments
    int n = lua_gettop(state);

    lua_getglobal(state, "hwstub");
    if(!lua_istable(state, -1))
        goto Lerr;
    lua_getfield(state, -1, "help");
    if(!lua_istable(state, -1))
        goto Lerr;

    for(int i = 1; i <= n; i++)
    {
        lua_pushvalue(state, i);
        lua_gettable(state, -2);
        if(lua_isnil(state, -1))
        {
            printf("I don't know subtopic '%s'!\n", lua_tostring(state, i));
            return 0;
        }
        if(!lua_istable(state, -1))
        {
            printf("Subtopic '%s' is not a table!\n", lua_tostring(state, i));
            return 0;
        }
    }

    printf("================[ HELP ");
    for(int i = 1; i <= n; i++)
        printf("> %s ", lua_tostring(state, i));
    printf("]================\n");

    lua_pushnil(state);
    while(lua_next(state, -2))
    {
        // key is at -2 and value at -1
        if(lua_isstring(state, -1))
            printf("%s\n", lua_tostring(state, -1));
        else if(lua_istable(state, -1))
            has_sub = true;
        // pop value but keep key
        lua_pop(state, 1);
    }

    if(has_sub)
    {
        printf("\n");
        printf("You can get more information on the following subtopics:\n");
        lua_pushnil(state);
        while(lua_next(state, -2))
        {
            // key is at -2 and value at -1
            if(lua_istable(state, -1))
                printf("* %s\n", lua_tostring(state, -2));
            // pop value but keep key
            lua_pop(state, 1);
        }
    }
    printf("================[ STOP ]================\n");

    return 0;

    Lerr:
    printf("There is a problem with the Lua context. Help is expected to be in hwstub.help\n");
    printf("You must have messed badly the environment.\n");
    return 0;
}

typedef soc_word_t (*hw_readn_fn_t)(lua_State *state, soc_addr_t addr);
typedef void (*hw_writen_fn_t)(lua_State *state, soc_addr_t addr, soc_word_t val);

soc_word_t hw_read8(lua_State *state, soc_addr_t addr)
{
    uint8_t u;
    if(hwstub_rw_mem_atomic(g_hwdev, 1, addr, &u, sizeof(u)) != sizeof(u))
        luaL_error(state, "fail to read8 @ %p", addr);
    return u;
}

soc_word_t hw_read16(lua_State *state, soc_addr_t addr)
{
    uint16_t u;
    if(hwstub_rw_mem_atomic(g_hwdev, 1, addr, &u, sizeof(u)) != sizeof(u))
        luaL_error(state, "fail to read16 @ %p", addr);
    return u;
}

soc_word_t hw_read32(lua_State *state, soc_addr_t addr)
{
    uint32_t u;
    if(hwstub_rw_mem_atomic(g_hwdev, 1, addr, &u, sizeof(u)) != sizeof(u))
        luaL_error(state, "fail to read32 @ %p", addr);
    return u;
}

void hw_write8(lua_State *state, soc_addr_t addr, soc_word_t val)
{
    uint8_t u = val;
    if(hwstub_rw_mem_atomic(g_hwdev, 0, addr, &u, sizeof(u)) != sizeof(u))
        luaL_error(state, "fail to write8 @ %p", addr);
}

void hw_write16(lua_State *state, soc_addr_t addr, soc_word_t val)
{
    uint16_t u = val;
    if(hwstub_rw_mem_atomic(g_hwdev, 0, addr, &u, sizeof(u)) != sizeof(u))
        luaL_error(state, "fail to write16 @ %p", addr);
}

void hw_write32(lua_State *state, soc_addr_t addr, soc_word_t val)
{
    uint32_t u = val;
    if(hwstub_rw_mem_atomic(g_hwdev, 0, addr, &u, sizeof(u)) != sizeof(u))
        luaL_error(state, "fail to write32 @ %p", addr);
}

int my_lua_readn(lua_State *state)
{
    hw_readn_fn_t fn = (hw_readn_fn_t)lua_touserdata(state, lua_upvalueindex(1));
    int n = lua_gettop(state);
    if(n != 1)
        luaL_error(state, "readn takes a single argument");
    lua_pushunsigned(state, fn(state, luaL_checkunsigned(state, 1)));
    return 1;
}

int my_lua_writen(lua_State *state)
{
    hw_writen_fn_t fn = (hw_writen_fn_t)lua_touserdata(state, lua_upvalueindex(1));
    int n = lua_gettop(state);
    if(n != 2)
        luaL_error(state, "writen takes two arguments");
    fn(state, luaL_checkunsigned(state, 1), luaL_checkunsigned(state, 2));
    return 0;
}

int my_lua_call(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 1)
        luaL_error(state, "call takes target address argument");

    hwstub_call(g_hwdev, luaL_checkunsigned(state, 1));
    return 0;
}

int my_lua_jump(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 1)
        luaL_error(state, "jump takes target address argument");

    hwstub_jump(g_hwdev, luaL_checkunsigned(state, 1));
    return 0;
}

int my_lua_printlog(lua_State *state)
{
    print_log(g_hwdev);
    return 0;
}

int my_lua_quit(lua_State *state)
{
    g_exit = true;
    return 0;
}

int my_lua_udelay(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 1)
        luaL_error(state, "udelay takes one argument");
    long usec = lua_tointeger(state, -1);
    usleep(usec);
    return 0;
}

bool my_lua_import_hwstub()
{
    int oldtop = lua_gettop(g_lua);

    lua_newtable(g_lua); // hwstub

    lua_newtable(g_lua); // options
    lua_pushboolean(g_lua, g_quiet);
    lua_setfield(g_lua, -2, "quiet");
    lua_setfield(g_lua, -2, "options");

    lua_newtable(g_lua); // dev
    lua_newtable(g_lua); // version
    lua_pushinteger(g_lua, g_hwdev_ver.bMajor);
    lua_setfield(g_lua, -2, "major");
    lua_pushinteger(g_lua, g_hwdev_ver.bMinor);
    lua_setfield(g_lua, -2, "minor");
    lua_pushinteger(g_lua, g_hwdev_ver.bRevision);
    lua_setfield(g_lua, -2, "revision");
    lua_setfield(g_lua, -2, "version");

    lua_newtable(g_lua); // layout
    lua_newtable(g_lua); // code
    lua_pushinteger(g_lua, g_hwdev_layout.dCodeStart);
    lua_setfield(g_lua, -2, "start");
    lua_pushinteger(g_lua, g_hwdev_layout.dCodeSize);
    lua_setfield(g_lua, -2, "size");
    lua_setfield(g_lua, -2, "code");
    lua_newtable(g_lua); // stack
    lua_pushinteger(g_lua, g_hwdev_layout.dStackStart);
    lua_setfield(g_lua, -2, "start");
    lua_pushinteger(g_lua, g_hwdev_layout.dStackSize);
    lua_setfield(g_lua, -2, "size");
    lua_setfield(g_lua, -2, "stack");
    lua_newtable(g_lua); // buffer
    lua_pushinteger(g_lua, g_hwdev_layout.dBufferStart);
    lua_setfield(g_lua, -2, "start");
    lua_pushinteger(g_lua, g_hwdev_layout.dBufferSize);
    lua_setfield(g_lua, -2, "size");
    lua_setfield(g_lua, -2, "buffer");
    lua_setfield(g_lua, -2, "layout");

    lua_newtable(g_lua); // target
    lua_pushstring(g_lua, g_hwdev_target.bName);
    lua_setfield(g_lua, -2, "name");
    lua_pushinteger(g_lua, g_hwdev_target.dID);
    lua_setfield(g_lua, -2, "id");
    lua_pushinteger(g_lua, HWSTUB_TARGET_UNK);
    lua_setfield(g_lua, -2, "UNK");
    lua_pushinteger(g_lua, HWSTUB_TARGET_STMP);
    lua_setfield(g_lua, -2, "STMP");
    lua_pushinteger(g_lua, HWSTUB_TARGET_PP);
    lua_setfield(g_lua, -2, "PP");
    lua_pushinteger(g_lua, HWSTUB_TARGET_RK27);
    lua_setfield(g_lua, -2, "RK27");
    lua_pushinteger(g_lua, HWSTUB_TARGET_ATJ);
    lua_setfield(g_lua, -2, "ATJ");
    lua_setfield(g_lua, -2, "target");

    if(g_hwdev_target.dID == HWSTUB_TARGET_STMP)
    {
        lua_newtable(g_lua); // stmp
        lua_pushinteger(g_lua, g_hwdev_stmp.wChipID);
        lua_setfield(g_lua, -2, "chipid");
        lua_pushinteger(g_lua, g_hwdev_stmp.bRevision);
        lua_setfield(g_lua, -2, "rev");
        lua_pushinteger(g_lua, g_hwdev_stmp.bPackage);
        lua_setfield(g_lua, -2, "package");
        lua_setfield(g_lua, -2, "stmp");
    }
    else if(g_hwdev_target.dID == HWSTUB_TARGET_PP)
    {
        lua_newtable(g_lua); // pp
        lua_pushinteger(g_lua, g_hwdev_pp.wChipID);
        lua_setfield(g_lua, -2, "chipid");
        lua_pushlstring(g_lua, (const char *)g_hwdev_pp.bRevision, 2);
        lua_setfield(g_lua, -2, "rev");
        lua_setfield(g_lua, -2, "pp");
    }

    lua_pushlightuserdata(g_lua, (void *)&hw_read8);
    lua_pushcclosure(g_lua, my_lua_readn, 1);
    lua_setfield(g_lua, -2, "read8");
    lua_pushlightuserdata(g_lua, (void *)&hw_read16);
    lua_pushcclosure(g_lua, my_lua_readn, 1);
    lua_setfield(g_lua, -2, "read16");
    lua_pushlightuserdata(g_lua, (void *)&hw_read32);
    lua_pushcclosure(g_lua, my_lua_readn, 1);
    lua_setfield(g_lua, -2, "read32");

    lua_pushlightuserdata(g_lua, (void *)&hw_write8);
    lua_pushcclosure(g_lua, my_lua_writen, 1);
    lua_setfield(g_lua, -2, "write8");
    lua_pushlightuserdata(g_lua, (void *)&hw_write16);
    lua_pushcclosure(g_lua, my_lua_writen, 1);
    lua_setfield(g_lua, -2, "write16");
    lua_pushlightuserdata(g_lua, (void *)&hw_write32);
    lua_pushcclosure(g_lua, my_lua_writen, 1);
    lua_setfield(g_lua, -2, "write32");
    lua_pushcclosure(g_lua, my_lua_printlog, 0);
    lua_setfield(g_lua, -2, "print_log");
    lua_pushcclosure(g_lua, my_lua_call, 0);
    lua_setfield(g_lua, -2, "call");
    lua_pushcclosure(g_lua, my_lua_jump, 0);
    lua_setfield(g_lua, -2, "jump");

    lua_setfield(g_lua, -2, "dev");

    lua_newtable(g_lua); // host
    lua_newtable(g_lua); // version
    lua_pushinteger(g_lua, HWSTUB_VERSION_MAJOR);
    lua_setfield(g_lua, -2, "major");
    lua_pushinteger(g_lua, HWSTUB_VERSION_MINOR);
    lua_setfield(g_lua, -2, "minor");
    lua_setfield(g_lua, -2, "version");
    lua_setfield(g_lua, -2, "host");

    lua_newtable(g_lua); // soc
    lua_setfield(g_lua, -2, "soc");

    lua_newtable(g_lua); // help
    lua_pushinteger(g_lua, 1);
    lua_pushstring(g_lua, "This is the help for hwstub_tool. This tools uses Lua to interpret commands.");
    lua_settable(g_lua, -3);
    lua_pushinteger(g_lua, 2);
    lua_pushstring(g_lua, "You can get help by running help(). Help is organised in topics and subtopics and so on.");
    lua_settable(g_lua, -3);
    lua_pushinteger(g_lua, 3);
    lua_pushstring(g_lua, "If you want to access the help of topic x, subtopic y, subsubtopic z, type help(x,y,z).");
    lua_settable(g_lua, -3);
    lua_pushinteger(g_lua, 4);
    lua_pushstring(g_lua, "Example: help(\"hwstub\").");
    lua_settable(g_lua, -3);
    lua_setfield(g_lua, -2, "help");

    lua_pushcclosure(g_lua, my_lua_udelay, 0);
    lua_setfield(g_lua, -2, "udelay");

    lua_setglobal(g_lua, "hwstub");

    lua_pushcfunction(g_lua, my_lua_help);
    lua_setglobal(g_lua, "help");

    lua_pushcfunction(g_lua, my_lua_quit);
    lua_setglobal(g_lua, "exit");

    lua_pushcfunction(g_lua, my_lua_quit);
    lua_setglobal(g_lua, "quit");

    if(lua_gettop(g_lua) != oldtop)
    {
        printf("internal error: unbalanced my_lua_import_soc\n");
        return false;
    }
    return true;
}

int my_lua_read_reg(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 0)
        luaL_error(state, "read() takes no argument");
    soc_addr_t addr = lua_tounsigned(state, lua_upvalueindex(1));
    lua_pushunsigned(state, hw_read32(state, addr));
    return 1;
}

int my_lua_write_reg(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 1)
        luaL_error(state, "write() takes one argument");
    soc_word_t val = luaL_checkunsigned(state, 1);
    soc_addr_t addr = lua_tounsigned(state, lua_upvalueindex(1));
    hw_write32(state, addr, val);
    return 0;
}

int my_lua_read_field(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 0)
        luaL_error(state, "read() takes no argument");
    soc_addr_t addr = lua_tounsigned(state, lua_upvalueindex(1));
    soc_word_t shift = lua_tounsigned(state, lua_upvalueindex(2));
    soc_word_t mask = lua_tounsigned(state, lua_upvalueindex(3));
    lua_pushunsigned(state, (hw_read32(state, addr) >> shift) & mask);
    return 1;
}

int my_lua_write_field(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 0 && n!= 1)
        luaL_error(state, "write() takes one or no argument");
    soc_addr_t addr = lua_tounsigned(state, lua_upvalueindex(1));
    soc_word_t shift = lua_tounsigned(state, lua_upvalueindex(2));
    soc_word_t mask = lua_tounsigned(state, lua_upvalueindex(3));
    bool is_sct = lua_toboolean(state, lua_upvalueindex(5));

    soc_word_t value = mask;
    if(n == 1)
    {
        if(!lua_isnumber(state, 1) && lua_isstring(state, 1))
        {
            lua_pushvalue(state, lua_upvalueindex(4));
            lua_pushvalue(state, 1);
            lua_gettable(state, -2);
            if(lua_isnil(state, -1))
                luaL_error(state, "field has no value %s", lua_tostring(state, 1));
            value = luaL_checkunsigned(state, -1);
            lua_pop(state, 2);
        }
        else
            value = luaL_checkunsigned(state, 1);
        value &= mask;
    }

    if(!is_sct)
        value = value << shift | (hw_read32(state, addr) & ~(mask << shift));
    else
        value <<= shift;

    hw_write32(state, addr, value);
    return 0;
}

int my_lua_sct_reg(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 1)
        luaL_error(state, "sct() takes one argument");
    soc_addr_t addr = lua_tounsigned(state, lua_upvalueindex(1));
    char op = lua_tounsigned(state, lua_upvalueindex(2));

    soc_word_t mask = luaL_checkunsigned(state, 1);
    soc_word_t value = hw_read32(state, addr);
    if(op == 's')
        value |= mask;
    else if(op == 'c')
        value &= ~mask;
    else if(op == 't')
        value ^= mask;
    else
        luaL_error(state, "sct() internal error");

    hw_write32(state, addr, value);
    return 0;
}

void my_lua_create_field(soc_addr_t addr, const soc_reg_field_t& field, bool sct)
{
    lua_newtable(g_lua);

    lua_pushstring(g_lua, field.name.c_str());
    lua_setfield(g_lua, -2, "name");

    lua_pushunsigned(g_lua, addr);
    lua_setfield(g_lua, -2, "addr");

    lua_pushboolean(g_lua, sct);
    lua_setfield(g_lua, -2, "sct");

    lua_pushunsigned(g_lua, field.first_bit);
    lua_setfield(g_lua, -2, "first_bit");

    lua_pushunsigned(g_lua, field.last_bit);
    lua_setfield(g_lua, -2, "last_bit");

    lua_pushunsigned(g_lua, field.bitmask());
    lua_setfield(g_lua, -2, "bitmask");

    soc_word_t local_bitmask = field.bitmask() >> field.first_bit;
    lua_pushunsigned(g_lua, local_bitmask);
    lua_setfield(g_lua, -2, "local_bitmask");

    lua_pushunsigned(g_lua, addr);
    lua_pushunsigned(g_lua, field.first_bit);
    lua_pushunsigned(g_lua, local_bitmask);
    lua_pushcclosure(g_lua, my_lua_read_field, 3);
    lua_setfield(g_lua, -2, "read");

    lua_pushunsigned(g_lua, addr);
    lua_pushunsigned(g_lua, field.first_bit);
    lua_pushunsigned(g_lua, local_bitmask);
    lua_pushvalue(g_lua, -4);
    lua_pushboolean(g_lua, false);
    lua_pushcclosure(g_lua, my_lua_write_field, 5);
    lua_setfield(g_lua, -2, "write");

    if(sct)
    {
        lua_pushunsigned(g_lua, addr + 4);
        lua_pushunsigned(g_lua, field.first_bit);
        lua_pushunsigned(g_lua, local_bitmask);
        lua_pushvalue(g_lua, -4);
        lua_pushboolean(g_lua, true);
        lua_pushcclosure(g_lua, my_lua_write_field, 5);
        lua_setfield(g_lua, -2, "set");

        lua_pushunsigned(g_lua, addr + 8);
        lua_pushunsigned(g_lua, field.first_bit);
        lua_pushunsigned(g_lua, local_bitmask);
        lua_pushvalue(g_lua, -4);
        lua_pushboolean(g_lua, true);
        lua_pushcclosure(g_lua, my_lua_write_field, 5);
        lua_setfield(g_lua, -2, "clr");

        lua_pushunsigned(g_lua, addr + 12);
        lua_pushunsigned(g_lua, field.first_bit);
        lua_pushunsigned(g_lua, local_bitmask);
        lua_pushvalue(g_lua, -4);
        lua_pushboolean(g_lua, true);
        lua_pushcclosure(g_lua, my_lua_write_field, 5);
        lua_setfield(g_lua, -2, "tog");
    }

    for(size_t i = 0; i < field.value.size(); i++)
    {
        lua_pushunsigned(g_lua, field.value[i].value);
        lua_setfield(g_lua, -2, field.value[i].name.c_str());
    }
}

void my_lua_create_reg(soc_addr_t addr, size_t index, const soc_reg_t& reg)
{
    lua_newtable(g_lua);

    lua_pushstring(g_lua, reg.addr[index].name.c_str());
    lua_setfield(g_lua, -2, "name");

    lua_pushunsigned(g_lua, addr + reg.addr[index].addr);
    lua_setfield(g_lua, -2, "addr");

    lua_pushboolean(g_lua, !!(reg.flags & REG_HAS_SCT));
    lua_setfield(g_lua, -2, "sct");

    lua_pushunsigned(g_lua, addr + reg.addr[index].addr);
    lua_pushcclosure(g_lua, my_lua_read_reg, 1);
    lua_setfield(g_lua, -2, "read");

    lua_pushunsigned(g_lua, addr + reg.addr[index].addr);
    lua_pushcclosure(g_lua, my_lua_write_reg, 1);
    lua_setfield(g_lua, -2, "write");

    if(reg.flags & REG_HAS_SCT)
    {
        lua_pushunsigned(g_lua, addr + reg.addr[index].addr + 4);
        lua_pushcclosure(g_lua, my_lua_write_reg, 1);
        lua_setfield(g_lua, -2, "set");

        lua_pushunsigned(g_lua, addr + reg.addr[index].addr + 8);
        lua_pushcclosure(g_lua, my_lua_write_reg, 1);
        lua_setfield(g_lua, -2, "clr");

        lua_pushunsigned(g_lua, addr + reg.addr[index].addr + 12);
        lua_pushcclosure(g_lua, my_lua_write_reg, 1);
        lua_setfield(g_lua, -2, "tog");
    }
    else
    {
        lua_pushunsigned(g_lua, addr + reg.addr[index].addr);
        lua_pushunsigned(g_lua, 's');
        lua_pushcclosure(g_lua, my_lua_sct_reg, 2);
        lua_setfield(g_lua, -2, "set");

        lua_pushunsigned(g_lua, addr + reg.addr[index].addr);
        lua_pushunsigned(g_lua, 'c');
        lua_pushcclosure(g_lua, my_lua_sct_reg, 2);
        lua_setfield(g_lua, -2, "clr");

        lua_pushunsigned(g_lua, addr + reg.addr[index].addr);
        lua_pushunsigned(g_lua, 't');
        lua_pushcclosure(g_lua, my_lua_sct_reg, 2);
        lua_setfield(g_lua, -2, "tog");
    }

    for(size_t i = 0; i < reg.field.size(); i++)
    {
        my_lua_create_field(addr + reg.addr[index].addr, reg.field[i],
            reg.flags & REG_HAS_SCT);
        lua_setfield(g_lua, -2, reg.field[i].name.c_str());
    }
}

void my_lua_create_dev(size_t index, const soc_dev_t& dev)
{
    lua_newtable(g_lua);

    lua_pushstring(g_lua, dev.addr[index].name.c_str());
    lua_setfield(g_lua, -2, "name");

    lua_pushunsigned(g_lua, dev.addr[index].addr);
    lua_setfield(g_lua, -2, "addr");

    for(size_t i = 0; i < dev.reg.size(); i++)
    {
        bool table = dev.reg[i].addr.size() > 1;
        if(table)
            lua_newtable(g_lua);
        else
            lua_pushnil(g_lua);

        for(size_t k = 0; k < dev.reg[i].addr.size(); k++)
        {
            my_lua_create_reg(dev.addr[index].addr, k, dev.reg[i]);
            if(table)
            {
                lua_pushinteger(g_lua, k);
                lua_pushvalue(g_lua, -2);
                lua_settable(g_lua, -4);
            }
            lua_setfield(g_lua, -3, dev.reg[i].addr[k].name.c_str());
        }

        if(table)
            lua_setfield(g_lua, -2, dev.reg[i].name.c_str());
        else
            lua_pop(g_lua, 1);
    }
}

bool my_lua_import_soc(const soc_t& soc)
{
    int oldtop = lua_gettop(g_lua);

    lua_getglobal(g_lua, "hwstub");
    lua_getfield(g_lua, -1, "soc");

    lua_newtable(g_lua);

    lua_pushstring(g_lua, soc.name.c_str());
    lua_setfield(g_lua, -2, "name");

    lua_pushstring(g_lua, soc.desc.c_str());
    lua_setfield(g_lua, -2, "desc");

    for(size_t i = 0; i < soc.dev.size(); i++)
    {
        bool table = soc.dev[i].addr.size() > 1;
        if(table)
            lua_newtable(g_lua);
        else
            lua_pushnil(g_lua);

        for(size_t k = 0; k < soc.dev[i].addr.size(); k++)
        {
            my_lua_create_dev(k, soc.dev[i]);
            if(table)
            {
                lua_pushinteger(g_lua, k + 1);
                lua_pushvalue(g_lua, -2);
                lua_settable(g_lua, -4);
            }
            lua_setfield(g_lua, -3, soc.dev[i].addr[k].name.c_str());
        }

        if(table)
            lua_setfield(g_lua, -2, soc.dev[i].name.c_str());
        else
            lua_pop(g_lua, 1);
    }

    lua_setfield(g_lua, -2, soc.name.c_str());

    lua_pop(g_lua, 2);

    if(lua_gettop(g_lua) != oldtop)
    {
        printf("internal error: unbalanced my_lua_import_soc\n");
        return false;
    }
    return true;
}

bool my_lua_import_soc(const std::vector< soc_t >& socs)
{
    for(size_t i = 0; i < socs.size(); i++)
    {
        if(!g_quiet)
            printf("importing %s...\n", socs[i].name.c_str());
        if(!my_lua_import_soc(socs[i]))
            return false;
    }
    return true;
}

/**
 * glue
 */

void usage(void)
{
    printf("hwstub_tool, compiled with hwstub protocol %d.%d\n",
        HWSTUB_VERSION_MAJOR, HWSTUB_VERSION_MINOR);
    printf("\n");
    printf("usage: hwstub_tool [options] <soc desc files>\n");
    printf("options:\n");
    printf("  --help/-?   Display this help\n");
    printf("  --quiet/-q  Quiet non-command messages\n");
    printf("  -i <init>   Set lua init file (default is init.lua)\n");
    printf("  -e <cmd>    Execute <cmd> at startup\n");
    printf("  -f <file>   Execute <file> at startup\n");
    printf("Relative order of -e and -f commands are preserved.\n");
    printf("They are executed after init file.\n");
    exit(1);
}

enum exec_type { exec_cmd, exec_file };

int main(int argc, char **argv)
{
    const char *lua_init = "init.lua";
    std::vector< std::pair< exec_type, std::string > > startup_cmds;
    // parse command line
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"quiet", no_argument, 0, 'q'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?qi:e:f:", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'q':
                g_quiet = true;
                break;
            case '?':
                usage();
                break;
            case 'i':
                lua_init = optarg;
                break;
            case 'e':
                startup_cmds.push_back(std::make_pair(exec_cmd, std::string(optarg)));
                break;
            case 'f':
                startup_cmds.push_back(std::make_pair(exec_file, std::string(optarg)));
                break;
            default:
                abort();
        }
    }

    // load register descriptions
    std::vector< soc_t > socs;
    for(int i = optind; i < argc; i++)
    {
        socs.push_back(soc_t());
        if(!soc_desc_parse_xml(argv[i], socs[socs.size() - 1]))
        {
            printf("Cannot load description '%s'\n", argv[i]);
            return 2;
        }
    }

    // create usb context
    libusb_context *ctx;
    libusb_init(&ctx);
    libusb_set_debug(ctx, 3);

    // look for device
    if(!g_quiet)
        printf("Looking for hwstub device ...\n");
    // open first device
    libusb_device **list;
    ssize_t cnt = hwstub_get_device_list(ctx, &list);
    if(cnt <= 0)
    {
        printf("No device found\n");
        return 1;
    }
    libusb_device_handle *handle;
    if(libusb_open(list[0], &handle) != 0)
    {
        printf("Cannot open device\n");
        return 1;
    }
    libusb_free_device_list(list, 1);

    // admin stuff
    libusb_device *mydev = libusb_get_device(handle);
    if(!g_quiet)
    {
        printf("device found at %d:%d\n",
            libusb_get_bus_number(mydev),
            libusb_get_device_address(mydev));
    }
    g_hwdev = hwstub_open(handle);
    if(g_hwdev == NULL)
    {
        printf("Cannot open device!\n");
        return 1;
    }

    // get hwstub information
    int ret = hwstub_get_desc(g_hwdev, HWSTUB_DT_VERSION, &g_hwdev_ver, sizeof(g_hwdev_ver));
    if(ret != sizeof(g_hwdev_ver))
    {
        printf("Cannot get version!\n");
        goto Lerr;
    }
    if(g_hwdev_ver.bMajor != HWSTUB_VERSION_MAJOR || g_hwdev_ver.bMinor < HWSTUB_VERSION_MINOR)
    {
        printf("Warning: this tool is possibly incompatible with your device:\n");
        printf("Device version: %d.%d.%d\n", g_hwdev_ver.bMajor, g_hwdev_ver.bMinor, g_hwdev_ver.bRevision);
        printf("Host version: %d.%d\n", HWSTUB_VERSION_MAJOR, HWSTUB_VERSION_MINOR);
    }

    // get memory layout information
    ret = hwstub_get_desc(g_hwdev, HWSTUB_DT_LAYOUT, &g_hwdev_layout, sizeof(g_hwdev_layout));
    if(ret != sizeof(g_hwdev_layout))
    {
        printf("Cannot get layout: %d\n", ret);
        goto Lerr;
    }

    // get target
    ret = hwstub_get_desc(g_hwdev, HWSTUB_DT_TARGET, &g_hwdev_target, sizeof(g_hwdev_target));
    if(ret != sizeof(g_hwdev_target))
    {
        printf("Cannot get target: %d\n", ret);
        goto Lerr;
    }

    // get STMP specific information
    if(g_hwdev_target.dID == HWSTUB_TARGET_STMP)
    {
        ret = hwstub_get_desc(g_hwdev, HWSTUB_DT_STMP, &g_hwdev_stmp, sizeof(g_hwdev_stmp));
        if(ret != sizeof(g_hwdev_stmp))
        {
            printf("Cannot get stmp: %d\n", ret);
            goto Lerr;
        }
    }

    // get PP specific information
    if(g_hwdev_target.dID == HWSTUB_TARGET_PP)
    {
        ret = hwstub_get_desc(g_hwdev, HWSTUB_DT_PP, &g_hwdev_pp, sizeof(g_hwdev_pp));
        if(ret != sizeof(g_hwdev_pp))
        {
            printf("Cannot get pp: %d\n", ret);
            goto Lerr;
        }
    }
    /** Init lua */

    // create lua state
    g_lua = luaL_newstate();
    if(g_lua == NULL)
    {
        printf("Cannot create lua state\n");
        return 1;
    }
    // import hwstub
    if(!my_lua_import_hwstub())
        printf("Cannot import hwstub description into Lua context\n");
    // open all standard libraires
    luaL_openlibs(g_lua);
    // import socs
    if(!my_lua_import_soc(socs))
        printf("Cannot import SoC descriptions into Lua context\n");

    if(luaL_dofile(g_lua, lua_init))
        printf("error in init: %s\n", lua_tostring(g_lua, -1));
    lua_pop(g_lua, lua_gettop(g_lua));

    /** start interactive mode */
    if(!g_quiet)
        printf("Starting interactive lua session. Type 'help()' to get some help\n");

    /** run startup commands */
    for(size_t i = 0; i < startup_cmds.size(); i++)
    {
        bool ret = false;
        if(!g_quiet)
            printf("Running '%s'...\n", startup_cmds[i].second.c_str());
        if(startup_cmds[i].first == exec_file)
            ret = luaL_dofile(g_lua, startup_cmds[i].second.c_str());
        else if(startup_cmds[i].first == exec_cmd)
            ret = luaL_dostring(g_lua, startup_cmds[i].second.c_str());
        if(ret)
            printf("error: %s\n", lua_tostring(g_lua, -1));
    }

    // use readline to provide some history and completion
    rl_bind_key('\t', rl_complete);
    while(!g_exit)
    {
        char *input = readline("> ");
        if(!input)
            break;
        add_history(input);
        // evaluate string
        if(luaL_dostring(g_lua, input))
            printf("error: %s\n", lua_tostring(g_lua, -1));
        // pop everything to start from a clean stack
        lua_pop(g_lua, lua_gettop(g_lua));
        free(input);
    }

    Lerr:
    // display log if handled
    if(!g_quiet)
        printf("Device log:\n");
    print_log(g_hwdev);
    hwstub_release(g_hwdev);
    return 1;
}
