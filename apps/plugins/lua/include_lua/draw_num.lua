--[[ Lua draw number function
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

--[[ Exposed Functions
    _draw_nums.print; binary (base = 2) , octal (base = 8), hexadecimal (base = 16)
    _draw_nums.nums; table of number characters
]]

if not rb.lcd_framebuffer then rb.splash(rb.HZ, "No Support!") return nil end

local _draw_nums = {} do
    local _poly = require "draw_poly"

        -- every 2 elements is an x, y coord pair
        -- n[?]  = {x,y,x,y,x,y}
    local nums = {
        ["b"] = {0,1,0,7,0,5,1,4,1,4,2,4,3,5,3,6,2,7,1,7,0,6,0,5},
        ["o"] = {1,4,0,5,0,6,1,7,2,7,3,6,3,5,2,4,1,4},
        ["x"] = {0,3,4,7,2,5,4,3,0,7},
        [-1] = {1,4, 3,4},
        [0] = {1,2,1,6,2,7,3,7,4,6,4,2,3,1,2,1,1,2},
        [1] = {3,1,3,7},
        [2] = {1,1,3,1,4,2,4,3,3,4,1,5,1,7,4,7},
        [3] = {1,1,3,1,4,2,4,3,3,4,2,4,3,4,4,5,4,6,3,7,1,7},
        [4] = {1,1,1,3,2,4,4,4,4,1,4,7},
        [5] = {1,1,4,1,1,1,1,4,3,4,4,5,4,7,1,7},
        [6] = {1,2,1,4,1,6,2,7,3,7,4,6,4,4,1,4,1,2,2,1,4,1},
        [7] = {1,1,4,1,4,2,1,7},
        [8] = {1,2,1,6,2,7,3,7,4,6,4,4,1,4,4,4,4,2,3,1,2,1,1,2},
        [9] = {4,6,4,4,4,2,3,1,2,1,1,2,1,4,4,4,4,6,3,7,1,7},
        [10] = {1,7,1,4,4,4,4,7,4,2,3,1,2,1,1,2,1,4},
        [11] = {1,1,1,7,3,7,4,6,4,5,3,4,1,4,3,4,4,3,4,2,3,1,1,1},
        [12] = {4,2,3,1,2,1,1,2,1,6,2,7,3,7,4,6},
        [13] = {1,1,1,7,3,7,4,6,4,2,3,1,1,1},
        [14] = {4,1,1,1,1,4,3,4,1,4,1,7,4,7},
        [15] = {4,1,1,1,1,4,3,4,1,4,1,7},
    }
    _draw_nums.nums = nums


    _draw_nums.print = function(img, num, x, y, chrw, color, base, prefix, bClip, scale_x, scale_y, t_nums)
        scale_x = scale_x or 1
        scale_y = scale_y or 1
        chrw = chrw * scale_x
        prefix = (prefix == nil or prefix == true) and true or false
        t_nums = t_nums or nums
        local max_x, max_y, digits = 0, 0, {}

        if num <= 0 then
            if num < 0 then
                digits[-3] = -1
                num = -num
            else
                digits[0] = 0
            end
        end

        if not prefix and (base == 2 or base == 8 or base == 16) then
            -- no prefix
        elseif base == 2 then
            digits[-1] = "b"
        elseif base == 8 then
            digits[-1] = "o"
        elseif base == 10 then
            -- no prefix
        elseif base == 16 then
            digits[-2] = 0
            digits[-1] = "x"
        elseif base == nil then -- default
            base = 10
        else
            error("unknown number base: " .. base)
            return nil
        end

        while num > 0 do -- get each digit (LeastSignificant)
            digits[#digits + 1] = num % base;
            num=num/base;
        end

        digits[#digits + 1] = digits[0]  -- zero
        digits[#digits + 1] = digits[-1] -- base prefix
        digits[#digits + 1] = digits[-2] -- base prefix (hex)
        digits[#digits + 1] = digits[-3] -- neg sign

        for i = #digits, 1, -1 do
                max_x, max_y = _poly.polyline(img, x, y, t_nums[digits[i]],
                                          color, false, bClip, scale_x, scale_y)
                x = x + chrw
        end

        return x, y + max_y, chrw
    end
end
return _draw_nums
