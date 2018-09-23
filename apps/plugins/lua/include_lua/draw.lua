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
    _draw.flood_fill
    _draw.hline
    _draw.image
    _draw.line
    _draw.polygon
    _draw.polyline
    _draw.rect
    _draw.rect_filled
    _draw.rounded_rect
    _draw.rounded_rect_filled
    _draw.text
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
    local _LCD = rb.lcd_framebuffer()
    local LCD_W, LCD_H = rb.LCD_WIDTH, rb.LCD_HEIGHT
    local BSAND = 8 -- blits color to dst if src <> 0
    local _NIL = nil -- nil placeholder

    local _abs     = math.abs
    local _clear   = rocklib_image.clear
    local _copy    = rocklib_image.copy
    local _ellipse = rocklib_image.ellipse
    local _get     = rocklib_image.get
    local _line    = rocklib_image.line
    local _marshal = rocklib_image.marshal
    local _min     = math.min
    local _newimg  = rb.new_image
    local _points  = rocklib_image.points

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

    -- rectangle
    local function rect(img, x, y, width, height, color, bClip)
        if width == 0 or height == 0 then return end

        polyline(img, x, y, {{0, 0}, {width, 0}, {width, height}, {0, height}}, color, true, bClip)

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

    -- floods an area of targetclr with fillclr x, y specifies the start seed
    _draw.flood_fill = function(img, x, y, targetclr, fillclr)
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

    -- draws a closed figure based on points in t_points
    _draw.polygon = function(img, x, y, t_points, color, fillcolor, bClip)
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
            _draw.flood_fill(fill_img, fill_img:width(), fill_img:height() , 0x1, 0x0)
            _copy(img, fill_img, x - 1, y - 1, _NIL, _NIL, _NIL, _NIL, bClip, BSAND, fillcolor)
        end

        polyline(img, x, y, t_points, color, true, bClip)
    end

    -- draw text onto image if width/height are supplied text is centered
    _draw.text = function(img, x, y, width, height, font, color, text)
        font = font or rb.FONT_UI

        local opts = {x = 0, y = 0, width = LCD_W - 1, height = LCD_H - 1,
               font = font, drawmode = 3, fg_pattern = 0x1, bg_pattern = 0}

        if rb.LCD_DEPTH  == 2 then -- invert 2-bit screens
            --vp.drawmode = bit.bxor(vp.drawmode, 4)
            vp.fg_pattern = 3 - vp.fg_pattern
            vp.bg_pattern = 3 - vp.bg_pattern
        end
        rb.set_viewport(opts)

        local res, w, h = rb.font_getstringsize(text, font)

        if not width then
            width = 0
        else
            width = (width - w) / 2
        end

        if not height then
            height = 0
        else
            height = (height - h) / 2
        end

        -- make a copy of the current screen for later
        local screen_img = _newimg(LCD_W, LCD_H)
        _copy(screen_img, _LCD)

        -- check if the screen buffer is supplied image if so set img to the copy
        if img == _LCD then
            img = screen_img
        end

        -- we will be printing the text to the screen then blitting into img
        rb.lcd_clear_display()

        if w > LCD_W then -- text is too long for the screen do it in chunks
            local l = 1
            local resp, wp, hp
            local lenr = text:len()

            while lenr > 1 do
                l = lenr
                resp, wp, hp = rb.font_getstringsize(text:sub(1, l), font)

                while wp >= LCD_W and l > 1 do
                    l = l - 1
                    resp, wp, hp = rb.font_getstringsize(text:sub( 1, l), font)
                end

                rb.lcd_putsxy(0, 0, text:sub(1, l))
                text = text:sub(l)

                if x + width > img:width() or y + height > img:height() then
                    break
                end

                -- using the mask we made blit color into img
                _copy(img, _LCD, x + width, y + height, _NIL, _NIL, _NIL, _NIL, false, BSAND, color)
                x = x + wp
                rb.lcd_clear_display()
                lenr = text:len()
            end
        else --w <= LCD_W
            rb.lcd_putsxy(0, 0, text)

            -- using the mask we made blit color into img
            _copy(img, _LCD, x + width, y + height, _NIL, _NIL, _NIL, _NIL, false, BSAND, color)
        end

        _copy(_LCD, screen_img) -- restore screen
        rb.set_viewport() -- set viewport default
        return res, w, h
    end

    -- expose internal functions to the outside through _draw table
    _draw.hline    = hline
    _draw.vline    = vline
    _draw.polyline = polyline
    _draw.rect     = rect
    _draw.rounded_rect  = rounded_rect
end -- _draw functions

return _draw

