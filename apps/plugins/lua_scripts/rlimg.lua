--[[
             __________               __   ___.
   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
                     \/            \/     \/    \/            \/
 $Id$
 Example Lua RBIMAGE script
 Copyright (C) 2009 by Maurus Cuelenaere -- some prior work
 Copyright (C) 2017 William Wilgus
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 KIND, either express or implied.
]]--

require("actions")   -- Contains rb.actions & rb.contexts
-- require("buttons") -- Contains rb.buttons -- not needed for this example

local _math  = require("math_ex") -- missing math sine cosine, sqrt, clamp functions
local _timer = require("timer")
local _clr   = require("color") -- clrset, clrinc provides device independent colors
local _lcd   = require("lcd")   -- lcd helper functions
local _print = require("print") -- advanced text printing
local _img      = require("image") -- image manipulation save, rotate, resize, tile, new, load
local _img_save = require("image_save")
local _blit     = require("blit") -- handy list of blit operations
local _draw           = require("draw") -- draw all the things (primitives)
local _draw_floodfill = require("draw_floodfill")
local _draw_text      = require("draw_text")

--local scrpath = rb.current_path()"

--package.path = scrpath .. "/?.lua;" .. package.path --add lua_scripts directory to path

require("printmenus") --menu

--[[ RBIMAGE library functions
NOTE!! on x, y coordinates + width & height
----------------------------------------------
When making a new image you specify the width and height say (10, 20).
Intutively (at least to me) (0,0) (offset addressing) would reference the first
pixel (top left) and the last pixel (bottom, right) would be (w-1, h-1) or (9, 19)
but the original rbimage library (like lua) uses 1-based arrays (ordinal addressing)
for the image data so the first pixel is (1,1) and the last pixel is (w, h)
this presents a bit of a problem when interfacing with the internal rockbox
functions as you must remeber to convert all lua coordinates to 0 based coordinates.
I have opted to not change this in the name of compatibility with old scripts

NOTE2!! on width & height versus offset_x & offset_y :copy()
------------------------------------------------------------------------
The only place where RBIMAGE deviates from (ordinal addressing) is in the blit
/ copy function sx, sy and dx, dy still refer to the same pixel as all the
other functions but offset_x, and offset_y are zero based
Example (assuming same sized images)
dst:copy(src, 1,1, 1,1, 0, 0)
would copy the first pixel from src to the first pixel of dst
dst:copy(src, 1,1, 1,1, dst:width()-1, dst:height()-1) would copy all of src to dst
this might be a bit confusing but makes reversing images (negative offsets) easier.
Since offsets are auto calculated if empty or out of range you could call it as
dst:copy(src, 1,1, 1,1, dst:width(), dst:height()) or even
dst:copy(src, 1,1, 1,1) if you prefered and get the same result
]]

--'CONSTANTS' (in lua there really is no such thing as all vars are mutable)
--------------------------------------------------------
--colors for fg/bg ------------------------
local WHITE = _clr.set(-1, 255, 255, 255)
local BLACK = _clr.set(0, 0, 0, 0)
local RED   = _clr.set(WHITE, 255)
local GREEN = _clr.set(WHITE, 0, 255)
local BLUE  = _clr.set(WHITE, 0, 0, 255)
-------------------------------------------
local clrs
local CANCEL_BUTTON = rb.actions.PLA_CANCEL

-- EXAMPLES ---------------------------------------------------------------------- EXAMPLES---------------------------------------------------------------------
function my_blit(dst_val, dx, dy, src_val, sx, sy)
    -- user defined blit operation
    --this function gets called for every pixel you specify
    --you may change pixels in both the source and dest image
    --return nil to stop early

    if _lcd.DEPTH < 2 then
        return src_val
    end

    if dst_val == WHITE and bit.band(dx, 1) == 1 and bit.band(dy, 1) == 1 then
        return BLACK;
    elseif dst_val == WHITE then
        return src_val
    end

    return dst_val
end

function create_logo()
    --[[creates a small logo from data array]]

    -- moves scope of white and black from global to local
    local WHITE = WHITE
    local BLACK = BLACK

    --[[small array with 16 bits of data per element (16-bits in y direction)
        while the number of elements (32) is the x direction.
        in other words 16 rows X 32 columns, totally abritrary actually
        you could easily rotate or flip it by changing the for loops below ]]
    local logo = {0xFFFF, 0xFFFF, 0x8001, 0x8001, 0x9E7F, 0x9E7F, 0x9E7F, 0x9E7F,
                  0x9E3F, 0x9E3F, 0x804F, 0x804F, 0xC0E1, 0xC0F1, 0xFFFD, 0xFFFF,
                  0xFFFF, 0xFFFF, 0x8001, 0x8001, 0xFF01, 0xFF01, 0xFEFB, 0xFEFB,
                  0xFDFD, 0xFDFD, 0xFDFD, 0xFCF9, 0xFE03, 0xFE03, 0xFFFF, 0xFFFF}

    local img, img1, img2, img3

    img = _img.new(_lcd.W, _lcd.H)
    img:clear(BLACK) --[[always clear an image after you create it if you
                         intend to do any thing besides copy something
                         entirely to it as there is no guarantee what
                         state the data within is in, it could be all
                         0's or it could be every digit of PI ]]

    -- check for an error condition bail if error
    if(not img or not logo) then
        return nil
    end

    local logosz = table.getn(logo)
    local bits = 16 -- each element contains 16 pixels
    local data = 0

    for i=1, math.min(logosz, _lcd.W)  do
        for j=0, math.min(bits, _lcd.H) do

            if bit.band(bit.lshift(1, bits - j), logo[i]) > 0 then
                data = WHITE
            else
                data = BLACK
            end
            -- Set the pixel at position i, j+1 to the copied data
            img:set(i, j + 1, data)
        end
    end

    -- make a new image the size of our generated logo
    img1 = _img.new(logosz + 1, bits + 1)

    -- img.copy(dest, source, dx, dy, sx, sy, [w, h])
    img1:copy(img, 1, 1, 1, 1)

    --[[lua does auto garbage collection, but it is still
        a good idea to set large items to nil when done anyways]]
    img = nil

    local sl -- new image size
    if _lcd.W < _lcd.H then
        sl = _lcd.W / 3
    else
        sl = _lcd.H / 3
    end

    -- make sl always even by subtracting 1 if needed
    sl = bit.band(sl, bit.bnot(1))
    if sl < 16 then
        sl = 16
    end

    img2 = _img.new(sl, sl)
    --img2:clear(BLACK) -- doesn't need cleared since we copy to it entirely

    --[[we are going to resize the image since the data supplied is 32 x 16
        pixels its really tiny on most screens]]
    _img.resize(img2, img1)

    img1 = nil

    img3 = _img.new(sl, sl)
    img3:clear(BLACK)

    if IS_COLOR_TARGET == true then
        local c_yellow = _clr.set(WHITE, 0xFC, 0xC0, 0x00)
        local c_grey = _clr.set(WHITE, 0xD4, 0xE3, 0xF3)
        local c_dkgrey = _clr.set(WHITE, 0xB4, 0xC3, 0xD3)
        -- if dest pixel == source pixel make dest pixel yellow
        img3:copy(img2, 1, 1, 1, 1, nil, nil, false, _blit.BDEQS, c_yellow)
        -- xor src pixel to dest pixel if both are 0 or both are 1 dest = 0
        img2:copy(img3, 1, 1, 2, 1, nil, nil, false, _blit.BXOR)
        -- if dest pixel color > src pixel color copy grey to dest
        img2:copy(img3, 1, 1, 1, 1, nil, nil, false, _blit.BDGTS, c_grey)
        -- set img3 to grey
        img3:clear(c_dkgrey)
    end

    -- make a WHITE square in the middle

    img3:clear(WHITE, 4,4, img3:width() - 3, img3:height() - 3)

    img3:copy(img2, 1, 1, 1, 1, nil, nil, false, _blit.CUSTOM, my_blit)
    img2 = nil
    _img_save(img3, "pix.bmp")
    return img3
end -- copy_logo

-- draws an olive erm ball and returns it
function create_ball()
    local sl -- image size
    if _lcd.W < _lcd.H then
        sl = _lcd.W / 5
    else
        sl = _lcd.H / 5
    end

    -- make sl always even by subtracting 1 if needed
    sl = bit.band(sl, bit.bnot(1))
    if sl < 16 then
        sl = 16
    end
    local img = _img.new(sl, sl)
    img:clear(0)
    _draw.circle_filled(img, sl/2, sl/2, sl/2 - 1, _clr.set(-1, 255), _clr.set(0, 100))
    _draw.circle_filled(img, sl/3, sl/3, sl/10, _clr.set(-1, 255, 0, 0))
    return img
end

-- bounces img around on screen
function bounce_image(img)
    local timer = _timer() -- creates a new timer -- saves index
    local wait
    -- make a copy of the current screen for later
    local screen_img = _lcd:duplicate()

    local img_sqy = _img.new(img:width() + img:width() / 8, img:height())
    local img_sqx = _img.new(img:width(), img:height() + img:height() / 8)
    _img.resize(img_sqy, img)
    _img.resize(img_sqx, img)

    -- moves definition of CANCEL_BUTTON from global to local
    local CANCEL_BUTTON = CANCEL_BUTTON
--------------------------------------------------------
    local imgn = img
    local hold = 0
    local sx = 1  -- starting x
    local sy = 1  -- starting y

    local ex = _lcd.W - img:width() - 2
    local ey = _lcd.H - img:width() - 2

    -- threshold resets speed, inverts image
    local tx = ex / 5
    local ty = ey / 5

    local last_x = sx
    local last_y = sy

    local x = sx
    local y = sy

    local dx = 1
    local dy = 1
    -- negative width\height cause the image to be drawn from the opposite end
    local fx = _lcd.W
    local fy = _lcd.H

    local function invert_images()
        img:invert();
        img_sqx:invert()
        img_sqy:invert()
    end

    local loops = (_lcd.W * _lcd.H) / 2
    while (loops > 0) do

        if IS_COLOR_TARGET then
            if bit.band(loops, 128) == 128 then
                _lcd:copy(imgn, x, y, 1, 1, fx, fy, false, _blit.BOR)
                _lcd:copy(screen_img, x, y, x, y, imgn:width(), imgn:height(),
                                         false, _blit.BDEQC, imgn:get(1,1))
            else
                _lcd:copy(imgn, x, y, 1, 1, fx, fy, false, _blit.BSNEC, imgn:get(1,1))
            end
        else
            local blitop

            if imgn:get(1,1) ~= 0 then
                blitop = _blit.BSNOT
            else
                blitop = _blit.BXOR
            end

            _lcd:copy(imgn, x, y, 1, 1, fx, fy, false, blitop, WHITE)
        end

        if hold < 1 then
            imgn = img
        else
            hold = hold - 1
        end
        _lcd:update()

        x = x + dx
        y = y + dy

        if y >= ey or y <= sy then
            dy = (-dy)
            fy = (-fy)
            imgn = img_sqy
            hold = 3
            if dx < 0 and y < 10 then
                dx = dx - 5
            end
            if dx > tx then
                dx = 5;
                dy = 5;
                invert_images()
            end
        end

        if x >= ex or x <= sx then
            dx = (-dx)
            fx = (-fx)
            imgn = img_sqx
            hold = 3
            if dy < 0 and x < 10 then
                dy = dy - 5
            end
            if dy > ty then
                dx = 5;
                dy = 5;
                invert_images()
            end
        end

        x = _math.clamp(x, sx, ex)
        y = _math.clamp(y, sy, ey)

        -- copy original image back to screen
        _lcd:copy(screen_img)

        loops = loops -1

        wait = timer:check(true) --checks timer and updates last time
        if wait >= 5 then
            wait = 0
        elseif wait < 5 then
            wait = 5 - wait
        end
        -- 0 = timeout immediately
        -- ( -1 would be never timeout, and >0 is amount of 'ticks' before timeout)
        if rb.get_plugin_action(wait) == CANCEL_BUTTON then
            break;
        end
    end

    timer:stop() -- destroys timer, also returns time since last time

    -- leave the screen how we found it
    _lcd:copy(screen_img)
end -- image_bounce

-- draws a gradient using available colors
function draw_gradient(img, x, y, w, h, direction, clrs)
    x = x or 1
    y = y or 1
    w = w or img:width() - x
    h = h or img:height() - y
    local zstep = 0
    local step = 1
    if IS_COLOR_TARGET == true then -- Only do this when we're on a color target
        local z = 1
        local c = 1
        clrs = clrs or {255,255,255}
        local function gradient_rgb(p, c1, c2)
                -- tried squares of difference but blends were very abrupt
                local r = c1.r + (p * (c2.r - c1.r) / 10500)
                local g = c1.g + (p * (c2.g - c1.g) / 10000)
                local b = c1.b + (p * (c2.b - c1.b) / 09999)
            return _clr.set(nil, r, g, b)
        end
        local function check_z()
               if z > 10000 and c < #clrs - 1 then
                    z = 1
                    c = c + 1
                elseif z > 10000 then
                    z = 10000
                end
        end
        if direction == "V" then
            zstep = math.max(1, (10000 / ((w - 1) / (#clrs - 1)) + bit.band(#clrs, 1)))
            for i=x, w + x do
                check_z()
                _draw.vline(img, i, y, h, gradient_rgb(z, clrs[c], clrs[c + 1]))
                z = z + zstep
            end
        else
            zstep = math.max(1, (10000 / ((h - 1) / (#clrs - 1)) + bit.band(#clrs, 1)))
            for j=y, h + y do
                check_z()
                _draw.hline(img, x, j, w, gradient_rgb(z, clrs[c], clrs[c + 1]))
                z = z + zstep
            end
        end
    else -- not a color target but might be greyscale
        local clr = _clr.set(-1)
        for i=x, w + x do
            for j=y, h + y do
                -- Set the pixel at position i, j to the specified color
                img:set(i, j, clr)
            end
                clr = _clr.inc(clr, 1)
        end
    end
--rb.sleep(1000)
end -- draw_gradient

function twist(img)
--[[    local fg
    if rb.lcd_get_foreground then
        fg = rb.lcd_get_foreground()
    end]]

    local ims  = {}
    ims.strip  = _img.tile(img, img:width(), _lcd.H + img:height())
    ims.y_init = {img:height(), 1}
    ims.y_finl = {1, img:height()}
    ims.y_inc  = {-2, 4}
    ims.y_pos  = nil

    -- together define the width of the overall object
    local x_off=(_lcd.W/2)
    local m = _lcd.W
    local z = -m
    local zi = 1

    -- angles of rotation for each leaf
    local ANGLES = {0, 45, 90, 135, 180, 225, 270, 315}
    local elems = #ANGLES
    local _XCOORD = {}

    -- color alternates each leaf
    local colors = { WHITE, _clr.set(0, 0, 0, 0) }
    local c = 0

    -- calculated position of each point in the sine wave(s)
    local xs, xe

    --[[--Profiling code
    local timer = _timer.start()]]

    for rot = 0, 6000, 4 do
        _lcd:clear(BLACK)

        for y=1, _lcd.H do

            local function get_sines()
                local sc = m + z
                if sc == 0 then
                    sc = 1 -- prevent divide by 0
                elseif sc + z > _lcd.W then
                    zi = -1
                    colors[2] = _clr.inc(colors[2], 1, 0, 50, 0)
                elseif sc + z < -(_lcd.W) then
                    zi = 1
                    colors[1] = _clr.inc(colors[1], -1, 0, 10, 0)
                end
                    if colors[2] == colors[1] then
                        colors[2] = _clr.inc(colors[2], 1)
                    end
                for j = 1, elems do
                    _XCOORD[j] = _math.d_sin(y + ANGLES[j] + rot) / sc + x_off
                end
                _XCOORD[0] = _XCOORD[elems] -- loop table for efficient wrap
            end

            get_sines()
            for k = 1, elems do
                xs = _XCOORD[k]
                xe = _XCOORD[(k+1) % elems]
                if xs < 1 or xe > _lcd.W  then
                    while xs < 1 or xe > _lcd.W do
                        m = m + 1 -- shift m in order to scale object min/max
                        get_sines()
                        xs = _XCOORD[k]
                        xe = _XCOORD[(k+1) % elems]
                    end
                end
                c = (c % 2) + 1
                if  xs < xe then
                    -- defines the direction of leaves & fills them

                    _lcd:set(xs, y, colors[c])
                    _lcd:set(xe, y, colors[c])
                    _lcd:line(xs + 1, y, xe - 1, y, colors[3 - c], true)
                end
            end

        end

        do -- stripes and shifts image strips; blits it into the colors(1) leaves
            local y
            local y_col = 1
            local w = ims.strip:width() - 1
            local h = ims.strip:height() - 1
            if ims.y_pos ~= nil then
                for i = 1, #ims.y_pos do
                    ims.y_pos[i] = ims.y_pos[i] + ims.y_inc[i]

                    if (ims.y_inc[i] > 0 and ims.y_pos[i] > ims.y_finl[i])
                    or (ims.y_inc[i] < 0 and ims.y_pos[i] < ims.y_finl[i]) then
                        ims.y_pos[i] = ims.y_init[i]
                    end
                end
            else
                ims.y_pos = {ims.y_init[1], ims.y_init[2]}
            end

            for ix = 1, _lcd.W, w do
                y_col = y_col + 1
                y = ims.y_pos[(y_col % 2) + 1]
                if _lcd.DEPTH > 1 then
                    _lcd:copy(ims.strip, ix, 1, 1, y, w, h, false, _blit.BDEQC, colors[1])
                else
                    _lcd:copy(ims.strip, ix, 1, 1, y, w, h, false, _blit.BSAND)
                end
            end
        end

        _lcd:update()
        z = z + zi

        if rb.get_plugin_action(0) == CANCEL_BUTTON then
            break
        end
    collectgarbage("step")
    end
    --[[--Profiling code
    _print.f("%d", _timer.stop(timer))
    rb.sleep(rb.HZ * 10)]]
end -- twist

function draw_target(img)

    local clr = _clr.set(0, 0, 0, 0)

    -- make a copy of original screen for restoration
    local screen_img = _lcd:duplicate()

    rad_m = math.min(_lcd.W, _lcd.H)

    for s = -_lcd.W /4, 16 do
        img:copy(screen_img)
        s = math.max(1, math.abs(s))
        for r = 1, rad_m /2 - 10, s do
            clr = _clr.inc(clr, 1, r * 5, r * 10, r * 20)
            _draw.circle(img, _lcd.CX, _lcd.CY, r, clr)
        end

        _lcd:update()
        if rb.get_plugin_action( 20) == CANCEL_BUTTON then
            z = 16;
            break;
        end
    end

end -- draw_target

function draw_sweep(img, cx, cy, radius, color)
    local timer = _timer() --creates a new timer saves index
    local wait
    local x
    local y
    --make a copy of original screen for restoration
    local screen_img = _lcd:duplicate()
    _draw.circle(img, cx, cy, radius, color)
    for d = 630, 270, - 5  do
        if d % 45 == 0 then
            img:copy(screen_img)
            _draw.circle(img, cx, cy, radius, color)
            l = 0
        end
        x = cx + radius * _math.d_cos(d) / 10000
        y = cy + radius * _math.d_sin(d) / 10000


        _draw.line(img, cx, cy, x, y, color)
        l = l + 1
        if l > 1 then
        x1 = cx + (radius - 1) * _math.d_cos(d + 1) / 10000
        y1 = cy + (radius - 1) * _math.d_sin(d + 1) / 10000
        _draw_floodfill(img, x1, y1, BLACK, color)
        end
        _lcd:update()
        wait = timer:check(true) --checks timer and updates last time
        if wait >= 50 then
            wait = 0
        elseif wait < 50 then
            wait = 50 - wait
        end
        if rb.get_plugin_action( wait) == CANCEL_BUTTON then
            break
        end
    end
    timer:stop() --destroys timer, also returns time since last time
    screen_img = nil
end -- draw_sweep

function rotate_image(img)
    local blitop = _blit.BOR
    local i = 1
    local d = 0
    local ximg
    local x, y, w, h, xr, yr

    ximg = _img.rotate(img, 45) -- image will be largest at this point
    w = ximg:width()
    h = ximg:height()
    xr = (_lcd.W - w) / 2
    yr = (_lcd.H - h) / 2
    --make a copy of original screen for restoration
    local screen_img -- = _lcd:duplicate()
    screen_img =_img.new(w, h)
    screen_img :copy(_LCD, 1, 1, xr, yr, w, h)
    --_print.f("CW")

    --[[--Profiling code
    local timer = _timer.start()]]

    while d >= 0 do
        ximg = _img.rotate(img, d)
        w = ximg:width()
        h = ximg:height()
        x = (_lcd.W - w) / 2
        y = (_lcd.H - h) / 2

        -- copy our rotated image onto the background
        _lcd:copy(ximg, x, y, 1, 1, w, h, false, blitop)
        _lcd:update()
        --restore the portion of the background we destroyed
        _lcd:copy(screen_img, xr, yr, 1, 1)

        if d > 0 and d % 360 == 0 then
            _lcd:copy(ximg, x, y, 1, 1, w, h)
            _lcd:update()
            if i == 1 then i = 0 end
            if d == 1440 or i < 0 then
                if i < 0 then
                    i = i - 5
                else
                    i = - 5
                end
                blitop = _blit.BXOR
                --_print.f("CCW")
                --rb.sleep(rb.HZ)
            else
                i = i + 5
                --_print.f("CW")
                --rb.sleep(rb.HZ)
            end

        end
        d = d + i

        if rb.get_plugin_action(0) == CANCEL_BUTTON then
            break;
        end
    end

    _lcd:copy(ximg, x, y, 1, 1, w, h)
    --[[-- Profiling code
    _print.f("%d", _timer.stop(timer))
    rb.sleep(rb.HZ * 10)]]
end -- rotate_image

-- shows blitting with a mask
function blit_mask(dst)
    local timer = _timer()
    local r = math.min(_lcd.CX, _lcd.CY) / 5

    local bmask = _img.new(_lcd.W, _lcd.H)
    bmask:clear(0)

    _draw.circle_filled(bmask, _lcd.CX, _lcd.CY, r, WHITE)
    local color = _clr.set(0, 0, 0 ,0)
    for z = 0, 100 do
        z = z + timer:check(true)
        color = _clr.inc(color, 1, z * 5, z * 10, z * 20)
        dst:copy(bmask, 1, 1, 1, 1, nil, nil, false, _blit.BSAND, color)
        _lcd:update()

        if rb.get_plugin_action(0) == CANCEL_BUTTON then
            break
        end
    end
end -- blit_mask

-- draws an X on the screen
function draw_x()
    _draw.line(_LCD, 1, 1, _lcd.W, _lcd.H, WHITE)
    _draw.line(_LCD, _lcd.W, 1, 1, _lcd.H, WHITE)

    _draw.hline(_LCD, 10, _lcd.CY , _lcd.W - 21, WHITE)

    _draw.vline(_LCD, _lcd.CX, 20 , _lcd.H - 41, WHITE)

    _draw.rect(_LCD, _lcd.CX - 17, _lcd.CY - 17, 34, 34, WHITE)
    _lcd:update()
    rb.sleep(100)
end -- draw_x

--fills an image with random colors
function random_img(img)
    local min = _clr.set(0, 0, 0, 0)
    local max = _clr.set(-1, 255, 255, 255)
    math.randomseed(rb.current_tick())
    for x = 1, img:width() do
        for y = 1, img:height() do
            img:set(x, y, math.random(min, max))
        end
    end
end -- random_img

function rainbow_img(img)
--draw a gradient using available colors
    if IS_COLOR_TARGET == true then
    --R O Y G B I V
        clrs = {
                {r = 255, g = 255, b = 255},  -- white
                {r = 000, g = 000, b = 000},  -- black
                {r = 200, g = 000, b = 000},  -- red
                {r = 255, g = 000, b = 000},  -- red
                {r = 255, g = 100, b = 033},  -- orange
                {r = 255, g = 127, b = 000},  -- orange
                {r = 255, g = 200, b = 033},  -- yellow
                {r = 255, g = 255, b = 000},  -- yellow
                {r = 050, g = 255, b = 000},  -- green
                {r = 000, g = 125, b = 125},  -- green
                {r = 000, g = 000, b = 255},  -- blue
                {r = 033, g = 025, b = 200},  -- indigo
                {r = 075, g = 000, b = 150},  -- indigo
                {r = 127, g = 000, b = 150},  -- violet
                {r = 150, g = 000, b = 255},  -- violet
                {r = 255, g = 255, b = 255},  -- white
                {r = 000, g = 000, b = 000},  -- black
               }
    else
    end
        draw_gradient(img, 1, 1, nil, nil, "V", clrs)
end -- rainbow_img

-- draws a rounded rectangle with text
function rock_lua()
    local res, w, h = _lcd:text_extent("W", rb.FONT_UI)
    w = _lcd.W - 10
    h = h + 4
    _draw.rounded_rect_filled(_LCD, 5, 5, w, h, 15, WHITE)
    _draw_text(_LCD, 5, 5, w, h, nil, BLACK, "ROCKlua!")
    _lcd:update()
    rb.sleep(100)
end -- rock_lua

-- draws a rounded rectangle with long text
function long_text()
    local txt = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"
    local res, w, h = _lcd:text_extent(txt, rb.FONT_UI)
    local resp, wp, hp = _lcd:text_extent(" ", rb.FONT_UI)
    local wait = 0
    w = w + wp * 3
    h = h + 4
    local img = _img.new(w + 1, h)
    img:clear(BLACK)
    _draw.rounded_rect_filled(img, 1, 1, w, h, 15, WHITE)
    _draw_text(img, 1, 2, nil, nil, nil, BLACK, txt)

    for p = -w + 1, w - 1 do
        wait = 0
        p = math.abs(p)
        _lcd:copy(img, 1, _lcd.H - h, w - p, 1)
        _lcd:update()
        if p == 0 or w - p == 1 then wait = 100; rb.sleep(50) end
        if rb.get_plugin_action(wait) == CANCEL_BUTTON then
            break
        end
    end

end -- long_text

-- creates or loads an image to use as logo
function get_logo()
    local logo_img = _img.load("/pix.bmp") --loads the previously saved image (if we saved it before)

    --create a logo if an image wasn't saved previously
    if(not logo_img) then
        logo_img = create_logo()
    end

    --fallback
    if(not logo_img) then
        -- Load a BMP file in the variable backdrop
        local base = "/.rockbox/icons/"
        local backdrop = _img.load("/image.bmp") --user supplied image
                  or _img.load(base .. "tango_small_viewers.bmp")
                  or _img.load(base .. "tango_small_viewers_mono.bmp")
                  or _img.load(base .. "tango_small_mono.bmp")
                  or _img.load(base .. "tango_icons.16x16.bmp")
                  or _img.load(base .. "tango_icons_viewers.16x16.bmp")
                  or _img.load(base .. "viewers.bmp")
                  or _img.load(base .. "viewers.6x8x1.bmp")
                  or _img.load(base .. "viewers.6x8x2.bmp")
                  or _img.load(base .. "viewers.6x8x10.bmp")
                  or _img.load(base .. "viewers.6x8x16.bmp")
        logo_img = backdrop
    end
    return logo_img
end -- get_logo

-- uses print_table to display a menu
function main_menu()

    local mt =  {
                [1] = "Rocklua RLI Example",
                [2] = "The X",
                [3] = "Blit Mask",
                [4] = "Target",
                [5] = "Sweep",
                [6] = "Bouncing Ball (olive)",
                [7] = "The Twist",
                [8] = "Image Rotation",
                [9] = "Long Text",
                [10] = "Rainbow Image",
                [11] = "Random Image",
                [12] = "Clear Screen",
                [13] = "Save Screen",
                [14] = "Exit"
                }
    local ft =  {
                [0] = exit_now, --if user cancels do this function
                [1] = function(TITLE) return true end, -- shouldn't happen title occupies this slot
                [2] = function(THE_X) draw_x() end,
                [3] = function(BLITM) blit_mask(_lcd()) end,
                [4] = function(TARGT) draw_target(_lcd()) end,
                [5] = function(SWEEP)
                        local r = math.min(_lcd.CX, _lcd.CY) - 20
                        draw_sweep(_lcd(), _lcd.CX, _lcd.CY, r, _clr.set(-1, 0, 255, 0))
                      end,
                [6]  = function(BOUNC) bounce_image(create_ball()) end,
                [7]  = function(TWIST) twist(get_logo()) end,
                [8]  = function(ROTAT) rotate_image(get_logo()) end,
                [9]  = long_text,
                [10] = function(RAINB)
                         rainbow_img(_lcd()); _lcd:update(); rb.sleep(rb.HZ)
                       end,
                [11] = function(RANDM)
                         random_img(_lcd()); _lcd:update(); rb.sleep(rb.HZ)
                       end,
                [12] = function(CLEAR) _lcd:clear(BLACK); rock_lua() end,
                [13] = function(SAVEI) _LCD:invert(); _img_save(_LCD, "/rocklua.bmp") end,
                [14] = function(EXIT_) return true end
                }

    if _lcd.DEPTH < 2 then
        table.remove(mt, 10)
        table.remove(ft, 10)
    end

    print_menu(mt, ft)

end
--------------------------------------------------------------------------------
function exit_now()
    _lcd:update()
    _lcd:splashf(rb.HZ * 5, "ran for %d seconds", _timer.stop("main") / rb.HZ)
    os.exit()
end -- exit_now
--------------------------------------------------------------------------------

--MAIN PROGRAM------------------------------------------------------------------
_timer("main") -- keep track of how long the program ran

-- Clear the screen
_lcd:clear(BLACK)

if _lcd.DEPTH > 1 then
--draw a gradient using available colors
if IS_COLOR_TARGET == true then
    clrs = {
            {r = 255, g = 000, b = 000},  -- red
            {r = 000, g = 255, b = 000},  -- green
            {r = 000, g = 000, b = 255},  -- blue
           }
else
end
    local w = bit.rshift(_lcd.W, 2)
    local h = bit.rshift(_lcd.H, 2)
    draw_gradient(_lcd(), (_lcd.W - w)/2 - 1, (_lcd.H - h)/3 - 1, w, h, "H", clrs)
    _lcd:update()
    rb.sleep(rb.HZ)
end

do
local img = _img.load("/rocklua.bmp")
    if not img then
        rock_lua()
    else
        _lcd:image(img)
    end
end
_lcd:update()
rb.sleep(rb.HZ / 2)

if rb.cpu_boost then rb.cpu_boost(true) end

main_menu()

if rb.cpu_boost then rb.cpu_boost(false) end

exit_now()

-- For a reference list of all functions available, see apps/plugins/lua/rocklib.c (and apps/plugin.h)
