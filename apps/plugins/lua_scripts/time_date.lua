--[[
             __________               __   ___.
   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
                     \/            \/     \/    \/            \/
 $Id$

 Copyright (C) 2026 William Wilgus
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 KIND, either express or implied.
]]--

if not os.time() then rb.splash(rb.HZ, "No Support!") return nil end

if not os.date() then
    require("strftime") -- needed for os.time..
end

local tmo_noblock = -1
local ctx = 0 --CONTEXT_STD

while rb.get_action(ctx, 50) == 0 do
    rb.splash(0, os.date())
end

collectgarbage("collect")

local used, allocd, free = rb.mem_stats()
local lu = collectgarbage("count")
local fmt = function(t, v) return string.format("%s: %d b\n", t, v) end

-- this is how lua recommends to concat strings rather than ..
local s_t = {}
s_t[1] = os.date()
s_t[2] = "\n"
s_t[3] = fmt("Used  ", used)
s_t[4] = fmt("Allocd ", allocd)
s_t[5] = fmt("Free  ", free)
s_t[6] = "\nlua:\n"
s_t[7] = fmt("Used", lu * 1024)
s_t[8] = "\n\nNote that the rockbox used count is a high watermark\n"
rb.splash_scroller(10 * rb.HZ, table.concat(s_t))
