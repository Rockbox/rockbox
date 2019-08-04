--[[ Lua Image functions
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

    _img.search
    _img.rotate
    _img.resize
    _img.tile
    _img.new
    _img.load

-- Exposed Constants
    _img.RLI_INFO_ALL
    _img.RLI_INFO_TYPE
    _img.RLI_INFO_WIDTH
    _img.RLI_INFO_HEIGHT
    _img.RLI_INFO_ELEMS
    _img.RLI_INFO_BYTES
    _img.RLI_INFO_DEPTH
    _img.RLI_INFO_FORMAT
    _img.RLI_INFO_ADDRESS

]]

--[[Other rbimage Functions:
--------------------------------------------------------------------------------
img:_len() or #img -- returns number of pixels in image

img:__tostring([item]) or tostring(img) -- returns data about the image item = 0
                                         is the same as tostring(img) otherwise
                                         item = 1 is the first item in list
                                         item = 7 is the 7th item
                                         item = 8 is the data address in hex
                                        -- See Constants _img.RLI_INFO_....

img:_data(element) -- returns/sets raw pixel data
                      NOTE!! this data is defined by the target and targets with
                      different color depth, bit packing, etc will not be
                      compatible with the same image's data on another target
]]
--------------------------------------------------------------------------------
if not rb.lcd_framebuffer then rb.splash(rb.HZ, "No Support!") return nil end

local _img = {} do

    local rocklib_image = getmetatable(rb.lcd_framebuffer())
    setmetatable(_img, rocklib_image)

    -- internal constants
    local _NIL = nil -- _NIL placeholder
    local _math = require("math_ex") -- math functions needed
    local LCD_W, LCD_H = rb.LCD_WIDTH, rb.LCD_HEIGHT

    local _copy    = rocklib_image.copy
    local _get     = rocklib_image.get
    local _marshal = rocklib_image.marshal
    local _points  = rocklib_image.points

    -- returns new image -of- img sized to fit w/h tiling to fit if needed
    _img.tile = function(img, w, h)
        local hs , ws = img:height(), img:width()
        local t_img = rb.new_image(w, h)

        for x = 1, w, ws do _copy(t_img, img, x, 1, 1, 1) end
        for y = hs, h, hs do _copy(t_img, t_img, 1, y, 1, 1, w, hs) end
        return t_img
    end

    -- resizes src to size of dst
    _img.resize = function(dst, src)
        -- simple nearest neighbor resize derived from rockbox - pluginlib_bmp.c
        -- pretty rough results highly recommend building one more suited..
        local dw, dh = dst:width(), dst:height()

        local xstep = (bit.lshift(src:width(), 8) / (dw)) + 1
        local ystep = (bit.lshift(src:height(), 8) / (dh))

        local xpos, ypos = 0, 0
        local src_x, src_y

        -- walk the dest get src pixel
        local function rsz_trans(val, x, y)
            if x == 1 then
                src_y = bit.rshift(ypos,8) + 1
                xpos = xstep - bit.rshift(xstep,4) + 1
                ypos = ypos + ystep;
            end
            src_x = bit.rshift(xpos,8) + 1
            xpos = xpos + xstep
            return (_get(src, src_x, src_y, true) or 0)
        end
        --/* (dst*, [x1, y1, x2, y2, dx, dy, clip, function]) */
        _marshal(dst, 1, 1, dw, dh, _NIL, _NIL, false, rsz_trans)
    end

    -- returns new image -of- img rotated in whole degrees 0 - 360
    _img.rotate = function(img, degrees)
        -- we do this backwards as if dest was the unrotated object
        degrees = 360 - degrees
        local c, s = _math.d_cos(degrees), _math.d_sin(degrees)

        -- get the center of the source image
        local s_xctr, s_yctr = img:width() / 2, img:height() / 2

        -- get the the new center of the dest image at rotation angle
        local d_xctr = ((math.abs(s_xctr * c) + math.abs(s_yctr * s))/ 10000) + 1
        local d_yctr = ((math.abs(s_xctr * s) + math.abs(s_yctr * c))/ 10000) + 1

        -- calculate size of rect new image will occupy
        local dw, dh = d_xctr * 2 - 1, d_yctr * 2 - 1

        local r_img = rb.new_image(dw, dh)
        -- r_img:clear() -- doesn't need cleared as we walk every pixel

        --[[rotation works on origin of 0,0 we need to offset to the center of the
            image and then place the upper left back at the origin (0, 0)]]
        --[[0,0|-----| ^<  |-------| v> 0,0|-------|
               |     |    |  0,0  |       |       |
               |_____|   |_______|       |_______|  ]]

        -- walk the dest get translated src pixel, oversamples src to fill gaps
        local function rot_trans(val, x, y)
            -- move center x/y to the origin
            local xtran = x - d_xctr;
            local ytran = y - d_yctr;

            -- rotate about the center of the image by x degrees
            local yrot = ((xtran * s) + (ytran * c)) / 10000 + s_yctr
            local xrot = ((xtran * c) - (ytran * s)) / 10000 + s_xctr
            -- upper left of src image back to origin, copy src pixel
            return _get(img, xrot, yrot, true) or 0
        end
        _marshal(r_img, 1, 1, dw, dh, _NIL, _NIL, false, rot_trans)
        return r_img
    end

    --searches an image for target color
    _img.search = function(img, x1, y1, x2, y2, targetclr, variation, stepx, stepy)

        if variation > 128 then variation = 128 end
        if variation < -128 then variation = -128 end

        local targeth = targetclr + variation
        local targetl = targetclr - variation

        if targeth < targetl then
            local swap = targeth
            targeth = targetl
            targetl = swap
        end

        for point, x, y in _points(img, x1, y1, x2, y2, stepx, stepy) do
            if point >= targetl and point <= targeth then
                return point, x, y
            end
        end
        return nil, nil, nil
    end

    --[[ we won't be extending these into RLI_IMAGE]]
    -- creates a new rbimage size w x h
    _img.new = function(w, h)
        return rb.new_image(w, h)
    end

    -- returns new image -of- file: name (_NIL if error)
    _img.load = function(name)
        return rb.read_bmp_file("/" .. name)
    end

    -- expose tostring constants to outside through _img table
    _img.RLI_INFO_ALL     = 0x0
    _img.RLI_INFO_TYPE    = 0x1
    _img.RLI_INFO_WIDTH   = 0x2
    _img.RLI_INFO_HEIGHT  = 0x3
    _img.RLI_INFO_ELEMS   = 0x4
    _img.RLI_INFO_BYTES   = 0x5
    _img.RLI_INFO_DEPTH   = 0x6
    _img.RLI_INFO_FORMAT  = 0x7
    _img.RLI_INFO_ADDRESS = 0x8
end -- _img functions

return _img

