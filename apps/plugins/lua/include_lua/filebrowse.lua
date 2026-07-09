--[[
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
local tmploader = require("temploader")
--local get_files = require("get_files")--Using tmploader instead

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

    local recurse  = false
    local f_finddir  = nil -- function to match directories; nil all, false none
    local f_findfile = nil -- function to match files; nil all, false none

    local p_settings = {wrap = true, hasheader = true}

    local timer
    local files = {}
    local dirs = {}
    local item = 1
    local cancel_fn = nil

    if CANCEL_BUTTON and CANCEL_BUTTON > 0 then
        cancel_fn = function()
                        if rb.get_plugin_action(0) == CANCEL_BUTTON then rb.splash(0, "Canceled") return true end
                        return false
                    end
    else
        cancel_fn = function()
                        if rb.action_userabort(0)then rb.splash(0, "Canceled") return true end
                        return false
                    end
    end

    _lcd:clear()
    local get_files = tmploader("get_files")
    while item > 0 do
        if not title then
            timer = _timer()
        end
        rb.splash(1, "Searching for Files")
        dirs, files = get_files(dir, recurse, f_finddir, f_findfile, sort_by, cancel_fn, dirs, files)

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
