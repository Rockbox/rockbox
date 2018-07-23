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

    _img.save
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

    -- internal constants
    local _NIL = nil -- _NIL placeholder
    local _math = require("math_ex") -- math functions needed
    local LCD_W, LCD_H = rb.LCD_WIDTH, rb.LCD_HEIGHT

    -- returns new image -of- img sized to fit w/h tiling to fit if needed
    local function tile(img, w, h)
        local hs , ws = img:height(), img:width()
        local t_img = rb.new_image(w, h)

        for x = 1, w, ws do t_img:copy(img, x, 1, 1, 1) end
        for y = hs, h, hs do t_img:copy(t_img, 1, y, 1, 1, w, hs) end
        return t_img
    end

    -- resizes src to size of dst
    local function resize(dst, src)
        -- simple nearest neighbor resize derived from rockbox - pluginlib_bmp.c
        -- pretty rough results highly recommend building one more suited..
        local dw, dh = dst:width(), dst:height()

        local xstep = (bit.lshift(src:width(),8) / (dw)) + 1
        local ystep = (bit.lshift(src:height(),8) / (dh))

        local xpos, ypos = 0, 0
        local src_x, src_y

        -- walk the dest get src pixel
        function rsz_trans(val, x, y)
            if x == 1 then
                src_y = bit.rshift(ypos,8) + 1
                xpos = xstep - bit.rshift(xstep,4) + 1
                ypos = ypos + ystep;
            end
            src_x = bit.rshift(xpos,8) + 1
            xpos = xpos + xstep
            return (src:get(src_x, src_y, true) or 0)
        end
        --/* (dst*, [x1, y1, x2, y2, dx, dy, clip, function]) */
        dst:marshal(1, 1, dw, dh, _NIL, _NIL, false, rsz_trans)
    end

    -- returns new image -of- img rotated in whole degrees 0 - 360
    local function rotate(img, degrees)
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
        function rot_trans(val, x, y)
            -- move center x/y to the origin
            local xtran = x - d_xctr;
            local ytran = y - d_yctr;

            -- rotate about the center of the image by x degrees
            local yrot = ((xtran * s) + (ytran * c)) / 10000 + s_yctr
            local xrot = ((xtran * c) - (ytran * s)) / 10000 + s_xctr
            -- upper left of src image back to origin, copy src pixel
            return img:get(xrot, yrot, true) or 0
        end
        r_img:marshal(1, 1, dw, dh, _NIL, _NIL, false, rot_trans)
        return r_img
    end

    -- saves img to file: name
    local function save(img, name)
        -- bmp saving derived from rockbox - screendump.c
        -- bitdepth is limited by the device
        -- eg. device displays greyscale, rgb images are saved greyscale
        local file

        local fbuffer = {} -- concat buffer for file writes, reused

        local function dump_fbuffer(thresh)
            if #fbuffer >= thresh then
                file:write(table.concat(fbuffer))
                for i=1, #fbuffer do fbuffer[i] = _NIL end -- reuse table
            end
        end

        local function s_bytesLE(bits, value)
            -- bits must be multiples of 8 (sizeof byte)
            local byte
            local result = ""
            for b = 1, bit.rshift(bits, 3) do
                if value > 0 then
                    byte  = value % 256
                    value = (value - byte) / 256
                    result = result .. string.char(byte)
                else
                    result = result .. string.char(0)
                end
            end
            return result
        end

        local function s_bytesBE(bits, value)
            -- bits must be multiples of 8 (sizeof byte)
            local byte
            local result = ""
            for b = 1, bit.rshift(bits, 3) do
                if value > 0 then
                    byte  = value % 256
                    value = (value - byte) / 256
                    result = string.char(byte) .. result
                else
                    result = string.char(0) .. result
                end
            end
            return result
        end

        local function c_cmp(color, shift)
            -- [RR][GG][BB]
            return bit.band(bit.rshift(color, shift), 0xFF)
        end

        local cmp = {["r"] =  function(c) return c_cmp(c, 16) end,
                     ["g"] =  function(c) return c_cmp(c, 08) end,
                     ["b"] =  function(c) return c_cmp(c, 00) end}

        local function bmp_color(color)
            return s_bytesLE(8, cmp.b(color))..
                   s_bytesLE(8, cmp.g(color))..
                   s_bytesLE(8, cmp.r(color))..
                   s_bytesLE(8, 0) .. ""
        end -- c_cmp(color, c.r))

        local function bmp_color_mix(c1, c2, num, den)
            -- mixes c1 and c2 as ratio of numerator / denominator
            -- used 2x each save results
            local bc1, gc1, rc1 = cmp.b(c1), cmp.g(c1), cmp.r(c1)

            return s_bytesLE(8, cmp.b(c2) - bc1 * num / den + bc1)..
                   s_bytesLE(8, cmp.g(c2) - gc1 * num / den + gc1)..
                   s_bytesLE(8, cmp.r(c2) - rc1 * num / den + rc1)..
                   s_bytesLE(8, 0) .. ""
        end

        local w, h = img:width(), img:height()
        local depth  = tonumber(img:__tostring(6)) -- RLI_INFO_DEPTH   = 0x6
        local format = tonumber(img:__tostring(7)) -- RLI_INFO_FORMAT  = 0x7

        local bpp, bypl -- bits per pixel, bytes per line
        -- bypl, pad rows to a multiple of 4 bytes
        if depth <= 4 then
            bpp = 8 -- 256 color image
            bypl = (w + 3)
        elseif depth <= 16 then
            bpp = 16
            bypl = (w * 2 + 3)
        else
            bpp = 24
            bypl = (w * 3 + 3)
        end

        local linebytes = bit.band(bypl, bit.bnot(3))

        local bytesperpixel = bit.rshift(bpp, 3)
        local headersz = 54
        local imgszpad = h * linebytes

        local compression, n_colors = 0, 0
        local h_ppm, v_ppm = 0x00000EC4, 0x00000EC4 --Pixels Per Meter ~ 96 dpi

        if depth == 16 then
            compression = 3 -- BITFIELDS
            n_colors    = 3
        elseif depth <= 8 then
            n_colors = bit.lshift(1, depth)
        end

        headersz = headersz + (4 * n_colors)

        file = io.open('/' .. name, "w+") -- overwrite, rb ignores the 'b' flag

        if not file then
            rb.splash(rb.HZ, "Error opening /" .. name)
            return
        end
        -- create a bitmap header 'rope' with image details -- concatenated at end
        local bmpheader = fbuffer

              bmpheader[01] = "BM"
              bmpheader[02] = s_bytesLE(32, headersz + imgszpad)
              bmpheader[03] = "\0\0\0\0" -- WORD reserved 1 & 2
              bmpheader[04] = s_bytesLE(32, headersz) -- BITMAPCOREHEADER size
              bmpheader[05] = s_bytesLE(32, 40) -- BITMAPINFOHEADER size

              bmpheader[06] = s_bytesLE(32, w)
              bmpheader[07] = s_bytesLE(32, h)
              bmpheader[08] = "\1\0" -- WORD color planes ALWAYS 1
              bmpheader[09] = s_bytesLE(16, bpp) -- bits/pixel
              bmpheader[10] = s_bytesLE(32, compression)
              bmpheader[11] = s_bytesLE(32, imgszpad)
              bmpheader[12] = s_bytesLE(32, h_ppm) -- biXPelsPerMeter
              bmpheader[13] = s_bytesLE(32, v_ppm) -- biYPelsPerMeter
              bmpheader[14] = s_bytesLE(32, n_colors)
              bmpheader[15] = s_bytesLE(32, n_colors)

        -- Color Table (#n_colors entries)
        if depth == 1 then -- assuming positive display
            bmpheader[#bmpheader + 1] = bmp_color(0xFFFFFF)
            bmpheader[#bmpheader + 1] = bmp_color(0x0)
        elseif depth == 2 then
            bmpheader[#bmpheader + 1] = bmp_color(0xFFFFFF)
            bmpheader[#bmpheader + 1] = bmp_color_mix(0xFFFFFF, 0, 1, 3)
            bmpheader[#bmpheader + 1] = bmp_color_mix(0xFFFFFF, 0, 2, 3)
            bmpheader[#bmpheader + 1] = bmp_color(0x0)
        elseif depth == 16 then
            -- red bitfield mask
            bmpheader[#bmpheader + 1] = s_bytesLE(32, 0x0000F800)
            -- green bitfield mask
            bmpheader[#bmpheader + 1] = s_bytesLE(32, 0x000007E0)
            -- blue bitfield mask
            bmpheader[#bmpheader + 1] = s_bytesLE(32, 0x0000001F)
        end

        dump_fbuffer(0) -- write the header to the file now

        local imgdata = fbuffer
        -- pad rows to a multiple of 4 bytes
        local bytesleft = linebytes - (bytesperpixel * w)
        local t_data = {}
        local fs_bytes_E = s_bytesLE -- default save in Little Endian

        if format == 3553 then -- RGB565SWAPPED
            fs_bytes_E = s_bytesBE -- Saves in Big Endian
        end

        -- Bitmap lines start at bottom unless biHeight is negative
        for point in img:points(1, h, w + bytesleft, 1) do
            imgdata[#imgdata + 1] = fs_bytes_E(bpp, point or 0)
            dump_fbuffer(31) -- buffered write, increase # for performance
        end

        dump_fbuffer(0) --write leftovers to file

        file:close()
    end -- save(img, name)

    --searches an image for target color
    local function search(img, x1, y1, x2, y2, targetclr, variation, stepx, stepy)

        if variation > 128 then variation = 128 end
        if variation < -128 then variation = -128 end

        local targeth = targetclr + variation
        local targetl = targetclr - variation

        if targeth < targetl then
            local swap = targeth
            targeth = targetl
            targetl = swap
        end

        for point, x, y in img:points(x1, y1, x2, y2, stepx, stepy) do
            if point >= targetl and point <= targeth then
                return point, x, y
            end
        end
        return nil, nil, nil
    end

    --[[ we won't be extending these into RLI_IMAGE]]
    -- creates a new rbimage size w x h
    local function new(w, h)
        return rb.new_image(w, h)
    end

    -- returns new image -of- file: name (_NIL if error)
    local function load(name)
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

    -- expose functions to the outside through _img table
    _img.save   = save
    _img.search = search
    _img.rotate = rotate
    _img.resize = resize
    _img.tile = tile

    -- adds the above _img functions into the metatable for RLI_IMAGE
    local ex = getmetatable(rb.lcd_framebuffer())
    for k, v in pairs(_img) do
        if ex[k] == _NIL then ex[k] = v end
    end
    -- not exposed through RLI_IMAGE
     _img.new    = new
     _img.load   = load

end -- _img functions

return _img

