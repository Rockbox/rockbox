--[[
             __________               __   ___.
   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
                     \/            \/     \/    \/            \/
 $Id$
 Example Lua Memory Use
 Copyright (C) 2020 William Wilgus
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 KIND, either express or implied.
]]--

local a = {}
loops = 1 --global

local function alloc_tables(loops)
    for i=1,loops do a[i] = {{}}; local b = {} end
    return true
end

local ret
local status = true
rb.lcd_putsxy(0, 0, "memchk loops : ")
while (status and loops < 1000)
do
    rb.lcd_putsxy(0, 20, loops)
    rb.lcd_update()
    alloc_tables(loops)
    -- do call protected to catch OOM condition
    status, ret = pcall(alloc_tables, loops * 1000)
    loops = loops + 1
    _G.loops = loops
end

local used, allocd, free = rb.mem_stats()
local lu = collectgarbage("count")
local fmt = function(t, v) return string.format("%s: %d Kb\n", t, v /1024) end

-- this is how lua recommends to concat strings rather than ..
local s_t = {}
s_t[1] = "rockbox:\n"
s_t[2] = "Loops : "
s_t[3] = loops - 1
s_t[4] = "\n"
s_t[5] = fmt("Used  ", used)
s_t[6] = fmt("Allocd ", allocd)
s_t[7] = fmt("Free  ", free)
s_t[8] = "\nlua:\n"
s_t[9] = fmt("Used", lu * 1024)
s_t[10] = "\n\nNote that the rockbox used count is a high watermark\n"
rb.splash_scroller(10 * rb.HZ, table.concat(s_t))
