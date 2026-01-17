--[[ Rockbox vector logo
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2026 William Wilgus
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
local _clr   = require("color") -- clrset, clrinc provides device independent colors
local _lcd   = require("lcd")   -- lcd helper functions
local actions = require("menubuttons")

local WHITE = _clr.set(-1, 255, 255, 255)
local BLACK = _clr.set(0, 0, 0, 0)
local YELLOW = _clr.set(BLACK, 255, 192, 0)
local GREY   = _clr.set(WHITE, 180, 195, 211)
if rb.LCD_DEPTH == 2 then --greyscale display
    YELLOW = _clr.set(2, 255, 192, 0)
    GREY   = _clr.set(1, 180, 195, 211)
end

if not rb.lcd_framebuffer then rb.splash(rb.HZ, "No Support!") return nil end

--poly draw from draw_poly.lua with negative (inverse) scaling added and optimizations
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

    local function scale_val_none(val, scale)
            return val
    end
    local function scale_val_up(val, scale)
            return val * scale
    end
    local function scale_val_dn(val, scale)
            return val / scale
    end

    local function get_scale_fn(scale)
        local scale_fn, fn
        if (scale < 0) then
            scale = -scale
            scale_fn = scale_val_dn
        elseif scale > 0 then
            scale_fn = scale_val_up
        else
            scale_fn = scale_val_none
        end
        return scale + 1, scale_fn
    end

    local function polyline_size_only(img, x, y, t_pts, scale_x, scale_y)
        scale_x = scale_x or 0
        scale_y = scale_y or 0
        local scale_val_x, scale_val_y

        scale_x, scale_val_x = get_scale_fn(scale_x)
        scale_y, scale_val_y = get_scale_fn(scale_y)

        local pt_first_last, pt1, pt2
        local max_x, max_y = 0, 0
        local len = #t_pts
        if len < 4 then error("not enough points", 3) end

        pt_first_last = {scale_val_x(t_pts[1], scale_x), scale_val_y(t_pts[2], scale_y)}

        pt2 = {scale_val_x(t_pts[1], scale_x), scale_val_y(t_pts[2], scale_y)}
        for i = 3, len + 2, 2 do
            pt1 = pt2
            if t_pts[i + 1] == nil then
                pt2 = pt_first_last
            else
                pt2 = {scale_val_x(t_pts[i], scale_x), scale_val_y(t_pts[i + 1], scale_y)}
            end-- first and last point
            if pt1[1] > max_x then max_x = pt1[1] end
            if pt1[2] > max_y then max_y = pt1[2] end
        end
        if pt2[1] > max_x then max_x = pt2[1] end
        if pt2[2] > max_y then max_y = pt2[2] end
        return max_x + x, max_y + y
    end

    -- draws a non-filled figure based on points in t-points
    local function polyline(img, x, y, t_pts, color, bClosed, bClip, scale_x, scale_y)
        local draw_fn = _line
        scale_x = scale_x or 1
        scale_y = scale_y or 1
        local scale_val_x, scale_val_y

        scale_x, scale_val_x = get_scale_fn(scale_x)
        scale_y, scale_val_y = get_scale_fn(scale_y)

        local pt_first_last, pt1, pt2

        local len = #t_pts
        if len < 4 then error("not enough points", 3) end

        if bClosed then
            pt_first_last = {scale_val_x(t_pts[1], scale_x), scale_val_y(t_pts[2], scale_y)}
        else
            pt_first_last = {scale_val_x(t_pts[len - 1], scale_x), scale_val_y(t_pts[len], scale_y)}
        end

        pt2 = {scale_val_x(t_pts[1], scale_x), scale_val_y(t_pts[2], scale_y)}
        for i = 3, len + 2, 2 do
            pt1 = pt2
            if t_pts[i + 1] == nil then
                pt2 = pt_first_last
            else
                pt2 = {scale_val_x(t_pts[i], scale_x), scale_val_y(t_pts[i + 1], scale_y)}
            end-- first and last point
            draw_fn(img, pt1[1] + x, pt1[2] + y, pt2[1] + x, pt2[2] + y, color, bClip)
        end
    end

    -- draws a closed figure based on points in t_pts
    _poly.polygon = function(img, x, y, t_pts, color, fillcolor, bClip, scale_x, scale_y)
        scale_x = scale_x or 1
        scale_y = scale_y or 1
        if #t_pts < 2 then error("not enough points", 3) end

        if fillcolor then
            flood_fill = flood_fill or require("draw_floodfill")
            local x_min, x_max = _lcd.W, 0
            local y_min, y_max = _lcd.H, 0
            local w, h = 0, 0
            local pt1, pt2
            -- find boundries of polygon
            for i = 1, #t_pts, 2 do
                if t_pts[i] < x_min then x_min = t_pts[i] end
                if t_pts[i] > x_max then x_max = t_pts[i] end

                if t_pts[i+1] < y_min then y_min = t_pts[i+1] end
                if t_pts[i+1] > y_max then y_max = t_pts[i+1] end
            end

            local scale
            if (scale_x < 0) then
                scale = -scale_x + 1
                x_max = x_max / scale
                x_min = x_min / scale
            else
                scale = scale_x + 1
                x_max = x_max * scale
                x_min = x_min * scale
            end

            if (scale_y < 0) then
                scale = -scale_y + 1
                y_max = y_max / -scale_y
                y_min = y_min / -scale_y
            else
                scale = scale_y + 1
                y_max = y_max * scale
                y_min = y_min * scale
            end

            w = _abs(x_max) + _abs(x_min)
            if w > _lcd.W then w = _lcd.W end

            h = _abs(y_max) + _abs(y_min)
            if h > _lcd.H then h = _lcd.H end

            if x_min < _lcd.W and y_min < _lcd.H then
                -- hack so we don't waste time drawing off screen
                x_min = 0
                y_min = 0
                x_min = -(x_min - 2)  -- leave a border to use flood_fill
                y_min = -(y_min - 2)

                local fill_img = _newimg(w + 3, h + 3)
                _clear(fill_img, 0x1)

                polyline(fill_img, x_min, y_min, t_pts, 0x0, true, bClip, scale_x, scale_y)
                -- flood the outside of the figure with 0 the inside will be fillcolor
                flood_fill(fill_img, fill_img:width(), fill_img:height() , 0x1, 0x0)
                _copy(img, fill_img, x - 1, y - 1,
                      _NIL, _NIL, _NIL, _NIL, bClip, BSAND, fillcolor)
            end

        end

        polyline(img, x, y, t_pts, color, true, bClip, scale_x, scale_y)
    end

    -- expose internal functions to the outside through _poly table
    _poly.polyline = polyline
    _poly.polyline_size_only = polyline_size_only
end

-- $Rb - Vectors, each is an (x,y) pair lines get drawn between them
local cross_left_pts = {8, 0, 8, 26, 8, 5, 4, 5, 36, 5}

local R_outline_pts = {12, 10, 43, 10, 47, 11, 50, 13, 54, 16, 56, 20,
                       58, 24, 58, 25, 59, 28, 59, 29, 60, 33, 60, 38,
                       60, 60, 57, 65, 54, 70, 50, 75, 50, 77, 70, 127,
                       50, 127, 34, 84, 27, 84, 26, 127, 12, 127, 12, 10}

local R_center_pts = {26, 31, 26, 63, 38, 63, 39, 61, 41, 60, 43, 58,
                      44, 55, 44, 39, 43, 35, 40, 32, 39, 31, 26, 31}

local R_shadow_pts = {26, 126, 35, 126, 32, 126, 32, 128, 32,
                      59, 37, 57, 40, 56, 41, 40, 38, 36, 27, 35}

local b_outline_pts = {59, 38, 79, 38, 79, 61, 83, 59, 87, 58, 90, 57, 98, 57,
                      101, 57, 105, 58, 110, 60, 114, 63, 117, 66, 121, 70, 124,
                       75, 126, 80, 127, 85, 128, 87, 128, 98, 127, 101, 126, 106,
                      124, 110, 121, 114, 115, 120, 106, 125, 100, 126, 96, 126,
                       88, 126, 82, 125, 78, 122, 76, 125, 59, 125, 59, 38}

local b_center_pts = {92, 77, 100, 77, 101, 78, 103, 78, 105, 80, 107, 81,
                     108, 83, 109, 84, 110, 86, 111, 89, 112, 90, 112, 95,
                     111, 97, 110, 100, 108, 103, 105, 105, 102, 106, 98,
                     107, 95, 107, 90, 106, 87, 104, 83, 100, 82, 97, 81,
                      94, 81, 88, 83, 85, 84, 83, 86, 81, 89, 79, 93, 77}

local clef_pts = {6, 46, 7, 44, 10, 44, 11, 45, 13, 46, 14, 47, 16, 49, 17,
                 50, 18, 52, 18, 54, 17, 59, 17, 60, 16, 62, 12, 75, 12, 85,
                 13, 87, 14, 88, 16, 90, 18, 91, 21, 91, 24, 91, 28, 89, 29,
                 86, 28, 82, 24, 79, 23, 80, 23, 84, 29, 106, 29, 114, 28,
                116, 25, 117, 24, 116, 23, 114, 23, 112, 25, 111, 26, 113,
                 27, 113, 27, 107, 26, 102, 24, 94, 20, 79, 16, 83, 15, 83,
                 20, 77, 6, 50, 6, 47, 9, 48, 11, 48, 13, 50, 15, 52, 15, 55,
                 14, 57, 9, 76, 9, 86, 10, 90, 13, 94, 19, 95, 23, 94, 29, 90,
                 31, 86, 31, 82, 29, 79, 26, 77, 24, 77, 22, 77, 8, 49}

local clef_void_1_pts = {13, 59, 8, 48, 13, 50, 14, 54, 14, 58}

local clef_void_2_pts = {15, 69, 13, 76, 12, 79, 12, 86, 13, 88, 17, 91, 21,
                         91, 23, 91, 20, 81, 16, 83, 15, 82, 18, 78, 16, 71}

local clef_void_3_pts = {23, 80, 26, 90, 28, 86, 28, 83, 27, 81, 23, 79}

local max_x = 1
local max_y = 1
--memorize the rotated points but let them still get collected if ram is needed
local rot_memoized = setmetatable({}, { __mode = 'k' })  --Keys are weak

local function rb_logo_sz(sx, sy)
    local x, y
    local t_pts = {cross_left_pts, R_outline_pts, R_shadow_pts, b_outline_pts, clef_pts}
    for k, points_t in pairs(t_pts) do
        x, y = _poly.polyline_size_only(_LCD, 0, 0, points_t, sx, sy)
        if x > max_x then max_x = x end
        if y > max_y then max_y = y end
    end
end

local function Rot(t_pts, rot)
    local count = #t_pts
    local nw = max_x
    local nh = max_y

    rot = rot % 7
    if rot == 0 then
        return t_pts
    end

    -- convert to string keys
    rot = tostring(rot)
    local pts = tostring(t_pts)

    if not rot_memoized[rot] then
        rot_memoized[rot] = {}
    end

    if not rot_memoized[rot][pts] then
        rot_memoized[rot][pts] = {}
    else
        return rot_memoized[rot][pts]
    end

    if rot == "1" then
        for i = 1, count, 2 do
            rot_memoized[rot][pts][count + 1 - i] = t_pts[i]
            rot_memoized[rot][pts][count + 1 - (i + 1)] = nw - t_pts[i+1]
        end
    elseif rot == "2" then
        for i = 1, count, 2 do
            rot_memoized[rot][pts][count + 1 - i] = nh - t_pts[i+1]
            rot_memoized[rot][pts][count + 1 - (i + 1)] = nw-t_pts[i]
        end
    elseif rot == "3" then
        for i = 1, count, 2 do
            rot_memoized[rot][pts][count + 1 - i] = nh - t_pts[i]
            rot_memoized[rot][pts][count + 1 - (i + 1)] = t_pts[i+1]
        end
    elseif rot == "4" then
        for i = 1, count, 2 do
            rot_memoized[rot][pts][count + 1 - i] = nh - t_pts[i+1]
            rot_memoized[rot][pts][count + 1 - (i + 1)] = t_pts[i]
        end
    elseif rot == "5" then
        for i = 1, count, 2 do
            rot_memoized[rot][pts][count + 1 - i] = nh - t_pts[i]
            rot_memoized[rot][pts][count + 1 - (i + 1)] = nw-t_pts[i+1]
        end
    elseif rot == "6" then
        for i = 1, count, 2 do
            rot_memoized[rot][pts][count + 1 - i] = t_pts[i+1]
            rot_memoized[rot][pts][count + 1 - (i + 1)] = nw-t_pts[i]
        end
    end

    return rot_memoized[rot][pts]
end
local count = -1

local function rb_logo_rot(x, y, sx, sy, rot)
    local polygon

    if count >= 0 then
        polygon = _poly.polygon
        _lcd:clear(BLACK)
        _poly.polyline(_LCD, x, y, Rot(cross_left_pts, rot), WHITE, false, true, sx, sy)
        _poly.polyline(_LCD, x, y, Rot(R_shadow_pts, rot), WHITE, false, true, sx, sy)
    else
       polygon = function() end
    end
    repeat
        polygon(_LCD, x, y, Rot(R_outline_pts, rot), BLACK, YELLOW, true, sx, sy)
        polygon(_LCD, x, y, Rot(R_center_pts, rot), BLACK, BLACK, true, sx, sy)
        polygon(_LCD, x, y, Rot(b_outline_pts, rot), BLACK, GREY, true, sx, sy)
        polygon(_LCD, x, y, Rot(b_center_pts, rot), BLACK, BLACK, true, sx, sy)

        polygon(_LCD, x, y, Rot(clef_pts, rot), WHITE, WHITE, true, sx, sy)
        polygon(_LCD, x, y, Rot(clef_void_1_pts, rot), WHITE, BLACK, true, sx, sy)
        polygon(_LCD, x, y, Rot(clef_void_2_pts, rot), WHITE, YELLOW, true, sx, sy)
        polygon(_LCD, x, y, Rot(clef_void_3_pts, rot), WHITE, YELLOW, true, sx, sy)
        rot = rot + 1
    until count >= 0 or rot >= 6;
end

local action
local redraw = true;
local rot = 0
local sx = -2
local sy = -2

-- we want the size of everything @ 1:1 scale so we can use that to flip/rotate the figure
rb_logo_sz(0, 0)

while true do

    if redraw then
        rb_logo_rot(0,0, sx, sy, rot)
        _lcd:update()
        if count > 0 then
            redraw = false
            local prot = (rot) % 7
            if prot > 0 then
                prot = prot - 1
                if prot == 0 then prot = 6 end
                rot_memoized[tostring(prot)] = nil
            end
        end
    end

    action = rb.get_plugin_action(rb.HZ/2, 1)
    if action == actions.CANCEL or action == actions.EXIT then
        break
    end

    redraw = redraw or (action ~= actions.NONE)

    if action == actions.LEFT then
        sx = sx - 1
    elseif action == actions.LEFTR then
        sx = sx - 1
        sy = sy - 1
    elseif action == actions.RIGHT then
        sx = sx + 1
    elseif action == actions.RIGHTR then
        sx = sx + 1
        sy = sy + 1
    elseif action == actions.UP then
        sy = sy - 1
    elseif action == actions.UPR then
        sy = sy - 1
        sx = sy
    elseif action == actions.DOWN then
        sy = sy + 1
    elseif action == actions.DOWNR then
        sy = sy + 1
        sx = sy
    elseif action == actions.SELR then
        rot = rot + 1
        count = 0;
        redraw = true
    elseif count > 5 then
        count = 0;
        rot = rot + 1
        redraw = true
    else
        count = count + 1
    end
end --wend

local used, allocd, free = rb.mem_stats()

collectgarbage("collect")
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
s_t[7] = "\n\nNote that the rockbox used count is a high watermark\n"
rb.splash_scroller(10 * rb.HZ, table.concat(s_t))
