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
if ... == nil then rb.splash(rb.HZ * 3, "use 'require'") end
require("printtable")
local _lcd = require("lcd")
local _timer = require("timer")

--------------------------------------------------------------------------------
--[[ returns a sorted tables of directories and (another) of files
-- path is the starting path; norecurse == true.. only that path will be searched
--   findfile & finddir are definable search functions
--  if not defined all files/dirs are returned if false is passed.. none
-- or you can provide your own function see below..
-- f_t and d_t allow you to pass your own tables for re-use but isn't necessary
]]
local function get_files(path, norecurse, finddir, findfile, sort_by, f_t, d_t)
    local quit = false
    local sort_by_function -- forward declaration
    local filepath_function -- forward declaration
    local files = f_t or {}
    local dirs = d_t or {}

    local function f_filedir(name)
        --default find function
        -- example: return name:find(".mp3", 1, true) ~= nil
        if name:len() <= 2 and (name == "." or name == "..") then
            return false
        end
        return true
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
        local filepath
        local finfo_t
        if string.sub(path, - 1) ~= "/" then sep = "/" end
        for fname, isdir, finfo_t in luadir.dir(path, true) do
            if isdir and finddir(fname) then
                table.insert(dirs, path .. sep ..fname)
            elseif not isdir and findfile(fname) then
                filepath = filepath_function(path, sep, fname, finfo_t.attribute, finfo_t.size, finfo_t.time)
                table.insert(files, filepath)
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
                return sort_by_function(op1, op2)
            end
            return op1 < op2
        end
     end

    _lcd:splashf(1, "Searching for Files")

    if sort_by == "name" then
        sort_by_function = function(s1, s2) return s1 < s2 end
        filepath_function = function(path, sep, fname, fattrib, fsize, ftime)
                return string.format("%s%s%s;", path, sep, fname)
        end
    elseif sort_by == "size" then
        filepath_function = function(path, sep, fname, fattrib, fsize, ftime)
                return string.format("%s%s%s; At:%d, Sz:%d, Tm:%d", path, sep, fname, fattrib, fsize, ftime)
        end
        sort_by_function = function(s1, s2)
            local v1, v2
            v1 = string.match(s1, "SZ:(%d+)")
            v2 = string.match(s2, "SZ:(%d+)")
            if v1 or v2 then
                return tonumber(v1 or 0) < tonumber(v2 or 0)
            end
            return s1 < s2
        end
    elseif sort_by == "date" then
        filepath_function = function(path, sep, fname, fattrib, fsize, ftime)
                return string.format("%s%s%s; At:%d, Sz:%d, Tm:%d", path, sep, fname, fattrib, fsize, ftime)
        end
        sort_by_function = function(s1, s2)
            local v1, v2
            v1 = string.match(s1, "TM:(%d+)")
            v2 = string.match(s2, "TM:(%d+)")
            if v1 or v2 then
                return tonumber(v1 or 0) < tonumber(v2 or 0)
            end
            return s1 < s2
        end
    end

    table.insert(dirs, path) -- root

    for key,value in pairs(dirs) do
        --luadir.dir may error out so we need to do the call protected
        -- _get_files(value, CANCEL_BUTTON)
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

-- uses print_table and get_files to display simple file browser
-- sort_by "date" "name" "size"
-- descending true/false
function file_choose(dir, title, sort_by, descending)
    local dstr, hstr = ""
    if not title then
        dstr = "%d items found in %0d.%02d seconds"
    else
        hstr = title
    end

    if not sort_by then sort_by = "name" end
    sort_by = sort_by:lower()

    -- returns whole seconds and remainder
    local function tick2seconds(ticks)
        local secs  = (ticks / rb.HZ)
        local csecs = (ticks - (secs * rb.HZ))
        return secs, csecs
    end

    local norecurse  = true
    local f_finddir  = nil -- function to match directories; nil all, false none
    local f_findfile = nil -- function to match files; nil all, false none

    local p_settings = {wrap = true, hasheader = true}

    local timer
    local files = {}
    local dirs = {}
    local item = 1
    _lcd:clear()

    while item > 0 do
        if not title then
            timer = _timer()
        end

        dirs, files = get_files(dir, norecurse, f_finddir, f_findfile, sort_by, dirs, files)

        local parentdir = dirs[1]
        for i = 1, #dirs do
            dirs[i] = "\t" .. dirs[i]
        end

        if not descending then
            for i = 1, #files do
                -- only store file name .. strip attributes from end
                table.insert(dirs, "\t" .. string.match(files[i], "[^;]+") or "?")
            end
        else
            for i = #files, 1, -1 do
                -- only store file name .. strip attributes from end
                table.insert(dirs, "\t" .. string.match(files[i], "[^;]+") or "?")
            end
        end
        for i=1, #files do files[i] = nil end -- empty table for reuse

        if not title then
            hstr = string.format(dstr, #dirs - 1, tick2seconds(timer:stop()))
        end

        table.insert(dirs, 1, hstr)

        item = print_table(dirs, #dirs, p_settings)

        -- If item was selected follow directory or return filename
        if item > 0 then
            dir = string.gsub(dirs[item], "%c+","")
            if not rb.dir_exists("/" .. dir) then
                return dir
            end
        end

        if dir == parentdir then
            dir = dir:sub(1, dir:match(".*()/") - 1)
            if dir == "" then dir = "/" end
        end
        for i=1, #dirs do dirs[i] = nil end -- empty table for reuse

    end
end -- file_choose
--------------------------------------------------------------------------------
