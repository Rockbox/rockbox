--[[ Lua Missing Math functions
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

    _math.clamp
    _math.clamp_roll
    _math.d_sin
    _math.d_cos
    _math.d_tan
    _math.i_sqrt

]]

local _math = {} do

    -- internal constants
    local _NIL = nil -- _NIL placeholder

   -- clamps value to >= min and <= max
    local function clamp(iVal, iMin, iMax)
        if iMin > iMax then
            local swap = iMin
            iMin, iMax = iMax, swap
        end

        if iVal < iMin then
            return iMin
        elseif iVal < iMax then
            return iVal
        end

        return iMax
    end

   -- clamps value to >= min and <= max rolls over to opposite
    local function clamp_roll(iVal, iMin, iMax)
        if iMin > iMax then
            local swap = iMin
            iMin, iMax = iMax, swap
        end

        if iVal < iMin then
            iVal = iMax
        elseif iVal > iMax then
            iVal = iMin
        end

        return iVal
    end

    local function i_sqrt(n)
        -- Newtons square root approximation
        if n < 2 then return n end
        local g = n / 2
        local l = 1

        for c = 1, 25 do -- if l,g haven't converged after 25 iterations quit

            l = (n / g + g)/ 2
            g = (n / l + l)/ 2

            if g == l then return g end
        end

        -- check for period-two cycle between g and l
        if g - l == 1 then
            return l
        elseif l - g == 1 then
            return g
        end

        return _NIL
    end

    local function d_sin(iDeg, bExtraPrecision)
    --[[  values are returned multiplied by 10000
          II  |  I       180-90   | 90-0
         ---(--)---      -------(--)-------
         III |  IV       180-270 | 270-360

        sine is only positive in quadrants I , II => 0 - 180 degrees
        sine 180-360 degrees is a reflection of sine 0-180
        Bhaskara I's sine approximation formula isn't overly accurate
        but it is close enough for rough image work.
    ]]
        local sign, x
        -- no negative angles -10 degrees = 350 degrees
        if iDeg < 0 then
            x = 360 + (iDeg % 360)
        else --keep rotation in 0-360 range
            x = iDeg % 360
        end

        -- reflect II & I onto III & IV
        if x > 180 then
            sign = -1
            x = x % 180
        else
            sign = 1
        end

        local x1 = x * (180 - x)

        if bExtraPrecision then -- ~halves the largest errors
            if x <= 22 or x >= 158 then
                return sign * 39818 * x1 / (40497 - x1)
            elseif (x >= 40 and x <= 56) or (x > 124 and x < 140) then
                return sign * 40002 * x1 / (40450 - x1)
            elseif (x > 31 and x < 71) or (x > 109 and x < 150) then
                return sign * 40009 * x1 / (40470 - x1)
            end
        end

        --multiplied by 10000 so no decimal in results (RB LUA is integer only)
        return sign * 40000 * x1 / (40497 - x1)
    end

    local function d_cos(iDeg, bExtraPrecision)
        --cos is just sine shifed by 90 degrees CCW
        return d_sin(90 - iDeg, bExtraPrecision)
    end

    local function d_tan(iDeg, bExtraPrecision)
        --tan = sin0 / cos0
        return (d_sin(iDeg, bExtraPrecision) * 10000 / d_sin(90 - iDeg, bExtraPrecision))
    end

    --expose functions to the outside through _math table
    _math.clamp       = clamp
    _math.clamp_roll  = clamp_roll
    _math.i_sqrt      = i_sqrt
    _math.d_sin       = d_sin
    _math.d_cos       = d_cos
    _math.d_tan       = d_tan
end -- missing math functions

return _math

