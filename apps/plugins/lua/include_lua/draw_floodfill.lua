--[[ Lua Floodfill function
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
-- floods an area of targetclr with fillclr x, y specifies the start seed
-- flood_fill(img, x, y, targetclr, fillclr)
if not rb.lcd_framebuffer then rb.splash(rb.HZ, "No Support!") return nil end
do

    local rocklib_image = getmetatable(rb.lcd_framebuffer())
    local _NIL = nil -- nil placeholder
    local _get     = rocklib_image.get
    local _line    = rocklib_image.line
    local _marshal = rocklib_image.marshal

    return function(img, x, y, targetclr, fillclr)
        -- scanline 4-way flood algorithm
        --          ^
        -- <--------x--->
        --          v
        -- check that target color doesn't = fill and the first point is target color
        if targetclr == fillclr or targetclr ~= _get(img, x, y, true) then return end
        local max_w = img:width()
        local max_h = img:height()

        local qpt = {} -- FIFO queue
        -- rather than moving elements around in our FIFO queue
        -- for each read; increment 'qhead' by 2
        -- set both elements to nil and let the
        -- garbage collector worry about it
        -- for each write; increment 'qtail' by 2
        -- x coordinates are in odd indices while
        -- y coordinates are in even indices

        local qtail = 0

        local function check_ns(val, x, y)
            if targetclr == val then
                y = y - 1
                if targetclr == _get(img, x, y, true) then -- north
                    qtail = qtail + 2
                    qpt[qtail - 1] = x
                    qpt[qtail] = y
                end
                y = y + 2
                if targetclr == _get(img, x, y, true) then -- south
                    qtail = qtail + 2
                    qpt[qtail - 1] = x
                    qpt[qtail] = y
                end
                return fillclr
            end
            return _NIL -- signal marshal to stop
        end

        local function seed_pt(x, y)
            -- should never hit max but make sure not to end early
            for qhead = 2, 0x40000000, 2 do

                if targetclr == _get(img, x, y, true) then
                    _marshal(img, x, y, 1, y, _NIL, _NIL, true, check_ns) -- west
                    _marshal(img, x + 1, y, max_w, y, _NIL, _NIL, true, check_ns) -- east
                end

                x = qpt[qhead - 1]
                    qpt[qhead - 1] = _NIL

                if not x then break end

                y = qpt[qhead]
                    qpt[qhead] = _NIL
            end
        end

        seed_pt(x, y) -- Begin
    end -- flood_fill
end
