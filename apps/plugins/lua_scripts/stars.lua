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
local BLUE  = _clr.set(WHITE, 0, 0, 100)

local STAR_SEED = 0x9d2c5680;
local STAR_TILE_SIZE = math.max(rb.LCD_WIDTH, rb.LCD_HEIGHT) * 4;
local bxor, band, rshift, lshift, arshift = bit.bxor, bit.band, bit.rshift, bit.lshift, bit.arshift
local random, randomseed = math.random, math.randomseed
local start_x, start_y, start_z

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
       else
           break;
       end
   end
   io.close( file )
end

-- Robert Jenkins' 96 bit Mix Function.
local function mix (a, b, c)
    a=a-b;  a=a-c;  a=bxor(a, (rshift(c, 13)));
    b=b-c;  b=b-a;  b=bxor(b, (lshift(a, 8)));
    c=c-a;  c=c-b;  c=bxor(c, (rshift(b, 13)));
    a=a-b;  a=a-c;  a=bxor(a, (rshift(c, 12)));
    b=b-c;  b=b-a;  b=bxor(b, (lshift(a, 16)));
    c=c-a;  c=c-b;  c=bxor(c, (rshift(b, 5)));
    a=a-b;  a=a-c;  a=bxor(a, (rshift(c, 3)));
    b=b-c;  b=b-a;  b=bxor(b, (lshift(a, 10)));
    c=c-a;  c=c-b;  c=bxor(c, (rshift(b, 15)));

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
        local v = t[random(1, total_elems)]
        if v then
            rand_t[i] = v
        else
            rand_t[i] = random(1, max_v)
        end
        i = i + 1
    until i > total_elems
    return rand_t
end

local function drawship(img, x, y, color, ship_t)
    --_poly.polyline(img, x, y, ship_t, color, true, true)
   _poly.polygon(img, x, y, ship_t, color, BLACK, true)
end

local function draw_astroid(img, x, y, size, shape, color, scale_x, scale_y)
    --the random number generator gets seeded with the hash so we get the same figure each time
    randomseed(shape)
    -- we also use the 4 bytes of the hash as 4 coord pairs and randomly generate 8 more (16) half the size (8)
    local uniq_t = randomize_table(s_bytes_nib(32, shape), 16, 8)
    _poly.polyline(img, x, y, uniq_t, color, true, true, scale_x or 1, scale_y or 1)
    --_poly.polygon(img, x, y, uniq_t, color, color, true, scale_x or 1, scale_y or 1) --filled figures
end

local function drawStars (img, xoff, yoff, starscale, color, scale_x, scale_y)
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
                local px = (hash % size) + (i - xoff);
                hash = arshift(hash, 3)

                local py = (hash % size) + (j - yoff);
                hash = arshift(hash, 3)

                if px > 0 and px < w and py > 0 and py < h then
                    if n > 0 and starscale < 5 then
                        draw_astroid(img, px, py, n, hash, color, s_x, s_y)
                    else
                        if scale_x > 1 then
                            img:set(px, py, color)
                            img:set(px + 1, py, color)
                        elseif scale_y > 1 then
                            img:set(px, py, color)
                            img:set(px, py + 1, color)
                        else
                            img:set(px, py, color)
                        end
                    end
                end
            end
        end
    end
end

do
    local act = rb.actions
    local quit = false
    local last_action = 0
    local x,y,z = start_x or 0, start_y or 0, start_z or 8
    local x_fast = rb.LCD_WIDTH / 4
    local y_fast = rb.LCD_HEIGHT / 4
    local ship_x = (rb.LCD_WIDTH - 0xF) / 2
    local ship_y = rb.LCD_HEIGHT - (rb.LCD_HEIGHT / 3)
    local scale_x, scale_y = 1, 1
    -- vector draw the ship points for each direction (<>^v)
    local ship_lt_t = {0,7, 15,0, 9,7, 15,15, 0,7}
    local ship_rt_t = {0,0, 5,7, 0,15, 15,7, 0,0}
    local ship_up_t = {0,15, 7,0, 15,15, 7,9, 0,15}
    local ship_dn_t = {0,0, 7,15, 15,0, 7,5, 0,0}
    local ship_t = ship_up_t

    function action_event(action)
        if action == act.PLA_EXIT or action == act.PLA_CANCEL then
            quit = true
            start_x, start_y, start_z = x, y, z
        elseif action == act.PLA_RIGHT_REPEAT then
            x = x + x_fast
            scale_x = scale_x + 1
            scale_y = 1
        elseif action == act.PLA_LEFT_REPEAT then
            x = x - x_fast
            scale_x = scale_x + 1
            scale_y = 1
        elseif action == act.PLA_UP_REPEAT then
            y = y - y_fast
            scale_y = scale_y + 1
            scale_x = 1
        elseif action == act.PLA_DOWN_REPEAT then
            y = y + y_fast
            scale_y = scale_y + 1
            scale_x = 1
        elseif action == act.PLA_RIGHT then
            x = x + 1
            ship_t = ship_rt_t
        elseif action == act.PLA_LEFT then
            x = x - 1
            ship_t = ship_lt_t
        elseif action == act.PLA_UP then
            y = y - 1
            ship_t = ship_up_t
        elseif action == act.PLA_DOWN then
            y = y + 1
            ship_t = ship_dn_t
        elseif action == act.PLA_SELECT then
            z = z + 4
            if z > 16 then z = 0 end
        elseif action == act.ACTION_NONE then
            scale_x = 1
            scale_y = 1
        end

        _lcd:clear(BLACK)
        for i = 0, z, 4 do
            drawStars(_LCD, x, y, i+1, RED, scale_x, scale_y)
            drawStars(_LCD, x, y, i+2, GREEN, scale_x, scale_y)
            drawStars(_LCD, x, y, i+3, BLUE, scale_x, scale_y)
            drawStars(_LCD, x, y, i+4, WHITE, scale_x, scale_y)
        end
        drawship(_LCD, ship_x, ship_y, BGREEN, ship_t)
        _lcd:update()

        last_action = action
    end

    function action_set_quit(bQuit)
        quit = bQuit
    end

    function action_quit()
        return quit
    end
end

action_event(rb.actions.ACTION_NONE) -- we can call this now but not after registering..
local eva = rockev.register("action", action_event)

while not action_quit() do rb.sleep(rb.HZ) end

if start_x and start_y then
    file = io.open(fname, "w")
    file:write(start_x, "\n", start_y, "\n", start_z, "\n")
    io.close( file )
end
