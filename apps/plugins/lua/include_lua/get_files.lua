--[[
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 William Wilgus
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

--------------------------------------------------------------------------------
--[[ returns a sorted tables of directories and (another) of files
-- path is the starting path; recurse == false.. only that path will be searched
-- findfile & finddir are definable search functions
-- if not defined all files/dirs are returned if false is passed.. none
    or you can provide your own function see below..
-- sort_by can be by "name" "size" "date" or "none" to perform no sorting
    note: for "size" and "date" you may need to strip the attribute data to use
    the returned filename
-- cancel_fn if not defined or not a function no user cancel otherwise supply
    your own cancel function which returns true to cancel searching for files
-- f_t and d_t allow you to pass your own tables for re-use but isn't necessary
]]
local function get_files(path, recurse, finddir, findfile, sort_by, cancel_fn, f_t, d_t)
    local quit = false
    local sort_by_function = nil
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

    if type(cancel_fn) ~= "function" then
        cancel_fn = function() return false end
    end

    local function _get_files(path)
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

            if cancel_fn() == true then
                return true
            end
        end
        return false;
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
    else -- "none"
        filepath_function = function(path, sep, fname, fattrib, fsize, ftime)
                return string.format("%s%s%s;", path, sep, fname)
        end
    end

    table.insert(dirs, path) -- root

    for key,value in pairs(dirs) do
        --luadir.dir may error out so we need to do the call protected
        -- _get_files(value, CANCEL_BUTTON)
        _, quit = pcall(_get_files, value)

        if quit == true or not recurse then
            break;
        end
    end
    if type(sort_by_function) == "function" then
        table.sort(files, cmp_alphanum)
        table.sort(dirs, cmp_alphanum)
    end

    return dirs, files
end -- get_files
--------------------------------------------------------------------------------
return get_files
