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
 * Copyright (C) 2019 William Wilgus
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

require("actions")
rb.contexts = nil

local act = rb.actions
local last_action = act.ACTION_NONE
local dir_iter, dir_data

local plugin_icon = 9
local excludedsrc = ";filebrowse.lua;fileviewers.lua;printmenu.lua;dbgettags.lua;"
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
--[[File Properties]]
local function file_prop(sPath, sFileName)
    local t_attrib
    local function to_datetime_str(nTm, bMDY)
        local t = os.date("*t", nTm)
        local t_date
        if bMDY then
            t_date= {t.month, t.day, t.year}
        else
            t_date= {t.day, t.month, t.year}
        end

        local sDate = table.concat(t_date, "/")
        local sTime = string.format("%d:%0d.%0d", t.hour,t.min,t.sec)

        return sDate, sTime
    end
    local function to_log_size(nSizeBytes)
        local tsuf = {" B", " KiB", " MiB", " GiB"}
        local n, nf = nSizeBytes, nSizeBytes
        local sSz = string.format("%d %s", n, tsuf[1])
        for i = 2, #tsuf do
            n = n/1024
            if n > 0 then
                sSz = string.format("%d.%d %s", n, (nf - n * 1024) / 100, tsuf[i])
            else
                break
            end
            nf = n
        end
        return sSz
    end

    if sPath and sFileName then
        for fname, isdir, attrib in luadir.dir(sPath, true) do
            if fname and rb.strncasecmp(fname, sFileName) == 0 then
                t_attrib = attrib
                break
            end
        end
    end
    if t_attrib then
        local sDate, sTime = to_datetime_str(t_attrib.time)
        local sSize = to_log_size(t_attrib.size)
        local sPath = string.gsub(sPath, "//", "/")
        local tprop = {"Date", sDate, "Time", sTime, "Size", sSize, "Path", sPath}
        sel = rb.do_menu(sFileName, tprop)
    end
end
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
--[[Menu display]]
local function menu_action_cb(action)
    if action == act.ACTION_STD_CONTEXT then
        last_action = action
        action = act.ACTION_STD_OK -- select the item so we get the item#
    else
        --rb.splash(0, "Action: " .. action)
    end
    return action
end

function context_menu(filename)
    local title = filename or "Unknown"
    local ctxmenu = {"Run" , "Properties", "Delete " .. title}
    local t_yn, t_y, t_n
    local sel = rb.do_menu(title, ctxmenu)

    if sel == 1 then
        file_prop(scrpath, filename)
    elseif sel == 2 then
        t_yn = {"Delete " .. filename, "Yes", "No"}
        t_y, t_n = {"Deleting ", filename}, {"Canceled"}
        if rb.gui_syncyesno_run(t_yn, t_y, t_n) == 0 then
            os.remove(scrpath .. "/" .. title)
        end
    end
    return sel -- go back
end

function scripts(item, bCount)
    local retry, status, script, isdir
    local count, retry_ct = 0, 100
    if bCount then dir_iter, dir_data = luadir.dir(scrpath) end
    repeat
        retry = false
        status, script, isdir = pcall(dir_iter, dir_data)
        if not status then
            dir_iter, dir_data = luadir.dir(scrpath)
            if bCount then return count end
            retry = true
            retry_ct = retry_ct - 1
        else
            if not script or isdir or
               string.sub(script, -4) ~= ".lua" or
               string.find(excludedsrc, ";" .. script .. ";") then 
                retry = true -- not a file we want to display
            else
                count = count + 1
                retry = bCount
            end
        end
        if bCount then retry = (item ~= count) end
    until not retry or retry_ct <= 0

    return script, plugin_icon
end

function script_choose(dir, title, start)
    local sel, filename

    while true do
        sel = rb.do_menu(title, scripts , start, false,
                         menu_action_cb, scripts(nil, true), plugin_icon)

        start = sel >= 0 and sel or start
        filename = scripts(sel + 1, true)

        if last_action == act.ACTION_STD_CONTEXT then
            last_action = act.ACTION_NONE

            if context_menu(filename) == 0 then
                return filename
            end
        elseif sel >= 0 then
            return filename
        else
            return nil, s -- exit
        end
    end
end

--------------------------------------------------------------------------------
--[[Main]]
local script_name = script_choose(scrpath, "lua Scripts")

if script_name then rb.restart_lua(scrpath .. "/" .. script_name) end
--------------------------------------------------------------------------------
