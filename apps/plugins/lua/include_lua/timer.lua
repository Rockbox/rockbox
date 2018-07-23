--[[ Lua Timer functions
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

    _timer.active
    _timer.check
    _timer.split
    _timer.start
    _timer.stop

]]

local _timer = {} do

    --internal constants
    local _NIL = nil -- _NIL placeholder

    -- newer versions of lua use table.unpack
    local unpack = unpack or table.unpack

    --stores time elapsed at call to split; only vaid for unique timers
    local function split(index)
        if type(index) ~= "table" then return end
        index[#index + 1] = rb.current_tick() - _timer[index]
    end

    -- starts a new timer, if index is not specified a unique index is returned
    -- numeric or string indices are valid to use directly for permanent timers
    -- in this case its up to you to make sure you keep the index unique
    local function start(index)
        if index == _NIL then
            ---if you have _timer.start create timer it returns a unique Id which
            -- then has the same methods of _timer :start :stop :check :split
            index = setmetatable({}, {__index = _timer})
        end
        if _timer[index] == _NIL then
            _timer[index] = rb.current_tick()
        end
        return index
    end

    -- returns time elapsed in centiseconds, assigning bCheckonly keeps timer active
    local function stop(index, bCheckonly)

        local time_end = rb.current_tick()
        index = index or 0
        if not _timer[index] then
            return 0
        else
            local time_start = _timer[index]
            if not bCheckonly then _timer[index] = _NIL end -- destroy timer
            if type(index) ~= "table" then
                return time_end - time_start
            else
                return time_end - time_start, unpack(index)
            end
        end
    end

    -- returns time elapsed in centiseconds, assigning to bUpdate.. updates timer
    local function check(index, bUpdate)
        local elapsed = stop(index, true)
        if bUpdate ~= _NIL and index then
            _timer[index] = rb.current_tick()
        end
        return elapsed
    end

    -- returns table of active timers
    local function active()
        local t_active = {}
        local n = 0
        for k,v in pairs(_timer) do
            if type(_timer[k]) ~= "function" then
                n = n + 1
                t_active[n]=(k)
            end
        end
        return n, t_active
    end

    -- expose functions to the outside through _timer table
    _timer.active = active
    _timer.check  = check
    _timer.split  = split
    _timer.start  = start
    _timer.stop   = stop

    -- allows a call to _timer.start() by just calling _timer()
    _timer = setmetatable(_timer,{__call = function(t, i) return start(i) end})
end -- timer functions

return _timer

