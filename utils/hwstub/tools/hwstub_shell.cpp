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
#include "soc_desc_v1.hpp"
#include "soc_desc.hpp"
extern "C" {
#include "prompt.h"
}

using namespace soc_desc_v1;

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
 * debug
 */

void print_context(const std::string& file, const soc_desc::error_context_t& ctx)
{
    for(size_t j = 0; j < ctx.count(); j++)
    {
        soc_desc::error_t e = ctx.get(j);
        switch(e.level())
        {
            case soc_desc::error_t::INFO: printf("[INFO]"); break;
            case soc_desc::error_t::WARNING: printf("[WARN]"); break;
            case soc_desc::error_t::FATAL: printf("[FATAL]"); break;
            default: printf("[UNK]"); break;
        }
        if(e.location().size() != 0)
            printf(" (%s) %s:", file.c_str(), e.location().c_str());
        printf(" %s\n", e.message().c_str());
    }
}

void my_lua_print_stack(lua_State *state = 0, int up_to = 0)
{
    if(state == 0)
        state = g_lua;
    up_to = lua_gettop(state) - up_to;
    printf("stack:");
    for(int i = -1; i >= -up_to; i--)
    {
        if(lua_isstring(state, i))
            printf(" <%s>", lua_tostring(state, i));
        else
            printf(" [%s]", lua_typename(state, lua_type(state, i)));
    }
    printf("\n");
}

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
    char op = lua_tounsigned(state, lua_upvalueindex(5));

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

    soc_word_t old_value = hw_read32(state, addr);
    if(op == 'w')
        value = value << shift | (old_value & ~(mask << shift));
    else if(op == 's')
        value = old_value | value << shift;
    else if(op == 'c')
        value = old_value & ~(value << shift);
    else if(op == 't')
        value = old_value ^ (value << shift);
    else
        luaL_error(state, "write_field() internal error");

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

/* lua stack on entry/exit: <reg table> */
void my_lua_create_field(soc_addr_t addr, soc_desc::field_ref_t field)
{
    soc_desc::field_t *f = field.get();
    /** create field table */
    lua_newtable(g_lua);
    /* lua stack: <field table> <reg table> */

    /** create various characteristics */
    lua_pushstring(g_lua, f->name.c_str());
    /* lua stack: <name> <field table> ... */
    lua_setfield(g_lua, -2, "name");
    /* lua stack: <field table> ... */

    lua_pushunsigned(g_lua, addr);
    /* lua stack: <addr> <field table> ... */
    lua_setfield(g_lua, -2, "addr");
    /* lua stack: <field table> ... */

    lua_pushunsigned(g_lua, f->pos);
    /* lua stack: <pos> <field table> ... */
    lua_setfield(g_lua, -2, "pos");
    /* lua stack: <field table> ... */

    lua_pushunsigned(g_lua, f->width);
    /* lua stack: <width> <field table> ... */
    lua_setfield(g_lua, -2, "width");
    /* lua stack: <field table> ... */

    lua_pushunsigned(g_lua, f->bitmask());
    /* lua stack: <bm> <field table> ... */
    lua_setfield(g_lua, -2, "bitmask");
    /* lua stack: <field table> ... */

    soc_word_t local_bitmask = f->bitmask() >> f->pos;
    lua_pushunsigned(g_lua, local_bitmask);
    /* lua stack: <local_bm> <field table> ... */
    lua_setfield(g_lua, -2, "local_bitmask");
    /* lua stack: <field table> ... */

    /** create read routine */
    lua_pushunsigned(g_lua, addr);
    lua_pushunsigned(g_lua, f->pos);
    lua_pushunsigned(g_lua, local_bitmask);
    /* lua stack: <local_bm> <pos> <addr> <field table> ... */
    lua_pushcclosure(g_lua, my_lua_read_field, 3);
    /* lua stack: <my_lua_read_field> <field table> ... */
    lua_setfield(g_lua, -2, "read");
    /* lua stack: <field table> ... */

    /** create write/set/clr/tog routines */
    static const char *name[] = {"write", "set", "clr", "tog"};
    static const char arg[] = {'w', 's', 'c', 't'};
    for(int i = 0; i < 4; i++)
    {
        lua_pushunsigned(g_lua, addr);
        lua_pushunsigned(g_lua, f->pos);
        lua_pushunsigned(g_lua, local_bitmask);
        /* lua stack: <local_bm> <pos> <addr> <field table> ... */
        lua_pushvalue(g_lua, -4);
        /* lua stack: <field table> <local_bm> <pos> <addr> <field table> ... */
        lua_pushunsigned(g_lua, arg[i]);
        /* lua stack: <'wsct'> <field table> <local_bm> <pos> <addr> <field table> ... */
        lua_pushcclosure(g_lua, my_lua_write_field, 5);
        /* lua stack: <my_lua_write_field> <field table> ... */
        lua_setfield(g_lua, -2, name[i]);
        /* lua stack: <field table> ... */
    }

    /** create values */
    for(size_t i = 0; i < f->enum_.size(); i++)
    {
        lua_pushunsigned(g_lua, f->enum_[i].value);
        /* lua stack: <value> <field table> ... */
        lua_setfield(g_lua, -2, f->enum_[i].name.c_str());
        /* lua stack: <field table> ... */
    }

    /** register field */
    lua_setfield(g_lua, -2, f->name.c_str());
    /* lua stack: <reg table> */
}

/* lua stack on entry/exit: <inst table> */
void my_lua_create_reg(soc_addr_t addr, soc_desc::register_ref_t reg)
{
    if(!reg.valid())
        return;
    /** create read/write routine */
    lua_pushunsigned(g_lua, addr);
    /* lua stack: <addr> <inst table> */
    lua_pushcclosure(g_lua, my_lua_read_reg, 1);
    /* lua stack: <my_lua_read_reg> <inst table> */
    lua_setfield(g_lua, -2, "read");
    /* lua stack: <inst table> */

    lua_pushunsigned(g_lua, addr);
    /* lua stack: <addr> <inst table> */
    lua_pushcclosure(g_lua, my_lua_write_reg, 1);
    /* lua stack: <my_lua_read_reg> <inst table> */
    lua_setfield(g_lua, -2, "write");
    /* lua stack: <inst table> */

    /** create set/clr/tog helpers */
    static const char *name[] = {"set", "clr", "tog"};
    static const char arg[] = {'s', 'c', 't'};
    for(int i = 0; i < 3; i++)
    {
        lua_pushunsigned(g_lua, addr);
        /* lua stack: <addr> <inst table> */
        lua_pushunsigned(g_lua, arg[i]);
        /* lua stack: <'s'/'c'/'t'> <addr> <inst table> */
        lua_pushcclosure(g_lua, my_lua_sct_reg, 2);
        /* lua stack: <my_lua_sct_reg> <inst table> */
        lua_setfield(g_lua, -2, name[i]);
        /* lua stack: <inst table> */
    }

    /** create fields */
    std::vector< soc_desc::field_ref_t > fields = reg.fields();
    for(size_t i = 0; i < fields.size(); i++)
        my_lua_create_field(addr, fields[i]);
}

/* lua stack on entry/exit: <parent table> */
void my_lua_create_instances(const std::vector< soc_desc::node_inst_t >& inst)
{
    for(size_t i = 0; i < inst.size(); i++)
    {
        /** if the instance is indexed, find the instance table, otherwise create it */
        if(inst[i].is_indexed())
        {
            /** try to get the instance table, otherwise create it */
            lua_getfield(g_lua, -1, inst[i].name().c_str());
            /* lua stack: <index table> <parent table> */
            if(lua_isnil(g_lua, -1))
            {
                lua_pop(g_lua, 1);
                lua_newtable(g_lua);
                /* lua stack: <index table> <parent table> */
                lua_pushvalue(g_lua, -1);
                /* lua stack: <index table> <index table> <parent table> */
                lua_setfield(g_lua, -3, inst[i].name().c_str());
                /* lua stack: <index table> <parent table> */
            }
            lua_pushinteger(g_lua, inst[i].index());
            /* lua stack: <index> <index table> <parent table> */
        }

        /** create a new table for the instance */
        lua_newtable(g_lua);
        /* lua stack: <instance table> [<index> <index table>] <parent table> */

        /** create name and desc fields */
        lua_pushstring(g_lua, inst[i].node().get()->name.c_str());
        /* lua stack: <node name> <instance table> ... */
        lua_setfield(g_lua, -2, "name");
        /* lua stack: <instance table> ... */

        lua_pushstring(g_lua, inst[i].node().get()->desc.c_str());
        /* lua stack: <node desc> <instance table> ... */
        lua_setfield(g_lua, -2, "desc");
        /* lua stack: <instance table> ... */

        lua_pushstring(g_lua, inst[i].node().get()->title.c_str());
        /* lua stack: <node title> <instance table> ... */
        lua_setfield(g_lua, -2, "title");
        /* lua stack: <instance table> ... */

        lua_pushunsigned(g_lua, inst[i].addr());
        /* lua stack: <node addr> <instance table> ... */
        lua_setfield(g_lua, -2, "addr");
        /* lua stack: <instance table> ... */

        /** create register */
        my_lua_create_reg(inst[i].addr(), inst[i].node().reg());

        /** create subinstances */
        my_lua_create_instances(inst[i].children());
        /* lua stack: <instance table> [<index> <index table>] <parent table> */

        if(inst[i].is_indexed())
        {
            /* lua stack: <instance table> <index> <index table> <parent table> */
            lua_settable(g_lua, -3);
            /* lua stack: <index table> <parent table> */
            lua_pop(g_lua, 1);
        }
        else
        {
            /* lua stack: <instance table> <parent table> */
            lua_setfield(g_lua, -2, inst[i].name().c_str());
        }
        /* lua stack: <parent table> */
    }
}

bool my_lua_import_soc(soc_desc::soc_t& soc)
{
    /** remember old stack index to check for unbalanced stack at the end */
    int oldtop = lua_gettop(g_lua);

    /** find hwstub.soc table */
    lua_getglobal(g_lua, "hwstub");
    /* lua stack: <hwstub table> */
    lua_getfield(g_lua, -1, "soc");
    /* lua stack: <hwstub.soc table> <hwstub table> */

    /** create a new table for the soc */
    lua_newtable(g_lua);
    /* lua stack: <soc table> <hwstub.soc table> <hwstub table> */

    /** create name and desc fields */
    lua_pushstring(g_lua, soc.name.c_str());
    /* lua stack: <soc name> <soc table> <hwstub.soc table> <hwstub table> */
    lua_setfield(g_lua, -2, "name");
    /* lua stack: <soc table> <hwstub.soc table> <hwstub table> */

    lua_pushstring(g_lua, soc.desc.c_str());
    /* lua stack: <soc desc> <soc table> <hwstub.soc table> <hwstub table> */
    lua_setfield(g_lua, -2, "desc");
    /* lua stack: <soc table> <hwstub.soc table> <hwstub table> */

    /** create instances */
    soc_desc::soc_ref_t rsoc(&soc);
    my_lua_create_instances(rsoc.root_inst().children());
    /* lua stack: <soc table> <hwstub.soc table> <hwstub table> */

    /** put soc table at hwstub.soc.<soc name> */
    lua_setfield(g_lua, -2, soc.name.c_str());
    /* lua stack: <hwstub.soc table> <hwstub table> */

    lua_pop(g_lua, 2);
    /* lua stack: <> */

    if(lua_gettop(g_lua) != oldtop)
    {
        printf("internal error: unbalanced my_lua_import_soc\n");
        return false;
    }
    return true;
}

bool my_lua_import_soc(std::vector< soc_desc::soc_t >& socs)
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
    std::vector< soc_desc::soc_t > socs;
    for(int i = optind; i < argc; i++)
    {
        socs.push_back(soc_desc::soc_t());
        soc_desc::error_context_t ctx;
        if(!soc_desc::parse_xml(argv[i], socs[socs.size() - 1], ctx))
        {
            printf("Cannot load description file '%s'\n", argv[i]);
            socs.pop_back();
        }
        print_context(argv[i], ctx);
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

    // start interactive shell
    luap_enter(g_lua, &g_exit);

    Lerr:
    // display log if handled
    if(!g_quiet)
        printf("Device log:\n");
    print_log(g_hwdev);
    hwstub_release(g_hwdev);
    return 1;
}
