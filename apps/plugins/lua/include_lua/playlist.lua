--[[ Lua RB Playlist Operations
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2018 William Wilgus
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

-- [[ conversion to old style playlist_ functions ]]
if not rb.playlist then rb.splash(rb.HZ, "No Support!") return nil end

rb.playlist_amount =            function()
                                    return rb.playlist("amount")
                                end
rb.playlist_add =               function (filename)
                                    return rb.playlist("add", filename)
                                end
rb.playlist_create =            function(dir, filename)
                                    return rb.playlist("create", dir, filename)
                                end
rb.playlist_start =             function(index, elapsed, offset)
                                    rb.playlist("start", index, elapsed, offset)
                                end
rb.playlist_resume_track =      function(index, crc, elapsed, offset)
                                    rb.playlist("resume_track", index, crc, elapsed, offset)
                                end
rb.playlist_resume =            function() return rb.playlist("resume") end
rb.playlist_shuffle =           function(random_seed, index)
                                    rb.playlist("shuffle", random_seed, index)
                                end
rb.playlist_sync =              function () rb.playlist("sync") end
rb.playlist_remove_all_tracks = function() return rb.playlist("remove_all_tracks") end
rb.playlist_insert_track =      function(filename, pos, bqueue, bsync)
                                    return rb.playlist("insert_track", filename, pos, bqueue, bsync)
                                end
rb.playlist_insert_directory =  function(dir, pos, bqueue, brecurse)
                                    return rb.playlist("insert_directory", dir, pos, bqueue, brecurse)
                                end
rb.playlist_tracks =            function(dir, filename)
                                    local tracks = {}
                                    local count = 0
                                    local fullpath = dir .. "/" .. filename
                                    local file = io.open('/' .. fullpath, "r")

                                    if not file then
                                        rb.splash(rb.HZ, "Error opening /" .. fullpath)
                                        return tracks
                                    end

                                    -- strip BOM --"\xEF\xBB\xBF"
                                    local bom = file:read(3)
                                    if not string.find(bom, "\239\187\191") then
                                        file:seek("set", 0)
                                    end

                                    for line in file:lines() do
                                        if string.len(line) > 3 and (not string.find(line, "%s*#.*")) then
                                            count = count + 1
                                            tracks[count] = line
                                        end
                                    end
                                    file:close()
                                    return tracks
                                end

