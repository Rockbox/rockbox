--[[ Lua Poly Drawing functions
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
    _poly.polygon
    _poly.polyline
]]

if not rb.lcd_framebuffer then rb.splash(rb.HZ, "No Support!") return nil end

local _poly = {} do
    -- Internal Constants
    local rocklib_image = getmetatable(rb.lcd_framebuffer())
    local BSAND = 8 -- blits color to dst if src <> 0
    local _NIL = nil -- nil placeholder

    local _abs     = math.abs
    local _copy    = rocklib_image.copy
    local _line    = rocklib_image.line
    local flood_fill = require("draw_floodfill")

    -- draws a non-filled figure based on points in t-points
    local function polyline(img, x, y, t_points, color, bClosed, bClip)
        if #t_points < 2 then error("not enough points", 3) end

        local pt_first_last

        if bClosed then
            pt_first_last = t_points[1]
        else
            pt_first_last = t_points[#t_points]
        end

        for i = 1, #t_points, 1 do
            local pt1 = t_points[i]

            local pt2 = t_points[i + 1] or pt_first_last-- first and last point

            _line(img, pt1[1] + x, pt1[2] + y, pt2[1] + x, pt2[2] + y, color, bClip)
        end

    end

    -- draws a closed figure based on points in t_points
    _poly.polygon = function(img, x, y, t_points, color, fillcolor, bClip)
        if #t_points < 2 then error("not enough points", 3) end

        if fillcolor then
                local x_min, x_max = 0, 0
                local y_min, y_max = 0, 0
                local w, h = 0, 0
                -- find boundries of polygon
                for i = 1, #t_points, 1 do
                    local pt = t_points[i]
                    if pt[1] < x_min then x_min = pt[1] end
                    if pt[1] > x_max then x_max = pt[1] end
                    if pt[2] < y_min then y_min = pt[2] end
                    if pt[2] > y_max then y_max = pt[2] end
                end
                w = _abs(x_max) + _abs(x_min)
                h = _abs(y_max) + _abs(y_min)
                x_min = x_min - 2  -- leave a border to use flood_fill
                y_min = y_min - 2

                local fill_img = _newimg(w + 3, h + 3)
                _clear(fill_img, 0x1)

                for i = 1, #t_points, 1 do
                    local pt1 = t_points[i]
                    local pt2 = t_points[i + 1] or t_points[1]-- first and last point
                    _line(fill_img, pt1[1] - x_min, pt1[2] - y_min,
                                      pt2[1]- x_min, pt2[2] - y_min, 0)

                end
            flood_fill(fill_img, fill_img:width(), fill_img:height() , 0x1, 0x0)
            _copy(img, fill_img, x - 1, y - 1, _NIL, _NIL, _NIL, _NIL, bClip, BSAND, fillcolor)
        end

        polyline(img, x, y, t_points, color, true, bClip)
    end

    -- expose internal functions to the outside through _poly table
    _poly.polyline = polyline
end
return _poly
