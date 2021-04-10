--[[
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 William Wilgus
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
]]

local scrpath = rb.current_path() .. "/lua_scripts/"

package.path = scrpath .. "/?.lua;" .. package.path --add lua_scripts directory to path
require("printmenus")

rb.actions = nil
package.loaded["actions"] = nil

local excludedsrc = ";filebrowse.lua;fileviewers.lua;printmenu.lua;dbgettags.lua;"
--------------------------------------------------------------------------------
local Icon_Plugin = 0x9


local function get_files(path, norecurse, finddir, findfile, f_t, d_t)

    local quit = false

    local files = f_t or {}
    local dirs = d_t or {}

    local function f_filedir(name)
        --default find function
        -- example: return name:find(".mp3", 1, true) ~= nil
        if name:len() <= 2 and (name == "." or name == "..") then
            return false
        end
        if string.find(excludedsrc, ";" .. name .. ";") then
            return false
        end
        if string.sub(name, -4) == ".lua" then
            return true
        end
        return false
    end
    local function d_filedir(name)
        --default discard function
        return false
    end

    if finddir == nil then
        finddir = f_filedir
    elseif type(finddir) ~= "function" then
        finddir = d_filedir
    end

    if findfile == nil then
        findfile = f_filedir
    elseif type(findfile) ~= "function" then
        findfile = d_filedir
    end

    local function _get_files(path, cancelbtn)
        local sep = ""
        if string.sub(path, - 1) ~= "/" then sep = "/" end
        for fname, isdir in luadir.dir(path) do

            if isdir and finddir(fname) then
                table.insert(dirs, path .. sep ..fname)
            elseif not isdir and findfile(fname) then
                table.insert(files, path .. sep ..fname)
            end

            if  rb.get_plugin_action(0) == cancelbtn then
                return true
            end
        end
    end

    local function cmp_alphanum (op1, op2)
        local type1= type(op1)
        local type2 = type(op2)

        if type1 ~= type2 then
            return type1 < type2
        else
            if type1 == "string" then
                op1 = op1:upper()
                op2 = op2:upper()
            end
            return op1 < op2
        end
     end

    table.insert(dirs, path) -- root

    for key,value in pairs(dirs) do
        --luadir.dir may error out so we need to do the call protected
        _, quit = pcall(_get_files, value, CANCEL_BUTTON)

        if quit == true or norecurse then
            break;
        end
    end

    table.sort(files, cmp_alphanum)
    table.sort(dirs, cmp_alphanum)

    return dirs, files
end -- get_files
--------------------------------------------------------------------------------

function icon_fn(item, icon)
    if item ~= 0 then
        icon = Icon_Plugin
    else
        icon = -1
    end
    return icon
end

-- uses print_table and get_files to display simple file browser
function script_choose(dir, title)
    local dstr
    local hstr = title

    local norecurse  = true
    local f_finddir  = false -- function to match directories; nil all, false none
    local f_findfile = nil -- function to match files; nil all, false none
    local t_linedesc = {show_icons = true, icon_fn = icon_fn}
    local p_settings = {wrap = true, hasheader = true, justify = "left", linedesc = t_linedesc}
    local files = {}
    local dirs = {}
    local item = 1
    rb.lcd_clear_display()

    while item > 0 do
        dirs, files = get_files(dir, norecurse, f_finddir, f_findfile, dirs, files)
        for i=1, #dirs do dirs[i] = nil end -- empty table for reuse
        table.insert(dirs, 1, hstr)
        for i = 1, #files do
            table.insert(dirs, "\t" .. string.gsub(files[i], ".*/",""))
        end
        --print_menu(menu_t, func_t, selected, settings, copy_screen)
        _, item = print_menu(dirs, nil, 0, p_settings)

        -- If item was selected follow directory or return filename
        item = item or -1
        if item > 0 then
            dir = files[item - 1]
            if not rb.dir_exists("/" .. dir) then
                return dir
            end
        end

    end
end -- file_choose
--------------------------------------------------------------------------------

local script_path = script_choose(scrpath, "lua scripts")
if script_path then rb.restart_lua(script_path) end
