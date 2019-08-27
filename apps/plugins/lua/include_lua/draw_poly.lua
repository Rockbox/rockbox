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
--[[
     every 2 elements in t_pts is an x, y coord pair
     p[?]  = {x,y,x,y,x,y}
     lines get drawn between the coords
]]
if not rb.lcd_framebuffer then rb.splash(rb.HZ, "No Support!") return nil end

local _poly = {} do
    -- Internal Constants
    local rocklib_image = getmetatable(rb.lcd_framebuffer())
    local BSAND = 8 -- blits color to dst if src <> 0
    local _NIL = nil -- nil placeholder

    local _abs     = math.abs
    local _clear   = rocklib_image.clear
    local _copy    = rocklib_image.copy
    local _line    = rocklib_image.line
    local _newimg  = rb.new_image
    local flood_fill

    -- draws a non-filled figure based on points in t-points
    local function polyline(img, x, y, t_pts, color, bClosed, bClip, scale_x, scale_y)
        scale_x = scale_x or 1
        scale_y = scale_y or 1

        local pt_first_last, pt1, pt2
        local max_x, max_y = 0, 0
        local len = #t_pts
        if len < 4 then error("not enough points", 3) end

        if bClosed then
            pt_first_last = {t_pts[1] * scale_x, t_pts[2] * scale_y}
        else
            pt_first_last = {t_pts[len - 1] * scale_x, t_pts[len] * scale_y}
        end

        pt2 = {t_pts[1] * scale_x, t_pts[2] * scale_y}
        for i = 3, len + 2, 2 do
            pt1 = pt2
            if t_pts[i + 1] == nil then 
                pt2 = pt_first_last
            else
                pt2 = {t_pts[i] * scale_x, t_pts[i + 1] * scale_y}
            end-- first and last point

            _line(img, pt1[1] + x, pt1[2] + y, pt2[1] + x, pt2[2] + y, color, bClip)
            if pt1[1] > max_x then max_x = pt1[1] end
            if pt1[2] > max_y then max_y = pt1[2] end
        end
        if pt2[1] > max_x then max_x = pt2[1] end
        if pt2[2] > max_y then max_y = pt2[2] end
        return max_x + x, max_y + y
    end

    -- draws a closed figure based on points in t_pts
    _poly.polygon = function(img, x, y, t_pts, color, fillcolor, bClip, scale_x, scale_y)
        scale_x = scale_x or 1
        scale_y = scale_y or 1
        if #t_pts < 2 then error("not enough points", 3) end

        if fillcolor then
            flood_fill = flood_fill or require("draw_floodfill")
            local x_min, x_max = 0, 0
            local y_min, y_max = 0, 0
            local w, h = 0, 0
            local pt1, pt2
            -- find boundries of polygon
            for i = 1, #t_pts, 2 do
                if t_pts[i] < x_min then x_min = t_pts[i] end
                if t_pts[i] > x_max then x_max = t_pts[i] end

                if t_pts[i+1] < y_min then y_min = t_pts[i+1] end
                if t_pts[i+1] > y_max then y_max = t_pts[i+1] end
            end
            x_max = x_max * scale_x
            x_min = x_min * scale_x
            y_max = y_max * scale_y
            y_min = y_min * scale_y
            w = _abs(x_max) + _abs(x_min)
            h = _abs(y_max) + _abs(y_min)
            x_min = -(x_min - 2)  -- leave a border to use flood_fill
            y_min = -(y_min - 2)

            local fill_img = _newimg(w + 3, h + 3)
            _clear(fill_img, 0x1)

            polyline(fill_img, x_min, y_min, t_pts,
                     0x0, true, bClip, scale_x, scale_y)
            -- flood the outside of the figure with 0 the inside will be fillcolor
            flood_fill(fill_img, fill_img:width(), fill_img:height() , 0x1, 0x0)
            _copy(img, fill_img, x - 1, y - 1,
                  _NIL, _NIL, _NIL, _NIL, bClip, BSAND, fillcolor)
        end

        polyline(img, x, y, t_pts, color, true, bClip, scale_x, scale_y)
    end

    -- expose internal functions to the outside through _poly table
    _poly.polyline = polyline
end
return _poly
