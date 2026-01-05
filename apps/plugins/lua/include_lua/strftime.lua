--[[
             __________               __   ___.
   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
                     \/            \/     \/    \/            \/
 $Id$

 strfrtime
 Copyright (C) 2026 William Wilgus
 gmtime
 * Copyright (C) 2017 by Michael Sevakis
 * Copyright (C) 2012 by Bertrik Sikken
 * Based on code from: rtc_as3514.c
 * Copyright (C) 2007 by Barry Wardell

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 KIND, either express or implied.
]]--

-- Lua implementation of strftime
-- if not os.time() then rb.splash(rb.HZ, "No Support!") return nil end

-- Weekday and month names
local weekdays = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}
local months = {"January", "February", "March", "April", "May", "June",
                "July", "August", "September", "October", "November", "December"
}
--local ampm = {"am", "pm", "AM", "PM"}

local strformat = string.format

-- Main strftime function
local function strftime(tmfmt, tm)
    local result = {}
    local i = 1

    while i <= #tmfmt do
        local out = ""
        local c = tmfmt:sub(i,i)
        if c == "%" then
            i = i + 1
            c = tmfmt:sub(i,i)
            if c ~= "%" then
                local handled
                repeat
                    handled = true
                    if c == "n" then
                        out = "\n"
                    elseif c == "N" then --custom for rocklua
                        out = strftime("%a %d %T %b %Y", tm)
                    elseif c == "t" then
                        out = "\t"
                    elseif c == "c" then
                        out = strftime("%x %T %Z %Y", tm)
                    elseif c == "r" then
                        out = strftime("%I:%M:%S %p", tm)
                    elseif c == "R" then
                        out = strftime("%H:%M", tm)
                    elseif c == "x" then
                        out = strftime("%b %a %d", tm)
                    elseif c == "T" or c == "X" then
                        out = strformat("%02d:%02d:%02d",
                                        tm.hour, tm.min, tm.sec)
                    elseif c == "D" then
                        out = strformat("%02d/%02d/%02d",
                                        tm.mon, tm.day, tm.year % 100)
                    elseif c == "F" then
                        out = strftime("%Y-%m-%d", tm)
                    elseif c == "a" then
                        out = weekdays[tm.wday + 1]:sub(1, 3)
                    elseif c == "A" then
                        out = weekdays[tm.wday + 1]
                    elseif c == "h" or c == "b" then
                        out = months[tm.month + 1]:sub(1, 3)
                    elseif c == "B" then
                        out = months[tm.month + 1]
                    elseif c == "p" then
                        --local idx = tm.hour > 12 and 4 or 5
                        out = tm.hour > 12 and "PM" or "AM"
                    elseif c == "P" then
                        --local idx = tm.hour > 12 and 2 or 1
                        out = tm.hour > 12 and "pm" or "am"
                    elseif c == "C" then
                        local no = (tm.year / 100) + 19
                        out = strformat("%02d", no)
                    elseif c == "d" then
                        out = strformat("%02d", tm.day)
                    elseif c == "e" then
                        out = strformat("%2.0d", tm.day)
                    elseif c == "H" then
                        out = strformat("%02d", tm.hour)
                    elseif c == "I" then
                        local no = tm.hour % 12
                        if no == 0 then no = 12 end
                        out = strformat("%02d", no)
                    elseif c == "j" then
                        out = strformat("%03d", tm.yday)
                    elseif c == "k" then
                        out = strformat("%2.0d", tm.hour)
                    elseif c == "l" then
                        local no = tm.hour % 12
                        if no == 0 then no = 12 end
                        out = strformat("%2.0d", no)
                    elseif c == "m" then
                        out = strformat("%02d", tm.mon)
                    elseif c == "M" then
                        out = strformat("%02d", tm.min)
                    elseif c == "S" then
                        out = strformat("%02d", tm.sec)
                    elseif c == "u" then
                        local no = tm.wday == 0 and 7 or tm.wday
                        out = strformat("%d", no)
                    elseif c == "w" then
                        out = strformat("%d", tm.wday)
                    elseif c == "U" then
                        local no = ((tm.yday - tm.wday + 7) / 7)
                        out = strformat("%02d", no)
                    elseif c == "W" then
                        local no = ((tm.yday - (tm.wday + 7) % 7 + 7) / 7)
                        out = strformat("%02d", no)
                    elseif c == "s" then
                        out = os.time()
                    elseif c == "Z" then
                        out = "[ETC?]"
                    elseif c == "Y" then
                        local y1 = (tm.year / 100) + 19
                        local y2 = tm.year % 100
                        out = strformat("%02d%02d", y1, y2)
                    elseif c == "y" then
                        out = strformat("%02d", tm.year % 100)
                    elseif c == "O" or c == "E" then
                        i = i + 1
                        c = tmfmt:sub(i,i)
                        handled = false
                    else
                        out = "%" .. c
                    end
                until (handled == true)
            else -- c == "%"
                out = "%"
            end
        else
            out = c
        end
        table.insert(result, out)
        i = i + 1
    end

    return table.concat(result)
end

--[[
Note: This implementation assumes the following structure for the `tm` 
table (similar to C's `struct tm`):

local tm = {
    year = 126,    -- Years since 1900
    mon = 0,       -- Month (0-11)
    day = 5,       -- Day of month (1-31)
    hour = 14,     -- Hours (0-23)
    min = 30,      -- Minutes (0-59)
    sec = 45,      -- Seconds (0-59)
    wday = 2,      -- Day of week (0-6, Sunday=0)
    yday = 4,      -- Day of year (0-365)
    isdst = false  -- Daylight saving time flag
}

For a complete implementation, you would need to add proper timezone handling
]]

local UNIX_EPOCH_DAY_NUM = 134774
local UNIX_EPOCH_YEAR = (1601 - 1900)
-- Last day number it can do
local MAX_DAY_NUM = 551879

--[[ d is days since epoch start, Monday, 1 January 1601 (d = 0) + UNIX_EPOCH_DAY_NUM
 *
 * That date is the beginning of a full 400 year cycle and so simplifies the
 * calculations a bit, not requiring offsets before divisions to shift the
 * leap year cycle.
 *
 * It can handle dates up through Sunday, 31 December 3111 (d = 551879).
 *
 * time_t can't get near the limits anyway for now but algorithm can be
 * altered slightly to increase range if even needed.
--Different than the rockbox version starting from midnight UTC on January 1, 1970
 ]]
local mon_yday = {
    -- year day of 1st of month (non-leap)
--  +31  +28  +31  +30  +31  +30  +31  +31  +30  +31  +30  +31  +31
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
}

local function get_tmdate(d, tm)
    d = d + UNIX_EPOCH_DAY_NUM  -- different than the rb version
    local x0 = (d / 1460)
    local x1 = (x0 / 25)
    local x2 = (x1 / 4)

    local y = ((d - x0 + x1 - x2) / 365)

    x0 = (y / 4)
    x1 = (x0 / 25)
    x2 = (x1 / 4)

    local yday = d - x0 + x1 - x2 - y * 365

    local x3 = yday
    local tm_yday = yday

    -- check if leap year; adjust February->March transition if so rather
    --   than keeping a leap year version of mon_yday[]

    if y - x0 * 4 == 3 and (x0 - x1 * 25 ~= 24 or x1 - x2 * 4 == 3) then
        if x3 >= mon_yday[3] then
            x3 = x3 - 1
            if x3 >= mon_yday[3] then
                yday = yday - 1
            end
        end
    end

    -- stab approximately at current month based on year day; advance if
    --   it fell short (never initially more than 1 short).

    x0 = (x3 / 32)
    if mon_yday[x0 + 2] <= x3 then
        x0 = x0 + 1
    end

    tm = tm or {}
    tm.year = y + UNIX_EPOCH_YEAR -- year - 1900
    tm.month = x0 -- 0..11
    tm.yday = tm_yday
    tm.day = yday - mon_yday[x0 + 1] + 1 -- 1..31
    tm.wday = (d + 1) % 7 -- 0..6
    return tm--return tm
end

local function gmtime(t, tm)
    tm = tm or {}

    local d = (t / 86400)
    local s = t - d * 86400

    if s < 0 then
        d = d - 1
        s = s + 86400
    end

    local x = (s / 3600)
    tm.hour = x -- 0..23

    s = s - x * 3600
    x = (s / 60)
    tm.min = x -- 0..59

    s = s - x * 60
    tm.sec = s -- 0..59

    tm.isdst = false -- not implemented right now

    return get_tmdate(d, tm)
end

local function os_date(format_string, timestamp)
    format_string = format_string or "%N"
    timestamp = timestamp or os.time()
    if format_string:sub(1, 1) == "!" then -- UTC
        format_string = format_string:sub(2)
    --else -- Localtime -- Rockbox doesn't support timezones
        --timestamp = get_tz(timestamp)
    end
    local tt = gmtime(timestamp);

    if format_string == "*t" then
        return tt
    else
        return strftime(format_string, tt)
    end
end

--fill os.date
if not os.date() then
    os.date = os_date;
end

return {
    strftime   = strftime;
    os_date    = os_date;
    get_tmdate = get_tmdate;
    gmtime     = gmtime;
}
