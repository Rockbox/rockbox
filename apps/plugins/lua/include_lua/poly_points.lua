--PolyPoints.lua
--[[
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

--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- decodes ascii string back to t_pts
function points_from_ascii(encstr, scale)
    scale = scale or 1
    local t_pts = {}
    encstr = encstr or "00000008"
    local chroffset = tonumber(string.sub(encstr, 1, 3)) or 0
    local chrlen = tonumber(string.sub(encstr, 4, 8)) or 0

    if string.len(encstr) ~= chrlen then
        error("Invalid Points String" .. string.len(encstr), 2)
    end

    for i = 9, string.len(encstr) - 1, 2 do
        local x = string.byte(encstr, i, i) - chroffset
        local y = string.byte(encstr, i + 1, i + 1) - chroffset
        t_pts[#t_pts + 1] = x * scale
        t_pts[#t_pts + 1] = y * scale
    end
    return t_pts
end
--------------------------------------------------------------------------------
-- encodes t_pts as a ascii string non print chars are excluded so
-- size is limited to approx ~ 90x90
function points_to_ascii(t_pts)
    if not t_pts then return "" end
    local chroffset = 33
    local maxoffset = 126 - 33
    local t_enc = {[1] = string.format("%03d%05d", chroffset, #t_pts + 8)}
    local max_n, min_n = 0, 0
    for i = 1, #t_pts, 2 do
        if t_pts[i] > max_n then max_n = t_pts[i] end
        if t_pts[i] < min_n then min_n = t_pts[i] end
        if t_pts[i+1] > max_n then max_n = t_pts[i+1] end
        if t_pts[i+1] < min_n then min_n = t_pts[i+1] end
        if max_n > maxoffset or min_n < 0 then break; end
        t_enc[#t_enc + 1] = string.char(t_pts[i] + chroffset)
        t_enc[#t_enc + 1] = string.char(t_pts[i+1] + chroffset)
    end
	if min_n >= 0 and (max_n - min_n) <= maxoffset then
        return  table.concat(t_enc)
    else
        return "00000008"
    end
end
--------------------------------------------------------------------------------
-- scales t_pts by percentage (x/y)
function points_scale_pct(t_pts, x_pct, y_pct)
    for i = 1, #t_pts, 2 do
        local t_pt = {t_pts[i], t_pts[i + 1]}
        t_pt = t_pt or {0, 0}
        t_pts[i] = (t_pt[1] * x_pct) / 100
        t_pts[i+1] = (t_pt[2] * y_pct) / 100
    end
    return t_pts
end
--------------------------------------------------------------------------------
-- scales t_pts by (x/y)
function points_scale(t_pts, width, height)
    local max_x, max_y = 0, 0
    for i = 1, #t_pts, 2 do
        if t_pts[i] > max_x then max_x = t_pts[i] end
        if t_pts[i+1] > max_y then max_y = t_pts[i+1] end
    end

    local x_pct = (width * 100) / max_x
    local y_pct = (height * 100) / max_y

    return points_scale_pct(t_pts, x_pct, y_pct)
end
--------------------------------------------------------------------------------
--[[
    function scaleup(t_pts, scale_x, scale_y)
        local t_coord
        for key,value in pairs(t_pts) do
            t_coord = t_pts[key]
            t_coord[1] = t_coord[1] * scale_x
            t_coord[2] = t_coord[2] * scale_y
            t_pts[key] = t_coord
        end
    end

    function scaledn(t_pts, scale_x, scale_y)
        local t_coord
        for key,value in pairs(t_pts) do
            t_coord = t_pts[key]
            t_coord[1] = t_coord[1] / scale_x
            t_coord[2] = t_coord[2] / scale_y
            t_pts[key] = t_coord
        end
    end
]]
