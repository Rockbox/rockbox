--[[ Lua RB Random Playlist -- random_playlist.lua V 1.0
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 William Wilgus
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
--[[ random_playlist
    This script opens the users database file containg track path + filenames
    first it reads the database file making an index of tracks
    [for large playlists it only saves an index every [10|100|1000] tracks.
    tracks will be incrementally loaded along with the results of the entries
    traversed but the garbage collector will erase them when needed]

    next tracks are choosen at random and added either to an in-ram playlist
    using plugin functions OR
    to a on disk playlist using a table as a write buffer
    the user can also choose to play the playlist in either case
]]

require ("actions")
require("dbgettags")
get_tags = nil -- unneeded

-- User defaults
local playlistpath = "/Playlists"
local max_tracks = 500;  -- size of playlist to create
local min_repeat = 500;  -- this many songs before a repeat
local play_on_success = true;
local playlist_name = "random_playback.m3u8"
--program vars
local playlist_handle
local t_playlistbuf -- table for playlist write buffer

-- Random integer function
local random = math.random; -- ref random(min, max)
math.randomseed(rb.current_tick()); -- some kind of randomness

-- Button definitions
local CANCEL_BUTTON = rb.actions.PLA_CANCEL
local OK_BUTTON = rb.actions.PLA_SELECT
local ADD_BUTTON = rb.actions.PLA_UP
local ADD_BUTTON_RPT = rb.actions.PLA_UP_REPEAT or ADD_BUTTON
local SUB_BUTTON = rb.actions.PLA_DOWN
local SUB_BUTTON_RPT = rb.actions.PLA_DOWN_REPEAT or SUB_BUTTON
-- remove action and context tables to free some ram
rb.actions = nil
rb.contexts = nil
-- Program strings
local sINITDATABASE    = "Initialize Database"
local sHEADERTEXT      = "Random Playlist"
local sPLAYLISTERROR   = "Playlist Error!"
local sSEARCHINGFILES  = "Searching for Files.."
local sERROROPENFMT    = "Error Opening %s"
local sINVALIDDBFMT    = "Invalid Database %s"
local sPROGRESSHDRFMT  = "%d \\ %d Tracks"
local sGOODBYE         = "Goodbye"

-- Gets size of text
local function text_extent(msg, font)
    font = font or rb.FONT_UI
    return rb.font_getstringsize(msg, font)
end

local function _setup_random_playlist(tag_entries, play, savepl, min_repeat, trackcount)
    -- Setup string tables
    local tPLAYTEXT   = {"Play? [ %s ] (up/dn)", "true = play tracks on success"}
    local tSAVETEXT   = {"Save to disk? [ %s ] (up/dn)",
                         "true = tracks saved to",
                         playlist_name};
    local tREPEATTEXT = {"Repeat hist? [ %d ] (up/dn)","higher = less repeated songs"}
    local tPLSIZETEXT = {"Find [ %d ] tracks? (up/dn)",
                         "Warning may overwrite dynamic playlist",
                         "Press back to cancel"};
    -- how many lines can we fit on the screen?
    local res, w, h = text_extent("I")
    h = h + 5 -- increase spacing in the setup menu
    local max_w = rb.LCD_WIDTH / w
    local max_h = rb.LCD_HEIGHT  - h
    local y = 0

    -- User Setup Menu
    local action, ask, increment
    local t_desc = {scroll = true} -- scroll the setup items

    -- Clears screen and adds title and icon, called first..
    function show_setup_header()
        local desc = {icon = 2, show_icons = true, scroll = true} -- 2 == Icon_Playlist
        rb.lcd_clear_display()
        rb.lcd_put_line(1, 0, sHEADERTEXT, desc)
    end

    -- Display up to 3 items and waits for user action -- returns action
    function ask_user_action(desc, ln1, ln2, ln3)
        if ln1 then rb.lcd_put_line(1, h, ln1, desc) end
        if ln2 then rb.lcd_put_line(1, h + h, ln2, desc) end
        if ln3 then rb.lcd_put_line(1, h + h + h, ln3, desc) end
        rb.lcd_hline(1,rb.LCD_WIDTH - 1, h - 5);
        rb.lcd_update()

        local act = rb.get_plugin_action(-1); -- Blocking wait for action
        -- handle magnitude of the increment here so consumer fn doesn't need to
        if act == ADD_BUTTON_RPT and act ~= ADD_BUTTON then
            increment = increment + 1
            if increment > 1000 then increment = 1000 end
            act = ADD_BUTTON
        elseif act == SUB_BUTTON_RPT and act ~= SUB_BUTTON then
            increment = increment + 1
            if increment > 1000 then increment = 1000 end
            act = SUB_BUTTON
        else
            increment = 1;
        end

        return act
    end

    -- Play the playlist on successful completion true/false?
    function setup_get_play()
        action = ask_user_action(tdesc,
                                 string.format(tPLAYTEXT[1], tostring(play)),
                                 tPLAYTEXT[2]);
        if action == ADD_BUTTON then
            play = true
        elseif action == SUB_BUTTON then
            play = false
        end
    end

    -- Save the playlist to disk true/false?
    function setup_get_save()
        action = ask_user_action(tdesc,
                                 string.format(tSAVETEXT[1], tostring(savepl)),
                                 tSAVETEXT[2], tSAVETEXT[3]);
        if action == ADD_BUTTON then
            savepl = true
        elseif action == SUB_BUTTON then
            savepl = false
        elseif action == OK_BUTTON then
            ask = setup_get_play;
            setup_get_save = nil
            action = 0
        end
    end

    -- Repeat song buffer list of previously added tracks 0-??
    function setup_get_repeat()
        if min_repeat >= trackcount then min_repeat = trackcount - 1 end
        if min_repeat >= tag_entries then min_repeat = tag_entries - 1 end
        action = ask_user_action(t_desc,
                                 string.format(tREPEATTEXT[1],min_repeat),
                                 tREPEATTEXT[2]);
        if action == ADD_BUTTON then
            min_repeat = min_repeat + increment
        elseif action == SUB_BUTTON then -- MORE REPEATS LESS RAM USED
            if min_repeat < increment then increment = 1 end
            min_repeat = min_repeat - increment
            if min_repeat < 0 then min_repeat = 0 end
        elseif action == OK_BUTTON then
            ask = setup_get_save;
            setup_get_repeat = nil
            action = 0
        end
    end

    -- How many tracks to find
    function setup_get_playlist_size()
        action = ask_user_action(t_desc,
                                 string.format(tPLSIZETEXT[1], trackcount),
                                 tPLSIZETEXT[2],
                                 tPLSIZETEXT[3]);
        if action == ADD_BUTTON then
            trackcount = trackcount + increment
        elseif action == SUB_BUTTON then
            if trackcount < increment then increment = 1 end
            trackcount = trackcount - increment
            if trackcount < 1 then trackcount = 1 end
        elseif action == OK_BUTTON then
            ask = setup_get_repeat;
            setup_get_playlist_size = nil
            action = 0
        end
    end
    ask = setup_get_playlist_size; -- \!FIRSTRUN!/

    repeat -- SETUP MENU LOOP
        show_setup_header()
        ask()
        rb.lcd_scroll_stop() -- I'm still wary of not doing this..
        collectgarbage("collect")
        if action == CANCEL_BUTTON then rb.lcd_scroll_stop(); return nil end
    until (action == OK_BUTTON)

    return play, savepl, min_repeat, trackcount;
end
--[[ manually create a playlist
playlist is created initially by creating a new file (or erasing old)
and adding the BOM]]
--deletes existing file and creates a new playlist
local function playlist_create(filename)
    local filehandle = io.open(filename, "w+") --overwrite
    if not filehandle then
        rb.splash(rb.HZ, "Error opening " .. filename)
        return false
    end
    t_playlistbuf = {}
    filehandle:write("\239\187\191") -- Write BOM --"\xEF\xBB\xBF"
    playlist_handle = filehandle
    return true
end

-- writes track path to a buffer must be later flushed to playlist file
local function playlist_insert(trackpath)
    local bufp = #t_playlistbuf + 1
    t_playlistbuf[bufp] = trackpath
    bufp = bufp + 1
    t_playlistbuf[bufp] = "\n"
    return bufp
end

-- flushes playlist buffer to file
local function playlist_flush()
    playlist_handle:write(table.concat(t_playlistbuf))
    t_playlistbuf = {}
end

-- closes playlist file descriptor
local function playlist_finalize()
    playlist_handle:close()
    return true
end

--[[ Given the filenameDB file [database]
    creates a random dynamic playlist with a default savename of [playlist]
    containing [trackcount] tracks, played on completion if [play] is true]]
local function create_random_playlist(database, playlist, trackcount, play, savepl)
    if not database or not playlist or not trackcount then return end
    if not play then play = false end
    if not savepl then savepl = false end

    local playlist_handle
    local playlistisfinalized = false
    local file = io.open('/' .. database or "", "r") --read
    if not file then rb.splash(100, string.format(sERROROPENFMT, database)) return end

    local fsz = file:seek("end")
    local fbegin
    local posln = 0
    local tag_len = TCHSIZE

    local anchor_index
    local ANCHOR_INTV
    local track_index = setmetatable({},{__mode = "v"}) --[[ weak table values
      this allows them to be garbage collected as space is needed / rebuilt as needed ]]

    -- Read character function sets posln as file position
    function readchrs(count)
        if posln >= fsz then return nil end
        file:seek("set", posln)
        posln = posln + count
        return file:read(count)
    end

    -- Check the header and get size + #entries
    local tagcache_header = readchrs(DATASZ) or ""
    local tagcache_sz = readchrs(DATASZ) or ""
    local tagcache_entries = readchrs(DATASZ) or ""

    if tagcache_header ~= sTCHEADER or
        bytesLE_n(tagcache_sz) ~= (fsz - TCHSIZE) then
        rb.splash(100, string.format(sINVALIDDBFMT, database))
        return
    end

    local tag_entries = bytesLE_n(tagcache_entries)

    play, savepl, min_repeat, trackcount = _setup_random_playlist(
                            tag_entries, play, savepl, min_repeat, trackcount);
    _setup_random_playlist = nil

    if savepl == false then
        -- Use the rockbox playlist functions to add tracks to in-ram playlist
        playlist_create  = function(filename)
            return (rb.playlist("create", playlistpath .. "/", playlist) >= 0)
        end
        playlist_insert = function(str)
            return rb.playlist("insert_track", str)
        end
        playlist_flush = function() end
        playlist_finalize = function()
            return (rb.playlist("amount") >= trackcount)
        end
    end
    if not playlist_create(playlistpath .. "/" .. playlist) then return end
    collectgarbage("collect")

    -- how many lines can we fit on the screen?
    local res, w, h = text_extent("I")
    local max_w = rb.LCD_WIDTH / w
    local max_h = rb.LCD_HEIGHT  - h
    local y = 0
    rb.lcd_clear_display()

    function get_tracks_random()
        local tries, idxp

        local tracks = 0
        local str = ""
        local t_lru = {}
        local lru_widx = 1
        local lru_max = min_repeat
        if lru_max >= tag_entries then lru_max = tag_entries / 2 + 1 end

        function do_progress_header()
            rb.lcd_put_line(1, 0, string.format(sPROGRESSHDRFMT,tracks, trackcount))
            rb.lcd_update()
            --rb.sleep(300)
        end

        function show_progress()
            local sdisp = str:match("([^/]+)$") or "?" --just the track name
            rb.lcd_put_line(1, y, sdisp:sub(1, max_w));-- limit string length
            y = y + h
            if y >= max_h then
                do_progress_header()
                rb.lcd_clear_display()
                playlist_flush(playlist_handle)
                rb.yield()
                y = h
            end
        end

        -- check for repeated tracks
        function check_lru(val)
            if lru_max <= 0 or val == nil then return 0 end --user wants all repeats
            local rv
            local i = 1
            repeat
                rv = t_lru[i]
                if rv == nil then
                    break;
                elseif rv == val then
                    return i
                end
                i = i + 1
            until (i == lru_max)
            return 0
        end

        -- add a track to the repeat list (overwrites oldest if full)
        function push_lru(val)
            t_lru[lru_widx] = val
            lru_widx = lru_widx + 1
            if lru_widx > lru_max then lru_widx = 1 end
        end

        function get_index()
            if ANCHOR_INTV > 1 then
                get_index =
                function(plidx)
                    local p = track_index[plidx]
                    if p == nil then
                        parse_database_offsets(plidx)
                    end
                    return track_index[plidx][1]
                end
            else -- all tracks are indexed
                get_index =
                function(plidx)
                    return track_index[plidx]
                end
            end
        end

        get_index() --init get_index fn
        -- Playlist insert loop
        while true do
            str = nil
            tries = 0
            repeat
                idxp = random(1, tag_entries)
                tries = tries + 1 -- prevent endless loops
            until check_lru(idxp) == 0 or tries > fsz -- check for recent repeats

            posln = get_index(idxp)

            tag_len = bytesLE_n(readchrs(DATASZ))
            posln = posln + DATASZ -- idx = bytesLE_n(readchrs(DATASZ))
            str = readchrs(tag_len) or "\0" -- Read the database string
            str = str:match("^(%Z+)%z$") -- \0 terminated string

            -- Insert track into playlist
            if str ~= nil then
                tracks = tracks + 1
                show_progress()
                push_lru(idxp) -- add to repeat list
                if playlist_insert(str) < 0 then
                    rb.sleep(rb.HZ) --rb playlist fn display own message wait for that
                    rb.splash(rb.HZ, sPLAYLISTERROR)
                    break; -- ERROR, PLAYLIST FULL?
                end
            end

            if tracks >= trackcount then
                playlist_flush()
                do_progress_header()
                break
            end

            -- check for cancel non-blocking
            if rb.get_plugin_action(0) == CANCEL_BUTTON then
                break
            end
        end
    end -- get_files

    function build_anchor_index()
        -- index every n files
        ANCHOR_INTV = 1 -- for small db we can put all the entries in ram
        local ent = tag_entries / 100 -- more than 1000 will be incrementally loaded
        while ent >= 10 do -- need to reduce the size of the anchor index?
            ent = ent / 10
            ANCHOR_INTV = ANCHOR_INTV * 10
        end -- should be power of 10 (10, 100, 1000..)
        --grab an index for every ANCHOR_INTV entries
        local aidx={}
        local acount = 0
        local next_idx = 1
        local index = 1
        local tlen
        if ANCHOR_INTV == 1 then acount = 1 end
        while index <= tag_entries and posln < fsz do
            if next_idx == index then
                acount = acount + 1
                next_idx = acount * ANCHOR_INTV
                aidx[index] = posln
            else -- fill the weak table, we already did the work afterall
                track_index[index] = {posln} -- put vals inside table to make them collectable
            end
            index = index + 1
            tlen = bytesLE_n(readchrs(DATASZ))
            posln = posln + tlen + DATASZ
        end
        return aidx
    end

    function parse_database_offsets(plidx)
        local tlen
        -- round to nearest anchor entry that is less than plidx
        local aidx = (plidx / ANCHOR_INTV) * ANCHOR_INTV
        local cidx = aidx
        track_index[cidx] = {anchor_index[aidx] or fbegin};
        -- maybe we can use previous work to get closer to the desired offset
        while track_index[cidx] ~= nil and cidx <= plidx do
            cidx = cidx + 1 --keep seeking till we find an empty entry
        end
        posln = track_index[cidx - 1][1]
        while cidx <= plidx do --[[ walk the remaining entries from the last known
            & save the entries on the way to our desired entry ]]
            tlen = bytesLE_n(readchrs(DATASZ))
            posln = posln + tlen + DATASZ
            track_index[cidx] = {posln} -- put vals inside table to make them collectable
            if posln >= fsz then posln = fbegin  end
            cidx = cidx + 1
        end
    end

    if trackcount ~= nil then
        rb.splash(10, sSEARCHINGFILES)
        fbegin = posln --Mark the beginning for later loops
        tag_len = 0
        anchor_index =  build_anchor_index() -- index track offsets
        if ANCHOR_INTV == 1 then
            -- all track indexes are in ram
            track_index = anchor_index
            anchor_index = nil
        end
--[[ --profiling
        local starttime = rb.current_tick();
        get_tracks_random()
        local endtime = rb.current_tick();
        rb.splash(1000, (endtime - starttime) .. " ticks");
        end
    if (false) then
--]]
        get_tracks_random()
        playlistisfinalized = playlist_finalize(playlist_handle)
    end

    file:close()
    collectgarbage("collect")
    if trackcount and play == true and playlistisfinalized == true then
        rb.audio("stop")
        rb.yield()
        if savepl == true then
            rb.playlist("create", playlistpath .. "/", playlist)
            rb.playlist("insert_playlist", playlistpath .. "/" .. playlist)
            rb.sleep(rb.HZ)
        end
        rb.playlist("start", 0, 0, 0)
    end

end -- playlist_create

local function main()
    if not rb.file_exists(rb.ROCKBOX_DIR .. "/database_4.tcd") then
        rb.splash(rb.HZ, sINITDATABASE)
        os.exit(1);
    end
    if rb.cpu_boost then rb.cpu_boost(true) end
    rb.backlight_force_on()
    if not rb.dir_exists(playlistpath) then
        luadir.mkdir(playlistpath)
    end
    rb.lcd_clear_display()
    rb.lcd_update()
    collectgarbage("collect")
    create_random_playlist(rb.ROCKBOX_DIR .. "/database_4.tcd",
                        playlist_name, max_tracks, play_on_success);
    -- Restore user backlight settings
    rb.backlight_use_settings()
    if rb.cpu_boost then rb.cpu_boost(false) end
    rb.sleep(rb.HZ)
    rb.splash(rb.HZ * 2, sGOODBYE)
--[[ 
local used, allocd, free = rb.mem_stats()
local lu = collectgarbage("count")
local fmt = function(t, v) return string.format("%s: %d Kb\n", t, v /1024) end

-- this is how lua recommends to concat strings rather than ..
local s_t = {}
s_t[1] = "rockbox:\n"
s_t[2] = fmt("Used  ", used)
s_t[3] = fmt("Allocd ", allocd)
s_t[4] = fmt("Free  ", free)
s_t[5] = "\nlua:\n"
s_t[6] = fmt("Used", lu * 1024)
s_t[7] = "\n\nNote that the rockbox used count is a high watermark"
rb.splash_scroller(10 * rb.HZ, table.concat(s_t)) --]]

end --MAIN

main() -- BILGUS
