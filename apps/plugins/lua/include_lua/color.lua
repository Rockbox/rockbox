--[[ Lua Color functions
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

--[[ Exposed Functions

    _clr.set
    _clr.inc

]]
if not rb.lcd_framebuffer then rb.splash(rb.HZ, "No Support!") return nil end

-- Exposed Constants
IS_COLOR_TARGET = false
-- Only true when we're on a color target, i.e. when LCD_RGBPACK is available
if rb.lcd_rgbpack ~= _NIL then
    IS_COLOR_TARGET = true
end

local _clr = {} do

    -- Internal Constants
    local _NIL = nil -- _NIL placeholder

    local maxstate = (bit.lshift(1, rb.LCD_DEPTH) - 1)

    local function init(v)
         return v or 0
    end

   -- clamps value to >= min and <= max rolls over to opposite
    local function clamp_roll(val, min, max)
        if min > max then
            local swap = min
            min, max = max, swap
        end

        if val < min then
            val = max
        elseif val > max then
            val = min
        end

        return val
    end

    -- sets color -- monochrome / greyscale use 'set' -- color targets 'r,b,g'
    -- on monochrome/ greyscale targets:
    -- '-1' sets the highest 'color' state & 0 is the minimum 'color' state
    local function clrset(set, r, g, b)
        local color = set or 0

        if IS_COLOR_TARGET then
            if (r ~= _NIL or g ~= _NIL or b ~= _NIL) then
                r, g, b = init(r), init(g), init(b)
                color = rb.lcd_rgbpack(r, g, b)
            end
        else
            color = clamp_roll(set, 0, maxstate)
        end

        return color
    end -- clrset

    -- de/increments current color by 'inc' -- optionally color targets by 'r,g,b'
    local function clrinc(current, inc, r, g, b)
        local color = 0
        current = current or color

        if IS_COLOR_TARGET then
            local ru, gu, bu = rb.lcd_rgbunpack(current);
            if (r ~= _NIL or g ~= _NIL or b ~= _NIL) then
                r, g, b = init(r), init(g), init(b)
                ru = ru + r; gu = gu + g; bu = bu + b
                color = rb.lcd_rgbpack(ru, gu, bu)
            else
                inc = inc or 1
                ru = ru + inc; gu = gu + inc; bu = bu + inc
                color = rb.lcd_rgbpack(ru, gu, bu)
            end
        else
            inc = inc or 1
            color = clamp_roll(current + inc, 0, maxstate)
        end

        return color
    end -- clrinc

    -- expose functions to the outside through _clr table
    _clr.set = clrset
    _clr.inc = clrinc
end -- color functions

return _clr
