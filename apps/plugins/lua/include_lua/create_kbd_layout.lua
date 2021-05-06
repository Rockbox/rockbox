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
--create keyboard layout
--BILGUS 4/2021
-- create_kbd_layout = require("create_kbd_layout")
-- local layout = create_kbd_layout("abcd")

local function encode_short(n)
    return string.char(bit.band(0x00FF, n), bit.rshift(bit.band(0xFF00, n), 8))
end

local function utf8decode(str)
    local INVALID = 0xfffd
    local t = {}
    local function check_char(c)
        local tail = false
        local code
        c = string.byte(c)
        if (c <= 0x7f) or (c >= 0xc2) then
            -- Start of new character
            if (c < 0x80) then        -- U-00000000 - U-0000007F, 1 string.byte
                code = c;
            elseif (c < 0xe0) then   -- U-00000080 - U-000007FF, 2 string.bytes
                tail = 1;
                code = bit.band(c, 0x1f)
            elseif (c < 0xf0) then   -- U-00000800 - U-0000FFFF, 3 string.bytes
                tail = 2;
                code = bit.band(c, 0x0f)
            elseif (c < 0xf5) then -- U-00010000 - U-001FFFFF, 4 string.bytes
                tail = 3;
                code = bit.band(c, 0x07)
            else
                -- Invalid size
                code = INVALID;
            end

            while tail and c ~= 0 do
                tail = tail - 1
                if bit.band(c, 0xc0) == 0x80 then
                    -- Valid continuation character
                    code = bit.bor(bit.lshift(code, 6),bit.band(c, 0x3f))
                else
                    -- Invalid continuation char
                    code = INVALID;
                    break;
                end
            end
        else
            -- Invalid UTF-8 char
            code = INVALID;
        end
        -- currently we don't support chars above U-FFFF
        t[#t + 1 ] = encode_short((code < 0x10000) and code or INVALID)
    end
    str:gsub(".", check_char) -- run check function for every char
    return table.concat(t)
end

local function create_keyboard_layout(s_layout)
    local insert = table.insert
    lines = {}

    for str in string.gmatch(s_layout, "([^\n]+)") do
        local len = string.len(str)
        lines[#lines + 1] =
        table.concat({encode_short(len), utf8decode(str)})
    end
    lines[#lines + 1] = encode_short(0xFEFF)

    return table.concat(lines)
end


--[[
local name = "Test_KBD_LAYOUT_" .. tostring(1)
local test = create_keyboard_layout("ABCDEFGHIJKLM\nNOPQRSTUVWXYZ\n0123456789")
local file = io.open('/' .. name, "w+") -- overwrite, rb ignores the 'b' flag
file:write(test)-- write the layout to the file now
file:close()

if not file then
    rb.splash(rb.HZ, "Error opening /" .. name)
    return
end
rb.kbd_input(name, test)
]]
return create_keyboard_layout
