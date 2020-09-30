--[[ Lua RB Playback
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2020 William Wilgus
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

local print = require("print")

local _draw = require("draw")
local _poly = require("draw_poly")
local _clr  = require("color")

require("actions")
require("rbsettings")
require("settings")

local scrpath = rb.current_path()

local metadata = rb.settings.read
local cur_trk = "audio_current_track"
local track_data = {}
-- grab only settings we are interested in
track_data.title = rb.metadata.mp3_entry.title
track_data.path = rb.metadata.mp3_entry.path

do     -- free up some ram by removing items we don't need
    local function strip_functions(t, ...)
        local t_keep = {...}
        local keep
        for key, val in pairs(t) do
            keep = false
            for _, v in ipairs(t_keep) do
                if string.find (key, v) then
                    keep = true; break
                end
            end
            if keep ~= true then
                t[key] = nil
            end
        end
    end

    strip_functions(rb.actions, "PLA_", "TOUCHSCREEN", "_NONE")
    rb.contexts = nil

    strip_functions(_draw, "^rect$", "^rect_filled$")
    strip_functions(_poly, "^polyline$")
    strip_functions(print.opt, "line", "get")

    _clr.inc = nil
    rb.metadata = nil -- remove metadata settings
    rb.system = nil -- remove system settings
    rb.settings.dump = nil
    collectgarbage("collect")
end

local t_icn = {}
t_icn[1] = {16,16,16,4,10,10,10,4,4,10,10,16,10,10} -- rewind
t_icn[2] = {4,5,4,15,10,9,10,15,12,15,12,5,10,5,10,11,4,5} -- play/pause
t_icn[3] = {4,4,4,16,10,10,10,16,16,10,10,4,10,10} -- fast forward

local pb = {}
local track_length

local track_name = metadata(cur_trk, track_data.title) or
                   metadata(cur_trk, track_data.path)

local clr_active = _clr.set(1, 0, 255, 0)
local clr_inactive = _clr.set(0, 255, 255, 255)
local t_clr_icn = {clr_inactive, clr_inactive, clr_inactive}


local function set_active_icon(idx)
    local tClr = t_clr_icn

    if idx == 4 and t_clr_icn[4] == clr_active then
        idx = 2
    end
    for i = 1, 4 do
        t_clr_icn[i] = clr_inactive
    end
    if idx >= 1 and idx <= 4 then
        t_clr_icn[idx] = clr_active
    end
end

local function clear_actions()
    track_length = (rb.audio("length") or 0)
    local playback = rb.audio("status")
    if playback == 1 then
        set_active_icon(2)
    elseif playback == 3 then
        set_active_icon(4)
        return
    end
    rockev.trigger("timer", false, rb.current_tick() + rb.HZ * 60)
end

--------------------------------------------------------------------------------
--[[ returns a sorted tables of directories and (another) of files
-- path is the starting path; norecurse == true.. only that path will be searched
--   findfile & finddir are definable search functions
--  if not defined all files/dirs are returned if false is passed.. none
-- or you can provide your own function see below..
-- f_t and d_t allow you to pass your own tables for re-use but isn't necessary
]]
local function get_files(path, norecurse, finddir, findfile, sort_by, max_depth, f_t, d_t)
    local quit = false
    --local sort_by_function -- forward declaration
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

    if not max_depth then max_depth = 3 end
    max_depth = max_depth + 1 -- determined by counting '/' 3 deep = /d1/d2/d3/

    local function _get_files(path)
        local sep = ""
        local filepath
        local finfo_t
        local count
        if string.sub(path, - 1) ~= "/" then sep = "/" end
        for fname, isdir, finfo_t in luadir.dir(path, true) do
            if isdir and finddir(fname) then
                path, count = string.gsub(path, "/", "/")
                if count <= max_depth then
                    table.insert(dirs, path .. sep ..fname)
                end
            elseif not isdir and findfile(fname) then
                filepath = filepath_function(path, sep, fname)
                table.insert(files, filepath)
            end

            if  action_quit() then
                return true
            end
        end
    end

    rb.splash(10, "Searching for Files")

    filepath_function = function(path, sep, fname)
                            return string.format("%s%s%s;", path, sep, fname)
                        end
    table.insert(dirs, path) -- root

    for key,value in pairs(dirs) do
        --luadir.dir may error out so we need to do the call protected
        -- _get_files(value, CANCEL_BUTTON)
        _, quit = pcall(_get_files, value)

        if quit == true or norecurse then
            break;
        end
    end

    return dirs, files
end -- get_files

local audio_elapsed, audio_ff_rew
do
    local elapsed = 0
    local ff_rew = 0
    audio_elapsed = function()
        if ff_rew == 0 then elapsed = (rb.audio("elapsed") or 0) end
        return elapsed
    end

    audio_ff_rew = function(time_ms)
        if ff_rew ~= 0 and time_ms == 0 then
            rb.audio("stop")
            rb.audio("play", elapsed, 0)
            rb.sleep(100)
        elseif time_ms ~= 0 then
            elapsed = elapsed + time_ms
            if elapsed < 0 then elapsed = 0 end
            if elapsed > track_length then elapsed = track_length end
        end
        ff_rew = time_ms
    end
end

do
    local act = rb.actions
    local quit = false
    local last_action = 0
    local magnitude = 1
    local skip_ms = 100
    local playback

    function action_event(action)
        local event
        if action == act.PLA_EXIT or action == act.PLA_CANCEL then
            quit = true
        elseif action == act.PLA_RIGHT_REPEAT then
            event = pb.TRACK_FF
            audio_ff_rew(skip_ms * magnitude)
            if magnitude < 300 then
                magnitude = magnitude + 1
            end
        elseif action == act.PLA_LEFT_REPEAT then
            event = pb.TRACK_REW
            audio_ff_rew(-skip_ms * magnitude)
            if magnitude < 300 then
                magnitude = magnitude + 1
            end
        elseif action == act.PLA_SELECT then
            playback = rb.audio("status")
            if playback == 1 then
                rb.audio("pause")
                collectgarbage("collect")
            elseif playback == 3 then
                rb.audio("resume")
            end
            event = rb.audio("status") + 1
        elseif action == act.ACTION_NONE then
            magnitude = 1
            audio_ff_rew(0)
            if last_action == act.PLA_RIGHT then
                rb.audio("next")
            elseif last_action == act.PLA_LEFT then
                rb.audio("prev")
            end
        end

        if event then -- pass event id to playback_event
            rockev.trigger(pb.EV, true, event)
        end

        last_action = action
    end

    function action_set_quit(bQuit)
        quit = bQuit
    end

    function action_quit()
        return quit
    end
end

do
    pb.EV = "playback"
    -- custom event ids
    pb.STOPPED = 1
    pb.PLAY = 2
    pb.PAUSE = 3
    pb.PAUSED = 4
    pb.TRACK_REW = 5
    pb.PLAY_     = 6
    pb.TRACK_FF  = 7

    function playback_event(id, event_data)
        if id == pb.PLAY then
            id = pb.PLAY_
        elseif id == pb.PAUSE then
            id = 8
        elseif id == rb.PLAYBACK_EVENT_TRACK_BUFFER or
               id == rb.PLAYBACK_EVENT_TRACK_CHANGE then
            rb.lcd_scroll_stop()
            track_name  = metadata(cur_trk, track_data.title) or
                          metadata(cur_trk, track_data.path)
        else
            -- rb.splash(0, id)
        end

        set_active_icon(id - 4)
        rockev.trigger("timer", false, rb.current_tick() + rb.HZ)
    end
end

local function pbar_init(img, x, y, w, h, fgclr, bgclr, barclr)
    local t
    -- when initialized table is returned that points back to this function
    if type(img) == "table" then
        t = img
        t.pct = x
        if not t.set then error("not initialized", 2) end
    else
        t = {}
        t.img = img or rb.lcd_framebuffer()
        t.x = x or 1
        t.y = y or 1
        t.w = w or 10
        t.h = h or 10
        t.pct = 0
        t.fgclr = fgclr or _clr.set(-1, 255, 255, 255)
        t.bgclr = bgclr or _clr.set(0, 0, 50, 200)
        t.barclr = barclr or t.fgclr

        t.set = pbar_init
        setmetatable(t,{__call = t.set})
        return t
    end -- initalization
--============================================================================--
    if t.pct < 0 then
        t.pct = 0
    elseif t.pct > 100 then
        t.pct = 100
    end

    local wp = t.pct * (t.w - 4) / 100

    if wp < 1 and t.pct > 0 then wp = 1 end

    _draw.rect_filled(t.img, t.x, t.y, t.w, t.h, t.fgclr, t.bgclr, true)
    _draw.rect_filled(t.img, t.x + 2, t.y + 2, wp, t.h - 4, t.barclr, nil, true)
end -- pbar_init

local function create_playlist(startdir, file_search, maxfiles)

    function f_filedir_(name)
        if name:len() <= 2 and (name == "." or name == "..") then
            return false
        end
        if name:find(file_search) ~= nil then
            maxfiles = maxfiles -1
            if maxfiles < 1 then
                action_set_quit(true)
                return true
            end
            return true
        else
            return false
        end
    end

    local norecurse  = false
    local f_finddir  = nil
    local f_findfile = f_filedir_
    local files = {}
    local dirs = {}
    local max_depth = 3

    dirs, files = get_files(startdir, norecurse, f_finddir, f_findfile, nil, max_depth, dirs, files)

    if #files > 0 then
        rb.audio("stop")
        rb.playlist("create", scrpath .. "/", "playback.m3u8")
    end

    for i = 1, #files do
        rb.playlist("insert_track", string.match(files[i], "[^;]+") or "?")
    end
    if #files > 0 then
        rb.playlist("start", 0, 0, 0)
    end

    for i=1, #dirs do dirs[i] = nil end -- empty table
    for i=1, #files do files[i] = nil end -- empty table
    action_set_quit(false)
end -- create_playlist

local function main()
    clear_actions()
    local lcd = rb.lcd_framebuffer()

    local eva = rockev.register("action", action_event)

    local evp = rockev.register("playback", playback_event)

    if not track_name then -- Nothing loaded lets search for some mp3's
        create_playlist("/", "%.mp3$", 10)
        collectgarbage("collect")
    end

    local evt = rockev.register("timer", clear_actions, rb.HZ)

    rb.lcd_clear_display()
    rb.lcd_update()
    do -- configure print function
        local t_print = print.opt.get(true)
        t_print.autoupdate = false
        t_print.justify    = "center"
        t_print.col = 2
    end

    local progress = pbar_init(nil, 1, rb.LCD_HEIGHT - 5, rb.LCD_WIDTH - 1, 5)

    local i = 0

    local colw = (rb.LCD_WIDTH - 16) / 4
    local scr_col = {colw, colw * 2, colw * 3}
    --local mem = collectgarbage("count")
    --local mem_min = mem
    --local mem_max = mem
    while not action_quit() do
        elapsed = audio_elapsed()

        -- control initialized with pbar_init now we set the value
        progress(track_length > 0 and elapsed / (track_length / 100) or 0)

        print.opt.line(1)
        print.f() -- clear the line
        print.f(track_name)
        print.f() -- clear the line
        _,_,_,h = print.f("%ds / %ds", elapsed / 1000, track_length / 1000)

        for i = 1, 3 do -- draw the rew/play/fwd icons
            _poly.polyline(lcd, scr_col[i], h * 2 + 1, t_icn[i], t_clr_icn[i], true)
        end
--[[
        mem = collectgarbage("count")
        if mem < mem_min then mem_min = mem end
        if mem > mem_max then mem_max = mem end

            if i >= 10 then
                rb.splash(0, mem_min .. " : " .. mem .. " : " ..mem_max)
                i = 0
            else
                i = i + 1
            end
--]]
        rb.lcd_update()
        rb.sleep(rb.HZ / 2)
    end

    rb.splash(100, "Goodbye")
end

main() -- BILGUS
