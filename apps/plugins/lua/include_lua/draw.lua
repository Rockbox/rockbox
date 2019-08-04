--[[ Lua Drawing functions
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

    _draw.circle
    _draw.circle_filled
    _draw.ellipse
    _draw.ellipse_filled
    _draw.ellipse_rect_filled
    _draw.ellipse_rect
    _draw.hline
    _draw.image
    _draw.line
    _draw.polygon
    _draw.polyline
    _draw.rect
    _draw.rect_filled
    _draw.rounded_rect
    _draw.rounded_rect_filled
    _draw.vline

]]

--[[ bClip allows drawing out of bounds without raising an error it is slower
     than having a correctly bounded figure, but can be helpful in some cases..
]]

if not rb.lcd_framebuffer then rb.splash(rb.HZ, "No Support!") return nil end

local _draw = {} do

    local rocklib_image = getmetatable(rb.lcd_framebuffer())
    setmetatable(_draw, rocklib_image)

    -- Internal Constants
    local BSAND = 8 -- blits color to dst if src <> 0
    local _NIL = nil -- nil placeholder


    local _clear   = rocklib_image.clear
    local _copy    = rocklib_image.copy
    local _ellipse = rocklib_image.ellipse
    local _get     = rocklib_image.get
    local _line    = rocklib_image.line
    local _min     = math.min
    local _newimg  = rb.new_image

    -- line
    _draw.line = function(img, x1, y1, x2, y2, color, bClip)
        _line(img, x1, y1, x2, y2, color, bClip)
    end

    -- horizontal line; x, y define start point; length in horizontal direction
    local function hline(img, x, y , length, color, bClip)
        _line(img, x, y, x + length, _NIL, color, bClip)
    end

    -- vertical line; x, y define start point; length in vertical direction
    local function vline(img, x, y , length, color, bClip)
        _line(img, x, y, _NIL, y + length, color, bClip)
    end

    -- draws a non-filled rect based on points in t-points
    local function polyrect(img, x, y, t_points, color, bClip)
        local pt_first_last = t_points[1]
        local pt1, pt2
        for i = 1, 4, 1 do
            pt1 = t_points[i]
            pt2 = t_points[i + 1] or pt_first_last-- first and last point
            _line(img, pt1[1] + x, pt1[2] + y, pt2[1] + x, pt2[2] + y, color, bClip)
        end
    end

    -- rectangle
    local function rect(img, x, y, width, height, color, bClip)
        if width == 0 or height == 0 then return end

        polyrect(img, x, y, {{0, 0}, {width, 0}, {width, height}, {0, height}}, color, bClip)

    end

    -- filled rect, fillcolor is color if left empty
    _draw.rect_filled = function(img, x, y, width, height, color, fillcolor, bClip)
        if width == 0 or height == 0 then return end

        if not fillcolor then
            _clear(img, color, x, y, x + width, y + height, bClip)
        else
            _clear(img, fillcolor, x, y, x + width, y + height, bClip)
            rect(img, x, y, width, height, color, bClip)
        end
    end

    -- circle cx,cy define center point
    _draw.circle = function(img, cx, cy, radius, color, bClip)
        local r = radius
        _ellipse(img, cx - r, cy - r, cx + r, cy + r, color, _NIL,  bClip)
    end

    -- filled circle cx,cy define center, fillcolor is color if left empty
    _draw.circle_filled = function(img, cx, cy, radius, color, fillcolor, bClip)
        fillcolor = fillcolor or color
        local r = radius
        _ellipse(img, cx - r, cy - r, cx + r, cy + r, color, fillcolor, bClip)
    end

    -- ellipse that fits into defined rect
    _draw.ellipse_rect = function(img, x1, y1, x2, y2, color, bClip)
        _ellipse(img, x1, y1, x2, y2, color, _NIL, bClip)
    end

    --ellipse that fits into defined rect, fillcolor is color if left empty
    _draw.ellipse_rect_filled = function(img, x1, y1, x2, y2, color, fillcolor, bClip)
        if not fillcolor then fillcolor = color end

        _ellipse(img, x1, y1, x2, y2, color, fillcolor, bClip)
    end

    -- ellipse cx, cy define center point; a, b the major/minor axis
    _draw.ellipse = function(img, cx, cy, a, b, color, bClip)
        _ellipse(img, cx - a, cy - b, cx + a, cy + b, color, _NIL, bClip)
    end

    -- filled ellipse cx, cy define center point; a, b the major/minor axis
    -- fillcolor is color if left empty
    _draw.ellipse_filled = function(img, cx, cy, a, b, color, fillcolor, bClip)
        if not fillcolor then fillcolor = color end

        _ellipse(img, cx - a, cy - b, cx + a, cy + b, color, fillcolor, bClip)
    end

    -- rounded rectangle
    local function rounded_rect(img, x, y, w, h, radius, color, bClip)
        local c_img
        if w == 0 or h == 0 then return end

        -- limit the radius of the circle otherwise it will overtake the rect
        radius = _min(w / 2, radius)
        radius = _min(h / 2, radius)

        local r = radius

        c_img = _newimg(r * 2 + 1, r * 2 + 1)
        _clear(c_img, 0)
        _ellipse(c_img, 1, 1, 1 + r + r, 1 + r + r, 0x1, _NIL, bClip)

        -- copy 4 pieces of circle to their respective corners
        _copy(img, c_img, x, y, _NIL, _NIL, r + 1, r + 1, bClip, BSAND, color) --TL
        _copy(img, c_img, x + w - r - 2, y, r, _NIL, r + 1, r + 1, bClip, BSAND, color) --TR
        _copy(img, c_img, x , y + h - r - 2, _NIL, r, r + 1, _NIL, bClip, BSAND, color) --BL
        _copy(img, c_img, x + w - r - 2, y + h - r - 2, r, r, r + 1, r + 1, bClip, BSAND, color)--BR
        c_img = _NIL

        vline(img, x, y + r, h - r * 2, color, bClip);
        vline(img, x + w - 1, y + r, h - r * 2, color, bClip);
        hline(img, x + r, y, w - r * 2, color, bClip);
        hline(img, x + r, y + h - 1, w - r * 2, color, bClip);
    end

    -- rounded rectangle fillcolor is color if left empty
    _draw.rounded_rect_filled = function(img, x, y, w, h, radius, color, fillcolor, bClip)
        local c_img

        if w == 0 or h == 0 then return end

        if not fillcolor then fillcolor = color end

        -- limit the radius of the circle otherwise it will overtake the rect
        radius = _min(w / 2, radius)
        radius = _min(h / 2, radius)

        local r = radius

        c_img = _newimg(r * 2 + 1, r * 2 + 1)
        _clear(c_img, 0)
        _ellipse(c_img, 1, 1, 1 + r + r, 1 + r + r, 0x1, 0x1, bClip)

        -- copy 4 pieces of circle to their respective corners
        _copy(img, c_img, x, y, _NIL, _NIL, r + 1, r + 1, bClip, BSAND, fillcolor) --TL
        _copy(img, c_img, x + w - r - 2, y, r, _NIL, r + 1, r + 1, bClip, BSAND, fillcolor) --TR
        _copy(img, c_img, x, y + h - r - 2, _NIL, r, r + 1, _NIL, bClip, BSAND, fillcolor) --BL
        _copy(img, c_img, x + w - r - 2, y + h - r - 2, r, r, r + 1, r + 1, bClip, BSAND, fillcolor) --BR
        c_img = _NIL

        -- finish filling areas circles didn't cover
        _clear(img, fillcolor, x + r, y, x + w - r, y + h - 1, bClip)
        _clear(img, fillcolor, x, y + r, x + r, y + h - r, bClip)
        _clear(img, fillcolor, x + w - r, y + r, x + w - 1, y + h - r - 1, bClip)

        if fillcolor ~= color then
            rounded_rect(img, x, y, w, h, r, color, bClip)
        end
    end

    -- draws an image at xy coord in dest image
    _draw.image = function(dst, src, x, y, bClip)
        if not src then --make sure an image was passed, otherwise bail
            rb.splash(rb.HZ, "No Image!")
            return _NIL
        end

        _copy(dst, src, x, y, 1, 1, _NIL, _NIL, bClip)
    end

    -- expose internal functions to the outside through _draw table
    _draw.hline    = hline
    _draw.vline    = vline
    _draw.rect     = rect
    _draw.rounded_rect  = rounded_rect
end -- _draw functions

return _draw

