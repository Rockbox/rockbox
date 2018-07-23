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

    -- Internal Constants
    local _LCD = rb.lcd_framebuffer()
    local LCD_W, LCD_H = rb.LCD_WIDTH, rb.LCD_HEIGHT
    local BSAND = 8 -- blits color to dst if src <> 0
    local _NIL = nil -- nil placeholder

    local function set_viewport(vp)
        if not vp then rb.set_viewport() return end
        if rb.LCD_DEPTH  == 2 then -- invert 2-bit screens
            --vp.drawmode = bit.bxor(vp.drawmode, 4)
            vp.fg_pattern = 3 - vp.fg_pattern
            vp.bg_pattern = 3 - vp.bg_pattern
        end
        rb.set_viewport(vp)
    end

    -- line
    local function line(img, x1, y1, x2, y2, color, bClip)
        img:line(x1, y1, x2, y2, color, bClip)
    end

    -- horizontal line; x, y define start point; length in horizontal direction
    local function hline(img, x, y , length, color, bClip)
        img:line(x, y, x + length, _NIL, color, bClip)
    end

    -- vertical line; x, y define start point; length in vertical direction
    local function vline(img, x, y , length, color, bClip)
        img:line(x, y, _NIL, y + length, color, bClip)
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

            img:line(pt1[1] + x, pt1[2] + y, pt2[1]+x, pt2[2]+y, color, bClip)
        end

    end

    -- rectangle
    local function rect(img, x, y, width, height, color, bClip)
        if width == 0 or height == 0 then return end

        local ppt = {{0, 0}, {width, 0}, {width, height}, {0, height}}
        polyline(img, x, y, ppt, color, true, bClip)
        --[[
        vline(img, x, y, height, color, bClip);
        vline(img, x + width, y, height, color, bClip);
        hline(img, x, y, width, color, bClip);
        hline(img, x, y + height, width, color, bClip);]]
    end

    -- filled rect, fillcolor is color if left empty
    local function rect_filled(img, x, y, width, height, color, fillcolor, bClip)
        if width == 0 or height == 0 then return end

        if not fillcolor then
            img:clear(color, x, y, x + width, y + height, bClip)
        else
            img:clear(fillcolor, x, y, x + width, y + height, bClip)
            rect(img, x, y, width, height, color, bClip)
        end
    end

    -- circle cx,cy define center point
    local function circle(img, cx, cy, radius, color, bClip)
        local r = radius
        img:ellipse(cx - r, cy - r, cx + r, cy + r, color, _NIL,  bClip)
    end

    -- filled circle cx,cy define center, fillcolor is color if left empty
    local function circle_filled(img, cx, cy, radius, color, fillcolor, bClip)
        fillcolor = fillcolor or color
        local r = radius
        img:ellipse(cx - r, cy - r, cx + r, cy + r, color, fillcolor, bClip)
    end

    -- ellipse that fits into defined rect
    local function ellipse_rect(img, x1, y1, x2, y2, color, bClip)
        img:ellipse(x1, y1, x2, y2, color, _NIL, bClip)
    end

    --ellipse that fits into defined rect, fillcolor is color if left empty
    local function ellipse_rect_filled(img, x1, y1, x2, y2, color, fillcolor, bClip)
        if not fillcolor then fillcolor = color end

        img:ellipse(x1, y1, x2, y2, color, fillcolor, bClip)
    end

    -- ellipse cx, cy define center point; a, b the major/minor axis
    local function ellipse(img, cx, cy, a, b, color, bClip)
        img:ellipse(cx - a, cy - b, cx + a, cy + b, color, _NIL, bClip)
    end

    -- filled ellipse cx, cy define center point; a, b the major/minor axis
    -- fillcolor is color if left empty
    local function ellipse_filled(img, cx, cy, a, b, color, fillcolor, bClip)
        if not fillcolor then fillcolor = color end

        img:ellipse(cx - a, cy - b, cx + a, cy + b, color, fillcolor, bClip)
    end

    -- rounded rectangle
    local function rounded_rect(img, x, y, w, h, radius, color, bClip)
        local c_img

        local function blit(dx, dy, sx, sy, ox, oy)
            img:copy(c_img, dx, dy, sx, sy, ox, oy, bClip, BSAND, color)
        end

        if w == 0 or h == 0 then return end

        -- limit the radius of the circle otherwise it will overtake the rect
        radius = math.min(w / 2, radius)
        radius = math.min(h / 2, radius)

        local r = radius

        c_img = rb.new_image(r * 2 + 1, r * 2 + 1)
        c_img:clear(0)
        circle(c_img, r + 1, r + 1, r, 0xFFFFFF)

        -- copy 4 pieces of circle to their respective corners
        blit(x, y, _NIL, _NIL, r + 1, r + 1) --TL
        blit(x + w - r - 2, y, r, _NIL, r + 1, r + 1) --TR
        blit(x , y + h - r - 2, _NIL, r, r + 1, _NIL) --BL
        blit(x + w - r - 2, y + h - r - 2, r, r, r + 1, r + 1)--BR
        c_img = _NIL

        vline(img, x, y + r, h - r * 2, color, bClip);
        vline(img, x + w - 1, y + r, h - r * 2, color, bClip);
        hline(img, x + r, y, w - r * 2, color, bClip);
        hline(img, x + r, y + h - 1, w - r * 2, color, bClip);
    end

    -- rounded rectangle fillcolor is color if left empty
    local function rounded_rect_filled(img, x, y, w, h, radius, color, fillcolor, bClip)
        local c_img

        local function blit(dx, dy, sx, sy, ox, oy)
            img:copy(c_img, dx, dy, sx, sy, ox, oy, bClip, BSAND, fillcolor)
        end

        if w == 0 or h == 0 then return end

        if not fillcolor then fillcolor = color end

        -- limit the radius of the circle otherwise it will overtake the rect
        radius = math.min(w / 2, radius)
        radius = math.min(h / 2, radius)

        local r = radius

        c_img = rb.new_image(r * 2 + 1, r * 2 + 1)
        c_img:clear(0)
        circle_filled(c_img, r + 1, r + 1, r, fillcolor)

        -- copy 4 pieces of circle to their respective corners
        blit(x, y, _NIL, _NIL, r + 1, r + 1) --TL
        blit(x + w - r - 2, y, r, _NIL, r + 1, r + 1) --TR
        blit(x, y + h - r - 2, _NIL, r, r + 1, _NIL) --BL
        blit(x + w - r - 2, y + h - r - 2, r, r, r + 1, r + 1) --BR
        c_img = _NIL

        -- finish filling areas circles didn't cover
        img:clear(fillcolor, x + r, y, x + w - r, y + h - 1, bClip)
        img:clear(fillcolor, x, y + r, x + r, y + h - r, bClip)
        img:clear(fillcolor, x + w - r, y + r, x + w - 1, y + h - r - 1, bClip)

        if fillcolor ~= color then
            rounded_rect(img, x, y, w, h, r, color, bClip)
        end
    end

    -- draws an image at xy coord in dest image
    local function image(dst, src, x, y, bClip)
        if not src then --make sure an image was passed, otherwise bail
            rb.splash(rb.HZ, "No Image!")
            return _NIL
        end

        dst:copy(src, x, y, 1, 1, _NIL, _NIL, bClip)
    end

    -- floods an area of targetclr with fillclr x, y specifies the start seed
    function flood_fill(img, x, y, targetclr, fillclr)
        -- scanline 4-way flood algorithm
        --          ^
        -- <--------x--->
        --          v
        -- check that target color doesn't = fill and the first point is target color
        if targetclr == fillclr or targetclr ~= img:get(x,y, true) then return end
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
        local iter_n; -- North iteration
        local iter_s; -- South iteration

        local function check_ns(val, x, y)
            if targetclr == val then
                if targetclr == iter_n() then
                    qtail = qtail + 2
                    qpt[qtail - 1] = x
                    qpt[qtail] = (y - 1)
                end

                if targetclr == iter_s() then
                    qtail = qtail + 2
                    qpt[qtail - 1] = x
                    qpt[qtail] = (y + 1)
                end
                return fillclr
            end
            return _NIL -- signal marshal to stop
        end

        local function seed_pt(x, y)
            -- will never hit max_w * max_h^2 but make sure not to end early
            for qhead = 2, max_w * max_h * max_w * max_h, 2 do

                if targetclr == img:get(x, y, true) then
                    iter_n = img:points(x, y - 1, 1, y - 1)
                    iter_s = img:points(x, y + 1, 1, y + 1)
                    img:marshal(x, y, 1, y, _NIL, _NIL, true, check_ns)

                    iter_n = img:points(x + 1, y - 1, max_w, y - 1)
                    iter_s = img:points(x + 1, y + 1, max_w, y + 1)
                    img:marshal(x + 1, y, max_w, y, _NIL, _NIL, true, check_ns)
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
    local function polygon(img, x, y, t_points, color, fillcolor, bClip)
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
                w = math.abs(x_max) + math.abs(x_min)
                h = math.abs(y_max) + math.abs(y_min)
                x_min = x_min - 2  -- leave a border to use flood_fill
                y_min = y_min - 2

                local fill_img = rb.new_image(w + 3, h + 3)
                fill_img:clear(0xFFFFFF)

                for i = 1, #t_points, 1 do
                    local pt1 = t_points[i]
                    local pt2 = t_points[i + 1] or t_points[1]-- first and last point
                    fill_img:line(pt1[1] - x_min, pt1[2] - y_min,
                                      pt2[1]- x_min, pt2[2] - y_min, 0)

                end
            flood_fill(fill_img, fill_img:width(), fill_img:height() , 1, 0)
            img:copy(fill_img, x - 1, y - 1, _NIL, _NIL, _NIL, _NIL, bClip, BSAND, fillcolor)
        end

        polyline(img, x, y, t_points, color, true, bClip)
    end

    -- draw text onto image if width/height are supplied text is centered
    local function text(img, x, y, width, height, font, color, text)
        font = font or rb.FONT_UI

        local opts = {x = 0, y = 0, width = LCD_W - 1, height = LCD_H - 1,
               font = font, drawmode = 3, fg_pattern = 0xFFFFFF, bg_pattern = 0}
        set_viewport(opts)

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
        local screen_img = rb.new_image(LCD_W, LCD_H)
        screen_img:copy(_LCD)

        -- check if the screen buffer is supplied image if so set img to the copy
        if img == _LCD then
            img = screen_img
        end

        -- we will be printing the text to the screen then blitting into img
        rb.lcd_clear_display()

        local function blit(dx, dy)
            img:copy(_LCD, dx, dy, _NIL, _NIL, _NIL, _NIL, false, BSAND, color)
        end

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
                blit(x + width, y + height)
                x = x + wp
                rb.lcd_clear_display()
                lenr = text:len()
            end
        else --w <= LCD_W
            rb.lcd_putsxy(0, 0, text)

            -- using the mask we made blit color into img
            blit(x + width, y + height)
        end

        _LCD:copy(screen_img) -- restore screen
        set_viewport() -- set viewport default
        return res, w, h
    end

    -- expose functions to the outside through _draw table
    _draw.image    = image
    _draw.text     = text
    _draw.line     = line
    _draw.hline    = hline
    _draw.vline    = vline
    _draw.polygon  = polygon
    _draw.polyline = polyline
    _draw.rect     = rect
    _draw.circle   = circle
    _draw.ellipse  = ellipse
    _draw.flood_fill    = flood_fill
    _draw.ellipse_rect  = ellipse_rect
    _draw.rounded_rect  = rounded_rect
    -- filled functions use color as fillcolor if fillcolor is left empty...
    _draw.rect_filled          = rect_filled
    _draw.circle_filled        = circle_filled
    _draw.ellipse_filled       = ellipse_filled
    _draw.ellipse_rect_filled  = ellipse_rect_filled
    _draw.rounded_rect_filled  = rounded_rect_filled

    -- adds the above _draw functions into the metatable for RLI_IMAGE
    local ex = getmetatable(rb.lcd_framebuffer())
    for k, v in pairs(_draw) do
        if ex[k] == _NIL then
            ex[k] = v
        end
    end

end -- _draw functions

return _draw

