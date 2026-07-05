--[[
             __________               __   ___.
   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
                     \/            \/     \/    \/            \/
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
local b = {}
loops = 1 --global
local last_btn = 1

local ret = ""
local status = true
local depth = rb.LCD_DEPTH
if depth < 8 then depth = 8 end
local used, allocd, free = rb.mem_stats()
local alloc_sz = (free / (depth / 8)) / 1024


local function alloc_tables(loops)
    for i=1,loops do a[i] = {{tostring(loops * i) .. " mem test"}}; b[i] = {} end
    b[1] = rb.new_image(loops, alloc_sz);
end


local function alloc_tables_sm(loops)
    for i=1,loops do b[i] = {{}}; end
    local btn = rb.button_get(false)
    if (btn ~= 0) then
        if last_btn ~= btn then
            last_btn = btn
            return false;
        end
        while rb.button_get(false) ~= 0
        do

        end
        ret = "User Abort"
        status = false
        return true
    end
    return false
end

rb.lcd_putsxy(0, 0, "memchk loops : ")
while (status and loops < 1000)
do
    rb.lcd_putsxy(0, 20, loops)
    rb.lcd_update()

    if alloc_tables_sm(loops) then break end
    -- do call protected to catch OOM condition
    status, ret = pcall(alloc_tables, loops * 1000)
    loops = loops + 1
    _G.loops = loops
    rb.yield()
end

used, allocd, free = rb.mem_stats()
local lu = collectgarbage("count")
local fmt = function(t, v) return string.format("%s: %d Kb\n", t, v /1024) end

-- this is how lua recommends to concat strings rather than ..
local s_t = {}
s_t[1] = "rockbox:\n"
s_t[2] = "Loops : "
s_t[3] = loops - 1
s_t[4] = "\n"
if not status == true then
    s_t[5] = ret

else
    s_t[4] = ""
    s_t[5] = ""
end
s_t[6] = "\n"
s_t[7] = fmt("Used  ", used)
s_t[8] = fmt("Allocd ", allocd)
s_t[9] = fmt("Free  ", free)
s_t[10] = "\nlua:\n"
s_t[11] = fmt("Used", lu * 1024)
s_t[12] = "\n\nNote that the rockbox used count is a high watermark\n"
rb.splash_scroller(10 * rb.HZ, table.concat(s_t))
