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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <readline/readline.h>
#include <readline/history.h>
#include <lua.hpp>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include "soc_desc_v1.hpp"
#include "soc_desc.hpp"
#include "hwstub.hpp"
#include "hwstub_usb.hpp"
#include "hwstub_uri.hpp"
extern "C" {
#include "prompt.h"
}

using namespace soc_desc_v1;
using namespace hwstub;

#if LUA_VERSION_NUM < 502
#warning You need at least lua 5.2
#endif

/**
 * Global variables
 */
bool g_quiet = false;
bool g_exit = false;
bool g_print_mem_rw = false;
std::shared_ptr<context> g_hwctx;
std::shared_ptr<handle> g_hwdev;
struct hwstub_version_desc_t g_hwdev_ver;
struct hwstub_layout_desc_t g_hwdev_layout;
struct hwstub_target_desc_t g_hwdev_target;
struct hwstub_stmp_desc_t g_hwdev_stmp;
struct hwstub_jz_desc_t g_hwdev_jz;
struct hwstub_pp_desc_t g_hwdev_pp;
std::vector<std::shared_ptr<device>> g_devlist;

lua_State *g_lua;

/**
 * debug
 */

void print_context(const std::string& file, const soc_desc::error_context_t& ctx)
{
    for(size_t j = 0; j < ctx.count(); j++)
    {
        soc_desc::err_t e = ctx.get(j);
        switch(e.level())
        {
            case soc_desc::err_t::INFO: printf("[INFO]"); break;
            case soc_desc::err_t::WARNING: printf("[WARN]"); break;
            case soc_desc::err_t::FATAL: printf("[FATAL]"); break;
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

void print_log(std::shared_ptr<handle> hwdev)
{
    do
    {
        char buffer[128];
        size_t length = sizeof(buffer) - 1;
        error err = hwdev->get_log(buffer, length);
        if(err != error::SUCCESS || length == 0)
            break;
        buffer[length] = 0;
        printf("%s", buffer);
    }while(1);
}

/**
 * Lua specific
 */

/* the lua_*unsigned functions were never really documented and got deprecated in
 * lua 5.3. There are simply casts anyway so reimplement them with proper typing */
static void mylua_pushunsigned(lua_State *L, lua_Unsigned n)
{
    lua_pushnumber(L, (lua_Number)n);
}

static lua_Unsigned mylua_checkunsigned(lua_State *L, int arg)
{
    return (lua_Unsigned)luaL_checknumber(L, arg);
}

static lua_Unsigned mylua_tounsigned(lua_State *L, int arg)
{
    return (lua_Unsigned)lua_tointeger(L, arg);
}

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
    size_t sz = sizeof(u);
    error ret = g_hwdev->read(addr, &u, sz, true);
    if(ret != error::SUCCESS || sz != sizeof(u))
        luaL_error(state, "fail to read8 @ %p", addr);
    if(g_print_mem_rw)
        printf("[read8 @ %#lx = %#lx]\n", (unsigned long)addr, (unsigned long)u);
    return u;
}

soc_word_t hw_read16(lua_State *state, soc_addr_t addr)
{
    uint16_t u;
    size_t sz = sizeof(u);
    error ret = g_hwdev->read(addr, &u, sz, true);
    if(ret != error::SUCCESS || sz != sizeof(u))
        luaL_error(state, "fail to read16 @ %p", addr);
    if(g_print_mem_rw)
        printf("[read16 @ %#lx = %#lx]\n", (unsigned long)addr, (unsigned long)u);
    return u;
}

soc_word_t hw_read32(lua_State *state, soc_addr_t addr)
{
    uint32_t u;
    size_t sz = sizeof(u);
    error ret = g_hwdev->read(addr, &u, sz, true);
    if(ret != error::SUCCESS || sz != sizeof(u))
        luaL_error(state, "fail to read32 @ %p", addr);
    if(g_print_mem_rw)
        printf("[read32 @ %#lx = %#lx]\n", (unsigned long)addr, (unsigned long)u);
    return u;
}

void hw_write8(lua_State *state, soc_addr_t addr, soc_word_t val)
{
    uint8_t u = val;
    size_t sz = sizeof(u);
    error ret = g_hwdev->write(addr, &u, sz, true);
    if(ret != error::SUCCESS || sz != sizeof(u))
        luaL_error(state, "fail to write8 @ %p", addr);
    if(g_print_mem_rw)
        printf("[write8 @ %#lx = %#lx]\n", (unsigned long)addr, (unsigned long)u);
}

void hw_write16(lua_State *state, soc_addr_t addr, soc_word_t val)
{
    uint16_t u = val;
    size_t sz = sizeof(u);
    error ret = g_hwdev->write(addr, &u, sz, true);
    if(ret != error::SUCCESS || sz != sizeof(u))
        luaL_error(state, "fail to write16 @ %p", addr);
    if(g_print_mem_rw)
        printf("[write16 @ %#lx = %#lx]\n", (unsigned long)addr, (unsigned long)u);
}

void hw_write32(lua_State *state, soc_addr_t addr, soc_word_t val)
{
    uint32_t u = val;
    size_t sz = sizeof(u);
    error ret = g_hwdev->write(addr, &u, sz, true);
    if(ret != error::SUCCESS || sz != sizeof(u))
        luaL_error(state, "fail to write32 @ %p", addr);
    if(g_print_mem_rw)
        printf("[write32 @ %#lx = %#lx]\n", (unsigned long)addr, (unsigned long)u);
}

int my_lua_readn(lua_State *state)
{
    hw_readn_fn_t fn = (hw_readn_fn_t)lua_touserdata(state, lua_upvalueindex(1));
    int n = lua_gettop(state);
    if(n != 1)
        luaL_error(state, "readn takes a single argument");
    mylua_pushunsigned(state, fn(state, mylua_checkunsigned(state, 1)));
    return 1;
}

int my_lua_writen(lua_State *state)
{
    hw_writen_fn_t fn = (hw_writen_fn_t)lua_touserdata(state, lua_upvalueindex(1));
    int n = lua_gettop(state);
    if(n != 2)
        luaL_error(state, "writen takes two arguments");
    fn(state, mylua_checkunsigned(state, 1), mylua_checkunsigned(state, 2));
    return 0;
}

int my_lua_call(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 1)
        luaL_error(state, "call takes one argument: target address");

    error ret = g_hwdev->exec(luaL_checkunsigned(state, 1), HWSTUB_EXEC_CALL);
    if(ret != error::SUCCESS)
        luaL_error(state, "call failed");
    return 0;
}

int my_lua_jump(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 1)
        luaL_error(state, "jump takes one argument: target address");

    error ret = g_hwdev->exec(luaL_checkunsigned(state, 1), HWSTUB_EXEC_JUMP);
    if(ret != error::SUCCESS)
        luaL_error(state, "jump failed");
    return 0;
}

int my_lua_read32_cop(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 1)
        luaL_error(state, "read32_cop takes one argument: arguments (array)");
    uint8_t args[HWSTUB_COP_ARGS] = {0};
    /* parse array */
    luaL_checktype(state, 1, LUA_TTABLE);
    size_t sz = lua_rawlen(state, 1);
    if(sz > HWSTUB_COP_ARGS)
        luaL_error(state, "coprocessor operation take at most %d arguments", HWSTUB_COP_ARGS);
    for(size_t i = 0; i < sz; i++)
    {
        lua_pushinteger(state, i + 1); /* index start at 1 */
        lua_gettable(state, 1);
        /* check it is an integer */
        args[i] = luaL_checkunsigned(state, -1);
        /* pop it */
        lua_pop(state, 1);
    }

    uint32_t val;
    error ret = g_hwdev->read32_cop(args, val);
    if(ret != error::SUCCESS)
        luaL_error(state, "coprocessor read failed");
    lua_pushunsigned(state, val);
    return 1;
}

int my_lua_write32_cop(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 2)
        luaL_error(state, "write32_cop takes two arguments: arguments (array) and value");
    uint8_t args[HWSTUB_COP_ARGS] = {0};
    /* parse array */
    luaL_checktype(state, 1, LUA_TTABLE);
    size_t sz = lua_rawlen(state, 1);
    if(sz > HWSTUB_COP_ARGS)
        luaL_error(state, "coprocessor operation take at most %d arguments", HWSTUB_COP_ARGS);
    for(size_t i = 0; i < sz; i++)
    {
        lua_pushinteger(state, i + 1); /* index start at 1 */
        lua_gettable(state, 1);
        /* check it is an integer */
        args[i] = luaL_checkunsigned(state, -1);
        /* pop it */
        lua_pop(state, 1);
    }

    error ret = g_hwdev->write32_cop(args, luaL_checkunsigned(state, 2));
    if(ret != error::SUCCESS)
        luaL_error(state, "coprocessor write failed");
    return 0;
}

int my_lua_printlog(lua_State *state)
{
    print_log(g_hwdev);
    return 0;
}

std::string get_dev_name(std::shared_ptr<device> dev)
{
    std::ostringstream name;
    usb::device *udev = dynamic_cast<usb::device*>(dev.get());
    if(udev)
    {
        name << "USB Bus " << (unsigned)udev->get_bus_number() <<
            " Device " << std::setw(2) << std::hex << (unsigned)udev->get_address() << ": ";
    }
    // try to open device
    std::shared_ptr<handle> h;
    error ret = dev->open(h);
    if(ret != error::SUCCESS)
    {
        name << "<cannot open dev>";
        return name.str();
    }
    // get target information
    hwstub_target_desc_t desc;
    ret = h->get_target_desc(desc);
    if(ret !=error::SUCCESS)
    {
        name << "<cannot get name: " << error_string(ret) << ">";
        return name.str();
    }
    name << desc.bName;
    return name.str();
}

int my_lua_get_dev_list(lua_State *state)
{
    error ret = g_hwctx->get_device_list(g_devlist);
    if(ret != error::SUCCESS)
    {
        printf("Cannot device list\n");
        return -1;
    }

    printf("=== Available device list ===\n");
    for(size_t i = 0; i < g_devlist.size(); i++)
    {
        std::string name = get_dev_name(g_devlist[i]);
        printf("%zu: %s%s\n", i, name.c_str(),
               g_devlist[i] == g_hwdev->get_device() ? " <--" : "");
    }
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

int my_lua_mdelay(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 1)
        luaL_error(state, "mdelay takes one argument");
    long usec = lua_tointeger(state, -1);
    usleep(usec * 1000);
    return 0;
}

int fetch_dev_info()
{
    // get memory layout information
    error ret = g_hwdev->get_layout_desc(g_hwdev_layout);
    if(ret != error::SUCCESS)
    {
        printf("Cannot get layout descriptor: %s\n", error_string(ret).c_str());
        goto Lerr;
    }
    // get hwstub information
    ret = g_hwdev->get_version_desc(g_hwdev_ver);
    if(ret != error::SUCCESS)
    {
        printf("Cannot get version descriptor: %s\n", error_string(ret).c_str());
        goto Lerr;
    }
    // get target
    ret = g_hwdev->get_target_desc(g_hwdev_target);
    if(ret != error::SUCCESS)
    {
        printf("Cannot get target descriptor: %s\n", error_string(ret).c_str());
        goto Lerr;
    }
    if(g_hwdev_target.dID == HWSTUB_TARGET_STMP)
        ret = g_hwdev->get_stmp_desc(g_hwdev_stmp);
    // get PP specific information
    else if(g_hwdev_target.dID == HWSTUB_TARGET_PP)
        ret = g_hwdev->get_pp_desc(g_hwdev_pp);
    // get JZ specific information
    else if(g_hwdev_target.dID == HWSTUB_TARGET_JZ)
        ret = g_hwdev->get_jz_desc(g_hwdev_jz);
    if(ret != error::SUCCESS)
    {
        printf("Cannot get soc specific descriptor: %s\n", error_string(ret).c_str());
        goto Lerr;
    }
    return 0;
Lerr:
    return -1;
}

int update_dev_lua_state(lua_State *state)
{
    // fetch info before starting to mess with the lua state
    int ret = fetch_dev_info();
    if(ret < 0)
        return ret;
    // hwstub
    lua_getglobal(state, "hwstub");
    // hwstub.dev
    lua_getfield(state, -1, "dev");

    // hwstub.dev.version
    lua_getfield(state, -1, "version");

    // hwstub.dev.version.major = g_hwdev_ver.bMajor
    lua_pushstring(state, "major");
    lua_pushinteger(state, g_hwdev_ver.bMajor);
    lua_settable(state, -3);

    // hwstub.dev.version.minor = g_hwdev_ver.bMinor
    lua_pushstring(state, "minor");
    lua_pushinteger(state, g_hwdev_ver.bMinor);
    lua_settable(state, -3);
    lua_pop(state, 1);

    // hwstub.dev.layout
    lua_getfield(state, -1, "layout");

    // hwstub.dev.layout.code
    lua_getfield(state, -1, "code");

    // hwstub.dev.layout.code.start = g_hwdev_layout.dCodeStart
    lua_pushstring(state, "start");
    lua_pushinteger(state, g_hwdev_layout.dCodeStart);
    lua_settable(state, -3);

    // hwstub.dev.layout.code.size = g_hwdev_layout.dCodeSize
    lua_pushstring(state, "size");
    lua_pushinteger(state, g_hwdev_layout.dCodeSize);
    lua_settable(state, -3);

    // hwstub.dev.layout
    lua_pop(state, 1);

    // hwstub.dev.layout.stack
    lua_getfield(state, -1, "stack");

    // hwstub.dev.layout.stack.start = g_hwdev_layout.dStackStart
    lua_pushstring(state, "start");
    lua_pushinteger(state, g_hwdev_layout.dStackStart);
    lua_settable(state, -3);

    // hwstub.dev.layout.stack.size = g_hwdev_layout.dStackSize
    lua_pushstring(state, "size");
    lua_pushinteger(state, g_hwdev_layout.dStackSize);
    lua_settable(state, -3);

    // hwstub.dev.layout
    lua_pop(state, 1);

    // hwstub.dev.layout.buffer
    lua_getfield(state, -1, "buffer");

    // hwstub.dev.layout.buffer.start = g_hwdev_layout.dBufferStart
    lua_pushstring(state, "start");
    lua_pushinteger(state, g_hwdev_layout.dBufferStart);
    lua_settable(state, -3);

    // hwstub.dev.layout.buffer.size = g_hwdev_layout.dBufferSize
    lua_pushstring(state, "size");
    lua_pushinteger(state, g_hwdev_layout.dBufferSize);
    lua_settable(state, -3);

    // hwstub.dev
    lua_pop(state, 2);

    // hwstub.dev.target
    lua_getfield(state, -1, "target");

    // hwstub.dev.target.id = g_hwdev_target.dID
    lua_pushstring(state, "id");
    lua_pushinteger(state, g_hwdev_target.dID);
    lua_settable(state, -3);

    // hwstub.dev.target.name = g_hwdev_target.bName
    lua_pushstring(state, "name");
    lua_pushstring(state, g_hwdev_target.bName);
    lua_settable(state, -3);

    // hwstub.dev
    lua_pop(state, 1);

    // get STMP specific information
    if(g_hwdev_target.dID == HWSTUB_TARGET_STMP)
    {
        // hwstub.dev.stmp
        lua_getfield(state, -1, "stmp");

        // hwstub.dev.stmp.chipid = g_hwdev_stmp.wChipID
        lua_pushstring(state, "chipid");
        lua_pushinteger(state, g_hwdev_stmp.wChipID);
        lua_settable(state, -3);

        // hwstub.dev.stmp.rev = g_hwdev_stmp.bRevision
        lua_pushstring(state, "rev");
        lua_pushinteger(state, g_hwdev_stmp.bRevision);
        lua_settable(state, -3);

        // hwstub.dev.stmp.package = g_hwdev_stmp.bPackage
        lua_pushstring(state, "package");
        lua_pushinteger(state, g_hwdev_stmp.bPackage);
        lua_settable(state, -3);

        // hwstub.dev
        lua_pop(state, 1);
    }
    // get PP specific information
    else if(g_hwdev_target.dID == HWSTUB_TARGET_PP)
    {
        // hwstub.dev.pp
        lua_getfield(state, -1, "pp");

        // hwstub.dev.pp.chipid = g_hwdev_pp.wChipID
        lua_pushstring(state, "chipid");
        lua_pushinteger(state, g_hwdev_pp.wChipID);
        lua_settable(state, -3);

        // hwstub.dev.pp.rev = g_hwdev_pp.bRevision
        lua_pushstring(state, "rev");
        lua_pushlstring(state, (const char *)g_hwdev_pp.bRevision, 2);
        lua_settable(state, -3);

        // hwstub.dev
        lua_pop(state, 1);
    }
    // get JZ specific information
    else if(g_hwdev_target.dID == HWSTUB_TARGET_JZ)
    {
        // hwstub.dev.jz
        lua_getfield(state, -1, "jz");

        // hwstub.dev.jz.chipid = g_hwdev_jz.wChipID
        lua_pushstring(state, "chipid");
        lua_pushinteger(state, g_hwdev_jz.wChipID);
        lua_settable(state, -3);

        // hwstub.dev.jz.rev = g_hwdev_jz.bRevision
        lua_pushstring(state, "rev");
        lua_pushinteger(state, g_hwdev_jz.bRevision);
        lua_settable(state, -3);

        // hwstub.dev
        lua_pop(state, 1);
    }
    // pop all globals (hwstub.dev)
    lua_pop(state, 2);
    return 0;
}

int my_lua_dev_open(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 1)
        luaL_error(state, "open_dev takes one argument");
    int32_t id = lua_tointeger(state, -1);
    if(id < 0 || (size_t)id >= g_devlist.size())
        luaL_error(state, "invalid device id");
    std::shared_ptr<handle> h;
    error ret = g_devlist[id]->open(h);
    if(ret != error::SUCCESS)
        luaL_error(state, "Cannot open device: %s. The current device was NOT changed.\n", error_string(ret).c_str());
    else
        g_hwdev = h;
    if(update_dev_lua_state(state) < 0)
        return -1;
    if(luaL_dostring(state, "init()"))
        luaL_error(state, "error in init: %s\n", lua_tostring(state, -1));
    lua_pop(state, lua_gettop(state));
    return 0;
}

int my_lua_dev_close(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 0)
        luaL_error(state, "close_dev takes no argument");
    std::shared_ptr<device> dev;
    error ret = g_hwctx->get_dummy_device(dev);
    if(ret != error::SUCCESS)
        luaL_error(state, "Cannot get dummy device: %s. The current device was NOT closed.\n", error_string(ret).c_str());
    std::shared_ptr<handle> h;
    ret = dev->open(h);
    if(ret != error::SUCCESS)
        luaL_error(state, "Cannot open dummy device: %s. The current device was NOT changed.\n", error_string(ret).c_str());
    else
        g_hwdev = h;
    return update_dev_lua_state(state);
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
    lua_pushinteger(g_lua, HWSTUB_TARGET_JZ);
    lua_setfield(g_lua, -2, "JZ");
    lua_setfield(g_lua, -2, "target");

    lua_newtable(g_lua); // stmp
    lua_pushinteger(g_lua, 0);
    lua_setfield(g_lua, -2, "chipid");
    lua_pushinteger(g_lua, 0);
    lua_setfield(g_lua, -2, "rev");
    lua_pushinteger(g_lua, 0);
    lua_setfield(g_lua, -2, "package");
    lua_setfield(g_lua, -2, "stmp");

    lua_newtable(g_lua); // pp
    lua_pushinteger(g_lua, 0);
    lua_setfield(g_lua, -2, "chipid");
    lua_pushstring(g_lua, "");
    lua_setfield(g_lua, -2, "rev");
    lua_setfield(g_lua, -2, "pp");

    lua_newtable(g_lua); // jz
    lua_pushinteger(g_lua, 0);
    lua_setfield(g_lua, -2, "chipid");
    lua_setfield(g_lua, -2, "jz");

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
    lua_pushcclosure(g_lua, my_lua_read32_cop, 0);
    lua_setfield(g_lua, -2, "read32_cop");
    lua_pushcclosure(g_lua, my_lua_write32_cop, 0);
    lua_setfield(g_lua, -2, "write32_cop");

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
    lua_pushstring(g_lua, "This is the help for tool. This tools uses Lua to interpret commands.");
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

    lua_pushcclosure(g_lua, my_lua_mdelay, 0);
    lua_setfield(g_lua, -2, "mdelay");

    lua_pushcfunction(g_lua, my_lua_get_dev_list);
    lua_setfield(g_lua, -2, "get_dev_list");

    lua_pushcclosure(g_lua, my_lua_dev_open, 0);
    lua_setfield(g_lua, -2, "open_dev");

    lua_pushcclosure(g_lua, my_lua_dev_close, 0);
    lua_setfield(g_lua, -2, "close_dev");

    lua_setglobal(g_lua, "hwstub");

    lua_pushcfunction(g_lua, my_lua_help);
    lua_setglobal(g_lua, "help");

    lua_pushcfunction(g_lua, my_lua_quit);
    lua_setglobal(g_lua, "exit");

    lua_pushcfunction(g_lua, my_lua_quit);
    lua_setglobal(g_lua, "quit");

    update_dev_lua_state(g_lua);

    if(lua_gettop(g_lua) != oldtop)
    {
        printf("internal error: unbalanced my_lua_import_soc\n");
        return false;
    }
    return true;
}

soc_word_t hw_readn(lua_State *state, unsigned width, soc_word_t addr)
{
    switch(width)
    {
        case 8: return hw_read8(state, addr); break;
        case 16: return hw_read16(state, addr); break;
        case 32: return hw_read32(state, addr); break;
        default: luaL_error(state, "read() has invalid width"); return 0xdeadbeef;
    }
}

void hw_writen(lua_State *state, unsigned width, soc_word_t addr, soc_word_t val)
{
    switch(width)
    {
        case 8: hw_write8(state, addr, val); break;
        case 16: hw_write16(state, addr, val); break;
        case 32: hw_write32(state, addr, val); break;
        default: luaL_error(state, "write() has invalid width");
    }
}

int my_lua_read_reg(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 0)
        luaL_error(state, "read() takes no argument");
    unsigned width = mylua_tounsigned(state, lua_upvalueindex(1));
    soc_addr_t addr = mylua_tounsigned(state, lua_upvalueindex(2));
    mylua_pushunsigned(state, hw_readn(state, width, addr));
    return 1;
}

int my_lua_write_reg(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 1)
        luaL_error(state, "write() takes one argument");
    soc_word_t val = mylua_checkunsigned(state, 1);
    unsigned width = mylua_tounsigned(state, lua_upvalueindex(1));
    soc_addr_t addr = mylua_tounsigned(state, lua_upvalueindex(2));
    hw_writen(state, width, addr, val);
    return 0;
}

int my_lua_read_field(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 0)
        luaL_error(state, "read() takes no argument");
    unsigned width = mylua_tounsigned(state, lua_upvalueindex(1));
    soc_addr_t addr = mylua_tounsigned(state, lua_upvalueindex(2));
    soc_word_t shift = mylua_tounsigned(state, lua_upvalueindex(3));
    soc_word_t mask = mylua_tounsigned(state, lua_upvalueindex(4));
    mylua_pushunsigned(state, (hw_readn(state, width, addr) >> shift) & mask);
    return 1;
}

int my_lua_write_field(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 0 && n!= 1)
        luaL_error(state, "write() takes one or no argument");
    unsigned width = mylua_tounsigned(state, lua_upvalueindex(1));
    soc_addr_t addr = mylua_tounsigned(state, lua_upvalueindex(2));
    soc_word_t shift = mylua_tounsigned(state, lua_upvalueindex(3));
    soc_word_t mask = mylua_tounsigned(state, lua_upvalueindex(4));
    char op = mylua_tounsigned(state, lua_upvalueindex(6));

    soc_word_t value = mask;
    if(n == 1)
    {
        if(!lua_isnumber(state, 1) && lua_isstring(state, 1))
        {
            lua_pushvalue(state, lua_upvalueindex(5));
            lua_pushvalue(state, 1);
            lua_gettable(state, -2);
            if(lua_isnil(state, -1))
                luaL_error(state, "field has no value %s", lua_tostring(state, 1));
            value = mylua_checkunsigned(state, -1);
            lua_pop(state, 2);
        }
        else
            value = mylua_checkunsigned(state, 1);
        value &= mask;
    }

    soc_word_t old_value = hw_readn(state, width, addr);
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

    hw_writen(state, width, addr, value);
    return 0;
}

int my_lua_sct_reg(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 1)
        luaL_error(state, "sct() takes one argument");
    unsigned width = mylua_tounsigned(state, lua_upvalueindex(1));
    soc_addr_t addr = mylua_tounsigned(state, lua_upvalueindex(2));
    char op = mylua_tounsigned(state, lua_upvalueindex(3));

    soc_word_t mask = mylua_checkunsigned(state, 1);
    soc_word_t value = hw_read32(state, addr);
    if(op == 's')
        value |= mask;
    else if(op == 'c')
        value &= ~mask;
    else if(op == 't')
        value ^= mask;
    else
        luaL_error(state, "sct() internal error");

    hw_writen(state, width, addr, value);
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

    mylua_pushunsigned(g_lua, addr);
    /* lua stack: <addr> <field table> ... */
    lua_setfield(g_lua, -2, "addr");
    /* lua stack: <field table> ... */

    mylua_pushunsigned(g_lua, f->pos);
    /* lua stack: <pos> <field table> ... */
    lua_setfield(g_lua, -2, "pos");
    /* lua stack: <field table> ... */

    mylua_pushunsigned(g_lua, f->width);
    /* lua stack: <width> <field table> ... */
    lua_setfield(g_lua, -2, "width");
    /* lua stack: <field table> ... */

    mylua_pushunsigned(g_lua, f->bitmask());
    /* lua stack: <bm> <field table> ... */
    lua_setfield(g_lua, -2, "bitmask");
    /* lua stack: <field table> ... */

    soc_word_t local_bitmask = f->bitmask() >> f->pos;
    mylua_pushunsigned(g_lua, local_bitmask);
    /* lua stack: <local_bm> <field table> ... */
    lua_setfield(g_lua, -2, "local_bitmask");
    /* lua stack: <field table> ... */

    /** create read routine */
    mylua_pushunsigned(g_lua, field.reg().get()->width);
    mylua_pushunsigned(g_lua, addr);
    mylua_pushunsigned(g_lua, f->pos);
    mylua_pushunsigned(g_lua, local_bitmask);
    /* lua stack: <local_bm> <pos> <addr> <width> <field table> ... */
    lua_pushcclosure(g_lua, my_lua_read_field, 4);
    /* lua stack: <my_lua_read_field> <field table> ... */
    lua_setfield(g_lua, -2, "read");
    /* lua stack: <field table> ... */

    /** create write/set/clr/tog routines */
    static const char *name[] = {"write", "set", "clr", "tog"};
    static const char arg[] = {'w', 's', 'c', 't'};
    for(int i = 0; i < 4; i++)
    {
        mylua_pushunsigned(g_lua, field.reg().get()->width);
        mylua_pushunsigned(g_lua, addr);
        mylua_pushunsigned(g_lua, f->pos);
        mylua_pushunsigned(g_lua, local_bitmask);
        /* lua stack: <local_bm> <pos> <addr> <width> <field table> ... */
        lua_pushvalue(g_lua, -5);
        /* lua stack: <field table> <local_bm> <pos> <addr> <width> <field table> ... */
        mylua_pushunsigned(g_lua, arg[i]);
        /* lua stack: <'wsct'> <field table> <local_bm> <pos> <addr> <width> <field table> ... */
        lua_pushcclosure(g_lua, my_lua_write_field, 6);
        /* lua stack: <my_lua_write_field> <field table> ... */
        lua_setfield(g_lua, -2, name[i]);
        /* lua stack: <field table> ... */
    }

    /** create values */
    for(size_t i = 0; i < f->enum_.size(); i++)
    {
        mylua_pushunsigned(g_lua, f->enum_[i].value);
        /* lua stack: <value> <field table> ... */
        lua_setfield(g_lua, -2, f->enum_[i].name.c_str());
        /* lua stack: <field table> ... */
    }

    /** register field */
    lua_setfield(g_lua, -2, f->name.c_str());
    /* lua stack: <reg table> */
}

/* lua stack on entry/exit: <inst table> */
void my_lua_create_reg(soc_addr_t addr, soc_desc::register_ref_t reg,
    bool in_variant = false)
{
    if(!reg.valid())
        return;
    /** create read/write routine */
    mylua_pushunsigned(g_lua, reg.get()->width);
    mylua_pushunsigned(g_lua, addr);
    /* lua stack: <addr> <width> <inst table> */
    lua_pushcclosure(g_lua, my_lua_read_reg, 2);
    /* lua stack: <my_lua_read_reg> <inst table> */
    lua_setfield(g_lua, -2, "read");
    /* lua stack: <inst table> */

    mylua_pushunsigned(g_lua, reg.get()->width);
    mylua_pushunsigned(g_lua, addr);
    /* lua stack: <addr> <width> <inst table> */
    lua_pushcclosure(g_lua, my_lua_write_reg, 2);
    /* lua stack: <my_lua_write_reg> <inst table> */
    lua_setfield(g_lua, -2, "write");
    /* lua stack: <inst table> */

    /** create set/clr/tog helpers */
    static const char *name[] = {"set", "clr", "tog"};
    static const char arg[] = {'s', 'c', 't'};
    for(int i = 0; i < 3; i++)
    {
        mylua_pushunsigned(g_lua, reg.get()->width);
        mylua_pushunsigned(g_lua, addr);
        /* lua stack: <addr> <width> <inst table> */
        mylua_pushunsigned(g_lua, arg[i]);
        /* lua stack: <'s'/'c'/'t'> <addr> <width> <inst table> */
        lua_pushcclosure(g_lua, my_lua_sct_reg, 3);
        /* lua stack: <my_lua_sct_reg> <inst table> */
        lua_setfield(g_lua, -2, name[i]);
        /* lua stack: <inst table> */
    }

    /** create variants */
    if(!in_variant)
    {
        for(auto v : reg.variants())
        {
            /** create a new table */
            lua_newtable(g_lua);
            /* lua stack: <instance table> <parent table> */
            /** create register */
            my_lua_create_reg(addr + v.offset(), reg, true);
            /** register table */
            /* lua stack: <instance table> <parent table> */
            std::string varname = v.type();
            for(size_t i = 0; i < varname.size(); i++)
                varname[i] = toupper(varname[i]);
            lua_setfield(g_lua, -2, varname.c_str());
        }
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

        mylua_pushunsigned(g_lua, inst[i].addr());
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
    printf("shell, compiled with hwstub protocol %d.%d\n",
        HWSTUB_VERSION_MAJOR, HWSTUB_VERSION_MINOR);
    printf("\n");
    printf("usage: hwstub_shell [options] <soc desc files>\n");
    printf("options:\n");
    printf("  --help/-?       Display this help\n");
    printf("  --quiet/-q      Quiet non-command messages\n");
    printf("  -b/--batch      Disable interactive mode after running commands and files\n");
    printf("  --verbose/-v    Verbose output\n");
    printf("  -i <init>       Set lua init file (default is init.lua)\n");
    printf("  -e <cmd>        Execute <cmd> at startup\n");
    printf("  -f <file>       Execute <file> at startup\n");
    printf("  --dev/-d <uri>  Device URI\n");
    printf("  --debug-rw      Print read/write\n");
    printf("Relative order of -e and -f commands are preserved.\n");
    printf("Unless specified, the shell will load soc desc in regtools/desc/\n");
    //usage_uri(stdout);
    exit(1);
}

enum exec_type { exec_cmd, exec_file };

void do_signal(int sig)
{
    g_exit = true;
}

void load_std_desc(std::vector< soc_desc::soc_t >& socs)
{
    std::string path = "../../regtools/desc/";
    DIR *dir = opendir(path.c_str());
    if(dir == nullptr)
        return;
    struct dirent *entry;
    while((entry = readdir(dir)) != nullptr)
    {
        /* only match regs-*.xml files but not regs-*-v1.xml */
        std::string file = entry->d_name;
        if(file.size() < 5 || file.substr(0, 5) != "regs-")
            continue;
        if(file.substr(file.size() - 4) != ".xml")
            continue;
        if(file.substr(file.size() - 7) == "-v1.xml")
            continue;
        /* prepend path */
        file = path + "/" + file;
        socs.push_back(soc_desc::soc_t());
        soc_desc::error_context_t ctx;
        if(!soc_desc::parse_xml(file, socs[socs.size() - 1], ctx))
        {
            printf("Cannot load description file '%s'\n", file.c_str());
            socs.pop_back();
        }
        if(!g_quiet)
            print_context(file, ctx);
    }
    closedir(dir);
}

int main(int argc, char **argv)
{
    std::string dev_uri = hwstub::uri::default_uri().full_uri();
    bool verbose = false;
    bool batch = false;

    const char *lua_init = "init.lua";
    std::vector< std::pair< exec_type, std::string > > startup_cmds;
    // parse command line
    while(1)
    {
        enum { OPT_DBG_RW = 256 };
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"quiet", no_argument, 0, 'q'},
            {"init", required_argument, 0, 'i'},
            {"startcmd", required_argument, 0, 'e'},
            {"startfile", required_argument, 0, 'f'},
            {"dev", required_argument, 0, 'd'},
            {"verbose", no_argument, 0, 'v'},
            {"batch", no_argument, 0, 'b'},
            {"debug-rw", no_argument, 0, OPT_DBG_RW},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?qi:e:f:d:vb", long_options, NULL);
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
            case 'd':
                dev_uri = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            case OPT_DBG_RW:
                g_print_mem_rw = true;
                break;
            case 'b':
                batch = true;
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
        if(!g_quiet)
            print_context(argv[i], ctx);
    }
    /* load standard desc files */
    load_std_desc(socs);

    /* create usb context */
    std::string errstr;
    g_hwctx = uri::create_context(uri::uri(dev_uri), &errstr);
    if(!g_hwctx)
    {
        printf("Cannot create context: %s\n", errstr.c_str());
        return 1;
    }
    if(verbose)
        g_hwctx->set_debug(std::cout);
    std::vector<std::shared_ptr<device>> list;
    error ret = g_hwctx->get_device_list(list);
    if(ret != error::SUCCESS)
    {
        printf("Cannot get device list: %s\n", error_string(ret).c_str());
        return 1;
    }
    if(list.size() == 0)
    {
        printf("No hwstub device detected!\n");
        return 1;
    }
    /* open first device */
    ret = list[0]->open(g_hwdev);
    if(ret != error::SUCCESS)
    {
        printf("Cannot open device: %s\n", error_string(ret).c_str());
        return 1;
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
    {
        printf("Starting interactive lua session. Type 'help()' to get some help\n");
        luaL_dostring(g_lua, "hwstub.info()");
    }

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

    /* intercept CTRL+C */
    signal(SIGINT, do_signal);
    // start interactive shell
    if(!batch)
        luap_enter(g_lua, &g_exit);
    // cleanup
    lua_close(g_lua);

    // display log if handled
    if(!g_quiet)
        printf("Device log:\n");
    print_log(g_hwdev);
    return 1;
}
