--[[
             __________               __   ___.
   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
                     \/            \/     \/    \/            \/
 $Id$

 Copyright (C) 2024 William Wilgus
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 KIND, either express or implied.
]]--
--https://nullprogram.com/blog/2011/06/13/ [Infinite Parallax Starfield]

-- Imports
local _clr   = require("color") -- clrset, clrinc provides device independent colors
local _lcd   = require("lcd")   -- lcd helper functions
local _draw  = require("draw")  -- draw all the things (primitives)
local _poly  = require("draw_poly") -- vector drawing with tables of coords
require("actions")
--'CONSTANTS' (in lua there really is no such thing as all vars are mutable)
--------------------------------------------------------
--colors for fg/bg ------------------------
--first number of each quad is fallback for monochrome devices, excluded columns default to 0
local WHITE = _clr.set(-1, 255, 255, 255)
local BLACK = _clr.set(0, 0, 0, 0)
local RED   = _clr.set(WHITE, 100)
local GREEN = _clr.set(WHITE, 0, 100)
local BGREEN = _clr.set(WHITE, 0, 255)
local BLUE  = _clr.set(WHITE, 0, 0, 255)

local STAR_SEED = 0x811C9DC5;
local STAR_TILE_SIZE = math.max(rb.LCD_WIDTH, rb.LCD_HEIGHT) * 4;
local bxor, band, rshift, lshift, arshift = bit.bxor, bit.band, bit.rshift, bit.lshift, bit.arshift
local random, randomseed = math.random, math.randomseed
local start_x, start_y, start_z, scale_x, scale_y

-- load users coords from file if it exists
local fname = rb.PLUGIN_DATA_DIR .. "/stars.pos"
file = io.open(fname, "r")
if file then
   local v = 0
   for line in file:lines() do
       v = v + 1
       if v == 1 then
           start_x = tonumber(line) or 0
       elseif v == 2 then
           start_y = tonumber(line) or 0
       elseif v == 3 then
           start_z = tonumber(line) or 0
       elseif v == 4 then
           scale_x = tonumber(line) or 1
       elseif v == 5 then
           scale_y = tonumber(line) or 1
       else
           break;
       end
   end
   io.close( file )
end

-- Robert Jenkins' 96 bit Mix Function.
local function mix (a, b, c)
    a=a-b;  a=a-c;  a=bxor(a, (rshift(c, 13)))
    b=b-c;  b=b-a;  b=bxor(b, (lshift(a, 8)))
    c=c-a;  c=c-b;  c=bxor(c, (rshift(b, 13)))
    a=a-b;  a=a-c;  a=bxor(a, (rshift(c, 12)))
    b=b-c;  b=b-a;  b=bxor(b, (lshift(a, 16)))
    c=c-a;  c=c-b;  c=bxor(c, (rshift(b, 5)))
    a=a-b;  a=a-c;  a=bxor(a, (rshift(c, 3)))
    b=b-c;  b=b-a;  b=bxor(b, (lshift(a, 10)))
    c=c-a;  c=c-b;  c=bxor(c, (rshift(b, 15)))

    return c
end

-- given 32 bit number returns a table of 8 nibbles (4 bits)
local function s_bytes_nib(bits, value)
    -- bits must be multiples of 8 (sizeof byte)
    local bbuffer = {}
    local byte
    local nbytes = bit.rshift(bits, 3)
    for b = 1, nbytes do
        if value > 0 then
            byte  = value % 256
            value = (value - byte) / 256
        else
            byte = 0
        end
        bbuffer[#bbuffer + 1] = band(byte,0xF)
        bbuffer[#bbuffer + 1] = band(rshift(byte, 2), 0xF)
    end
    return bbuffer
end

--[[ given table t and total elems desired uses random elems of t
   and random numbers between 1 and max_v if #t < total_elems]]
function randomize_table(t, total_elems, max_v)
    local rand_t = {}
    local i = 1
    repeat
        local v = t[random(i, total_elems)]
        if v then
            rand_t[i] = v
        else
            rand_t[i] = random(1, max_v)
        end
        i = i + 1
    until i > total_elems
    return rand_t
end

local function drawship(img, ship_t)
    --_poly.polyline(img, x, y, ship_t, color, true, true)
   _poly.polygon(img, ship_t.x, ship_t.y, ship_t.disp_t, ship_t.color, ship_t.fillcolor, true)
end

local function draw_astroid(img, x, y, shape, color, fillcolor, scale_x, scale_y)
    --the random number generator gets seeded with the hash so we get the same figure each time
    randomseed(shape)
    local move_x, move_y
    -- we also use the 4 bytes of the hash as 4 coord pairs and randomly generate 8 more (16) half the size (8)
    local uniq_t = randomize_table(s_bytes_nib(32, shape), 16, 8)
    move_x, move_y = _poly.polyline(img, 0, 0, uniq_t, color, true, true, scale_x or 1, scale_y or 1, true)
    x = x - move_x / 2
    y = y - move_y / 2
    if fillcolor then
        _poly.polygon(img, x, y, uniq_t, color, fillcolor, true, scale_x or 1, scale_y or 1) --filled figures
    else
        _poly.polyline(img, x, y, uniq_t, color, true, true, scale_x or 1, scale_y or 1)
    end

    return x, y, move_x, move_y
end

local function drawStars(img, drawFn, xoff, yoff, starscale, color, scale_x, scale_y)
    local size = STAR_TILE_SIZE / starscale
    local s_x, s_y = scale_x, scale_y
    local w, h = rb.LCD_WIDTH, rb.LCD_HEIGHT

    -- Top-left tile's top-left position
    local sx = ((xoff - w/2) / size) * size - size;
    local sy = ((yoff - h/2) / size) * size - size;

    --Draw each tile currently in view.
    for i = sx, w + sx + size*3, size do
        for j = sy, h + sy + size*3, size do
            local hash = mix(STAR_SEED, (i / size), (j / size))
            for n = 0, 2 do
                local px = (hash % size) + (i - xoff)
                hash = arshift(hash, 3)

                local py = (hash % size) + (j - yoff)
                hash = arshift(hash, 3)
                if px > 0 and px < w and py > 0 and py < h then
                    drawFn(img, px, py, color, n, hash)
                end
            end
        end
    end
end

local function update_lcd()
    rb.lcd_puts(0,0, "[Infinite Starfield]")
    _lcd:update()
    rb.sleep(100)
    update_lcd = _lcd.update
end

local backlight_on
local function turn_on_backlight()
    rb.backlight_force_on();
    backlight_on = function() end
end
backlight_on = turn_on_backlight

do
    local act = rb.actions
    local quit = false
    --local last_action = 0
    local x,y,z = start_x or 0, start_y or 0, start_z or 8
    local s_x, s_y = scale_x or 1, scale_y or 1
    local ship_t = {x = (rb.LCD_WIDTH - 0xF) / 2,
                    y = rb.LCD_HEIGHT - (rb.LCD_HEIGHT / 3),
                    color = BGREEN,
                    fillcolor = BLACK,
                    -- ship vector coords x,y, x,y,...
                    lt_t = {0,7, 15,0, 9,7, 15,15, 0,7},
                    rt_t = {0,0, 5,7, 0,15, 15,7, 0,0},
                    up_t = {0,15, 7,0, 15,15, 7,9, 0,15},
                    dn_t = {0,0, 7,15, 15,0, 7,5, 0,0}
                    }
    ship_t.disp_t = ship_t.up_t

    local fast = {x = 1, y = 1, count = 0, inc_x = rb.LCD_WIDTH / 16, inc_y = rb.LCD_HEIGHT / 16}

    local last = {sx = s_x, sy = s_y, dx = 0, dy = 0, inc_x = 0, inc_y = 0}

    local function draw_points(img, x, y, color, n, hash)
        if s_x > s_y then
            img:line(x, y, x + s_x, y, color, true)
        elseif s_y > s_x then
            img:line(x, y, x, y + s_y, color, true)
        else
            img:set(x, y, color, true)
        end
    end

    function action_drift()
        if last.dx > 0 then
            last.dx = last.dx - 1
            x = x + last.dx
        elseif last.dx < 0 then
            last.dx = last.dx + 1
            x = x + last.dx
        end
        if last.dy > 0 then
            last.dy = last.dy - 1
            y = y + last.dy
        elseif last.dy < 0 then
            last.dy = last.dy + 1
            y = y + last.dy
        end
        if last.dx == 0 and last.dy == 0 then
            rockev.suspend("timer")
        end
        rockev.trigger("action", true, act.ACTION_REDRAW)
    end

    function action_event(action)
        backlight_on()
        if action == act.PLA_EXIT or action == act.PLA_CANCEL then
            quit = true
            start_x, start_y, start_z = x, y, z
            scale_x, scale_y = last.sx, last.sy
        elseif action == act.PLA_RIGHT_REPEAT then
            fast.count = fast.count + 1
            if fast.count % 10 == 0 then
                fast.x = fast.x + fast.inc_x
                if fast.count > 100 then s_x = s_x + 1 end
            end
            x = x + fast.x + last.inc_x
            s_y = last.sy
            last.dx = fast.x
            ship_t.disp_t = ship_t.rt_t
        elseif action == act.PLA_LEFT_REPEAT then
            fast.count = fast.count + 1
            if fast.count % 10 == 0 then
                fast.x = fast.x + fast.inc_x
                if fast.count > 100 then s_x = s_x + 1 end
            end
            x = x - fast.x + last.inc_x
            s_y = last.sy
            last.dx = -fast.x
            ship_t.disp_t = ship_t.lt_t
        elseif action == act.PLA_UP_REPEAT then
            fast.count = fast.count + 1
            if fast.count % 10 == 0 then
                fast.y = fast.y + fast.inc_y
                if fast.count > 100 then s_y = s_y + 1 end
            end
            y = y - fast.y + last.inc_y
            s_x = last.sx
            last.dy = -fast.y
            ship_t.disp_t = ship_t.up_t
        elseif action == act.PLA_DOWN_REPEAT then
            fast.count = fast.count + 1
            if fast.count % 10 == 0 then
                fast.y = fast.y + fast.inc_y
                if fast.count > 100 then s_y = s_y + 1 end
            end
            y = y + fast.y + last.inc_y
            s_x = last.sx
            last.dy = fast.y
            ship_t.disp_t = ship_t.dn_t
        elseif action == act.PLA_RIGHT then
            last.inc_x = last.inc_x + 1
            x = x + last.dx + 1
            if last.inc_x < 0 then
                last.inc_x = 0
            end
            last.dx = last.inc_x
            ship_t.disp_t = ship_t.rt_t
        elseif action == act.PLA_LEFT then
            last.inc_x = last.inc_x - 1
            x = x + last.dx - 1
            if last.inc_x > 0 then
                last.inc_x = 0
            end
            last.dx = last.inc_x
            ship_t.disp_t = ship_t.lt_t
        elseif action == act.PLA_UP then
            last.inc_y = last.inc_y - 1
            y = y + last.dy - 1
            if last.inc_y > 0 then
                last.inc_y = 0
            end
            last.dy = last.inc_y
            ship_t.disp_t = ship_t.up_t
        elseif action == act.PLA_DOWN then
            last.inc_y = last.inc_y + 1
            y = y + last.dy + 1
            if last.inc_y < 0 then
                last.inc_y = 0
            end
            last.dy = last.inc_y
            ship_t.disp_t = ship_t.dn_t
        elseif action == act.PLA_SELECT_REPEAT then
            rockev.suspend("timer", true)
            if s_x < 10 and s_y < 10 then
                s_x = last.sx + 1
                s_y = last.sy + 1
                last.sx = s_x
                last.sy = s_y
            end
        elseif action == act.PLA_SELECT then
            s_x = last.sx + 1
            s_y = last.sy + 1
            if s_x > 10 or s_y > 10 then
                s_x = 1
                s_y = 1
            end
            last.sx = s_x
            last.sy = s_y
        elseif action == act.ACTION_NONE then
            if fast.count > 100 then
                z = (random(0, 400) / 100) * 4
            end
            fast.count = 0
            fast.x = fast.inc_x
            fast.y = fast.inc_y
            s_x = last.sx
            s_y = last.sy
            backlight_on = turn_on_backlight
            rb.backlight_use_settings()
            if last.dx ~= 0 or last.dy ~= 0 then
                rockev.suspend("timer", false)
            else
                last.inc_x = 0
                last.inc_y = 0
            end
        end

        _lcd:clear(BLACK)
       for i = 0, z, 4 do
            drawStars(_LCD, draw_points, x, y, i+1, RED, s_x, s_y)
            drawStars(_LCD, draw_points, x, y, i+2, GREEN, s_x, s_y)
            drawStars(_LCD, draw_points, x, y, i+3, BLUE, s_x, s_y)
            drawStars(_LCD, draw_points, x, y, i+4, WHITE, s_x, s_y)
        end

        local hit_t = {}
        local SHIP_X, SHIP_Y = ship_t.x + 8, ship_t.y + 8 --center the ship coords
        local function draw_asteroids(img, x, y, color, n, hash)
            if n > 0 then
                local x0, y0, w0, h0
                x0,y0,w0,h0 = draw_astroid(img, x, y, hash, color, false, s_x, s_y)
                --check bounds
                if s_x == s_y and x0 <= SHIP_X and x0+w0 >= SHIP_X and y0+h0 >= SHIP_Y and y0 <= SHIP_Y then
                    local r_t = {x = x0, y = y0, w = w0, h= h0, hash = hash, color = color}
                    hit_t[#hit_t + 1] = r_t
                end
            end
        end

        drawStars(_LCD, draw_asteroids, x, y, 1, RED, s_x, s_y)
        drawStars(_LCD, draw_asteroids, x, y, 2, GREEN, s_x, s_y)
        drawStars(_LCD, draw_asteroids, x, y, 3, BLUE, s_x, s_y)
        drawStars(_LCD, draw_asteroids, x, y, 4, WHITE, s_x, s_y)
        if fast.count < 10 and last.dx == last.dy then
            local seen = {} -- might have multiple hits but only show unique hashes
            for i, v in ipairs(hit_t) do
                    if i < 4 then
                        draw_astroid(_LCD, v.x + v.w / 2, v.y + v.h / 2, v.hash, WHITE, v.color, s_x, s_y)
                    end
            end
            for i, v in ipairs(hit_t) do
                if not seen[v.hash] then
                    rb.lcd_puts(0, (i - 1), string.format("[%x]", v.hash))
                end
                seen[v.hash] = i
            end
        end

        drawship(_LCD, ship_t)
        update_lcd()

        --last_action = action
    end

    function action_set_quit(bQuit)
        quit = bQuit
    end

    function action_quit()
        return quit
    end
end

if not rb.backlight_force_on then
    rb.backlight_force_on = function() end
end

if not rb.backlight_use_settings then
    rb.backlight_use_settings = function() end
end

action_event(rb.actions.ACTION_NONE) -- we can call this now but not after registering..
local eva = rockev.register("action", action_event)
local evc = rockev.register("timer", action_drift, rb.HZ/7)

while not action_quit() do rb.sleep(rb.HZ) end

if start_x and start_y then
    file = io.open(fname, "w")
    file:write(start_x, "\n", start_y, "\n", start_z or 0, "\n", scale_x or 1, "\n", scale_y or 1, "\n")
    io.close( file )
end
