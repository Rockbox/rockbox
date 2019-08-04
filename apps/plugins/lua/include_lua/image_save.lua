--[[ Lua Image save
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
-- save(img, path/name)
-- bmp saving derived from rockbox - screendump.c
-- bitdepth is limited by the device
-- eg. device displays greyscale, rgb images are saved greyscale
if not rb.lcd_framebuffer then rb.splash(rb.HZ, "No Support!") return nil end

do
    local rocklib_image = getmetatable(rb.lcd_framebuffer())

    -- internal constants
    local _NIL = nil -- _NIL placeholder
    local _points  = rocklib_image.points

    -- saves img to file: name
    return function(img, name)
        local file
        local bbuffer = {} -- concat buffer for s_bytes
        local fbuffer = {} -- concat buffer for file writes, reused

        local function s_bytesLE(bits, value)
            -- bits must be multiples of 8 (sizeof byte)
            local byte
            local nbytes = bit.rshift(bits, 3)
            for b = 1, nbytes do
                if value > 0 then
                    byte  = value % 256
                    value = (value - byte) / 256
                else
                    byte = 0
                end
                bbuffer[b] = string.char(byte)
            end
            return table.concat(bbuffer, _NIL, 1, nbytes)
        end

        local function s_bytesBE(bits, value)
            -- bits must be multiples of 8 (sizeof byte)
            local byte
            local nbytes = bit.rshift(bits, 3)
            for b = nbytes, 1, -1 do
                if value > 0 then
                    byte  = value % 256
                    value = (value - byte) / 256
                else
                    byte = 0
                end
                bbuffer[b] = string.char(byte)
            end
            return table.concat(bbuffer, _NIL, 1, nbytes)
        end

        local cmp = {["r"] =  function(c) return bit.band(bit.rshift(c, 16), 0xFF) end,
                     ["g"] =  function(c) return bit.band(bit.rshift(c, 08), 0xFF) end,
                     ["b"] =  function(c) return bit.band(c, 0xFF) end}

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
        elseif depth <= 24 then
            bpp = 24
            bypl = (w * 3 + 3)
        else
            bpp = 32
            bypl = (w * 4 + 3)
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
            if format == 555 then
                -- red bitfield mask
                bmpheader[#bmpheader + 1] = s_bytesLE(32, 0x00007C00)
                -- green bitfield mask
                bmpheader[#bmpheader + 1] = s_bytesLE(32, 0x000003E0)
                -- blue bitfield mask
                bmpheader[#bmpheader + 1] = s_bytesLE(32, 0x0000001F)
			else --565
                -- red bitfield mask
                bmpheader[#bmpheader + 1] = s_bytesLE(32, 0x0000F800)
                -- green bitfield mask
                bmpheader[#bmpheader + 1] = s_bytesLE(32, 0x000007E0)
                -- blue bitfield mask
                bmpheader[#bmpheader + 1] = s_bytesLE(32, 0x0000001F)
            end
        end

        file:write(table.concat(fbuffer))-- write the header to the file now
        for i=1, #fbuffer do fbuffer[i] = _NIL end -- reuse table

        local imgdata = fbuffer
        -- pad rows to a multiple of 4 bytes
        local bytesleft = linebytes - (bytesperpixel * w)
        local t_data = {}
        local fs_bytes_E = s_bytesLE -- default save in Little Endian

        if format == 3553 then -- RGB565SWAPPED
            fs_bytes_E = s_bytesBE -- Saves in Big Endian
        end

        -- Bitmap lines start at bottom unless biHeight is negative
        for point in _points(img, 1, h, w + bytesleft, 1) do
            imgdata[#imgdata + 1] = fs_bytes_E(bpp, point or 0)

            if #fbuffer >= 31 then -- buffered write, increase # for performance
                file:write(table.concat(fbuffer))
                for i=1, #fbuffer do fbuffer[i] = _NIL end -- reuse table
            end

        end
        file:write(table.concat(fbuffer)) --write leftovers to file
        fbuffer = _NIL

        file:close()
    end -- save(img, name)
end
