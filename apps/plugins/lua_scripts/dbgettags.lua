-- dbgettags.lua Bilgus 2017
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

require("actions")
local CANCEL_BUTTON = rb.actions.PLA_CANCEL

local sINVALIDDATABASE = "Invalid Database"
local sERROROPENING    = "Error opening"

-- tag cache header
sTCVERSION = string.char(0x10)
sTCHEADER  = string.reverse("TCH" .. sTCVERSION)
DATASZ    = 4  -- int32_t
TCHSIZE   = 3 * DATASZ -- 3 x int32_t

-- Converts array of bytes to proper endian
function bytesLE_n(str)
    str = str or ""
    local tbyte={str:byte(1, -1)}
    local bpos = 1
    local num  = 0
    for k = 1,#tbyte do -- (k = #t, 1, -1 for BE)
        num = num + tbyte[k] * bpos
        bpos = bpos * 256
    end
    return num
end

-- uses database files to retrieve database tags
-- adds all unique tags into a lua table
-- ftable is optional
function get_tags(filename, hstr, ftable)

    if not filename then return end
    if not ftable then ftable = {} end
    hstr = hstr or filename

    local file = io.open('/' .. filename or "", "r") --read
    if not file then rb.splash(100, sERROROPENING .. " " ..  filename) return end

    local fsz = file:seek("end")

    local posln = 0
    local tag_len = TCHSIZE
    local idx

    local function readchrs(count)
        if posln >= fsz then return nil end
        file:seek("set", posln)
        posln = posln + count
        return file:read(count)
    end

    -- check the header and get size + #entries
    local tagcache_header = readchrs(DATASZ) or ""
    local tagcache_sz = readchrs(DATASZ) or ""
    local tagcache_entries = readchrs(DATASZ) or ""

    if tagcache_header ~= sTCHEADER or
        bytesLE_n(tagcache_sz) ~= (fsz - TCHSIZE) then
        rb.splash(100, sINVALIDDATABASE .. " " .. filename)
        return
    end
    
    -- local tag_entries = bytesLE_n(tagcache_entries)

    for k, v in pairs(ftable) do ftable[k] = nil end -- clear table
    ftable[1] = hstr

    local tline = #ftable + 1
    ftable[tline] = ""

    local str = ""

    while true do
        tag_len = bytesLE_n(readchrs(DATASZ))
        readchrs(DATASZ) -- idx = bytesLE_n(readchrs(DATASZ))
        str = readchrs(tag_len) or ""
        str = string.match(str, "(%Z+)%z") -- \0 terminated string 

        if str then
            if ftable[tline - 1] ~= str then -- Remove dupes
                ftable[tline] = str
                tline = tline + 1
            end
        elseif posln >= fsz then
            break
        end

        if rb.get_plugin_action(0) == CANCEL_BUTTON then
            break
        end
    end

    file:close()

    return ftable 
end -- get_tags
