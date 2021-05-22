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
if not rb.lcd_framebuffer then rb.splash(rb.HZ, "No Support!") return nil end

require("printtable")
local _print = require("print")
local _clr   = require("color")
local _lcd = require("lcd")
local _timer = require("timer")
local _draw = require("draw")
local BUTTON = require("menubuttons")
local sb_width = 5

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

-- Gets size of text
local function text_extent(msg, font)
    -- res, w, h
    return rb.font_getstringsize(msg, font or rb.FONT_UI)
end
--------------------------------------------------------------------------------
--[[ cursor style button routine
-- left / right are x, xi is increment xir is increment when repeat
-- up / down are y, yi is increment yir is increment when repeat
-- cancel is returned as 0,1
-- select as 0, 1, 2, 3 (none, pressed, repeat, relesed)
-- x_chg and y_chg are the amount x or y changed
-- timeout == nil or -1 loop waits indefinitely till button is pressed
-- time since last button press is returned in ticks..
-- make xi, xir, yi, yir negative to flip direction...
]]
local function dpad(x, xi, xir, y, yi, yir, timeout, overflow, selected)
    local scroll_is_fixed = overflow ~= "manual"
    _timer("dpad") -- start a persistant timer; keeps time between button events
    if timeout == nil then timeout = -1 end
    local cancel, select = 0, 0
    local x_chg, y_chg = 0, 0
    local button
    while true do
        button = rb.get_plugin_action(timeout)

        if button == BUTTON.CANCEL then
            cancel = 1
            break;
        elseif button == BUTTON.EXIT then
            cancel = 1
            break;
        elseif button == BUTTON.SEL then
            select = 1
            timeout = timeout + 1
        elseif button == BUTTON.SELR then
            select = 2
            timeout = timeout + 1
        elseif button == BUTTON.SELREL then
            select = -1
            timeout = timeout + 1
        elseif button == BUTTON.LEFT then
            x_chg = x_chg - xi
            if scroll_is_fixed then
                cancel = 1
                break;
            end
        elseif button == BUTTON.LEFTR then
            x_chg = x_chg - xir
        elseif button == BUTTON.RIGHT then
            x_chg = x_chg + xi
            if scroll_is_fixed then
                select = 1
                timeout = timeout + 1
            end
        elseif button == BUTTON.RIGHTR then
            x_chg = x_chg + xir
        elseif button == BUTTON.UP then
            y_chg = y_chg + yi
        elseif button == BUTTON.UPR then
            y_chg = y_chg + yir
        elseif button == BUTTON.DOWN then
            y_chg = y_chg - yi
        elseif button == BUTTON.DOWNR then
            y_chg = y_chg - yir
        elseif timeout >= 0 then--and rb.button_queue_count() < 1 then
            break;
        end

        if x_chg ~= 0 or y_chg ~= 0 then
            timeout = timeout + 1
        end
    end

    x = x + x_chg
    y = y + y_chg

    return cancel, select, x_chg, x, y_chg, y, _timer.check("dpad", true)
end -- dpad



--------------------------------------------------------------------------------
--[[ prints a scrollable table to the screen;
-- requires a contiguous table with only strings;
-- 1st item in table is the title if hasheader == true
-- returns select item indice if NOT m_sel..
-- if m_sel == true a table of selected indices are returned ]]
--------------------------------------------------------------------------------
-- SECOND MODE OF OPERATION -- if co_routine is defined...
-- prints values returned from a resumable factory in a coroutine this allows
-- very large files etc to be displayed.. the downside is it takes time
-- to load data when scrolling also NO multiple selection is allowed
-- table is passed along with the final count nrows
--------------------------------------------------------------------------------
-- uses print_table to display a portion of a file
require("printtable")
function print_cells(settings, nrows, ncols, t_col1, ...)
    local o = _print.opt.get(true)
    --oldprintf = _print.f


    local header_sep = " | "
    local t_cells =  {t_col1, ...}
    local t_col_width = {}
    local hstr = {}
    local max_h = 0;
-- (table, nrows, {hasheader, wrap, m_sel, start, curpos, justify, co_routine})






    for i, v in ipairs(t_cells) do
        if type(v) ~= "table" then
            rb.splash(rb.HZ * 5, "table expected got ".. type(v))
            return
        end
        local w
        t_col_width[i] = 0
        for j = 1, nrows, 1 do
            _, w, _ = text_extent(v[j])
            if w > t_col_width[i] then t_col_width[i] = w end 
        end
        local val = v[1]
        repeat 
            _, w, _ = text_extent(val)
            if w < t_col_width[i] then
                val = val .. " "
            end
        until w >= t_col_width[i]
        hstr[#hstr+1] = val
    end
    hstr = table.concat(hstr, "\t")

    local o_putline = rb.lcd_put_line
    local function pl(x, y, msg, ld)
    local width = x + 1
        o_putline(x, y, msg, ld)
        if width > 0 and width <= _lcd.W then
            _draw.vline(_lcd(), width, y + 2 , max_h, _clr.set(1, 255,255,255))
        end
        for i, v in ipairs(t_cells) do
            width = width + t_col_width[i] + 8
            if width > 0 and width <= _lcd.W then
                _draw.vline(_lcd(), width, y + 2 , max_h, _clr.set(1, 255,255,255))
            end
        end
        x = x + 1
        y = y + 1
        --_draw.rect(_lcd(), x, y, _lcd.W - x, o.height / o.max_line , o.fg_pattern)
    end
    rb.lcd_put_line = pl
    local function pf(fmt, str, item)
        local line, max_line, w, h = oldprintf(fmt, str)
        _draw.rect(_lcd(), 1, h, _lcd.W - 1, h)
        _lcd.update()

    end

    local file_t = setmetatable({},{__mode = "kv"}) --weak keys and values
    -- this allows them to be garbage collected as space is needed
    -- rebuilds when needed
    local ovf = 0
    local lpos = 1
    local timer = _timer()

    function print_co()
        while true do
            collectgarbage("step")
            file_t[1] = hstr --position 1 is ALWAYS header/title


--[[
            for i = 1, nrows do
                if not file_t[i] then
                    for i, v in ipairs(t_cells) do
                        hstr[#hstr+1] = v[1] 
                    end
                    file_t[i] = table.concat(item)
                end
            end
]]
                local bpos = coroutine.yield()
                    local res, w, h
                    local item = {}
                    for i, v in ipairs(t_cells) do
                        
                        local val = v[bpos]
                        repeat 
                            res, w, h = text_extent(val)
                            if h > max_h then max_h = h end
                            if w < t_col_width[i] then
                                val = val .. "  "
                            end
                        until w >= t_col_width[i]

                        repeat 
                            res, w, h = text_extent(val)
                            if h > max_h then max_h = h end
                            if w > t_col_width[i] then
                                val = val:sub(1, -2);
                            end
                        until w <= t_col_width[i]
                        item[#item+1] = val
                    end
                    file_t[bpos] = table.concat(item, "\t") or "??"
                    if bpos == 2 then
                        --rb.splash(1000, bpos .. " " .. file_t[bpos])
                    end
--[[
                if bpos <= lpos then -- roll over or scroll up
                    bpos = (bpos - bufsz) + bpos % bufsz
                    timer:check(true)
                end

                lpos = bpos - bpos % bufsz

                if lpos < 1 then
                    lpos = 1
                elseif lpos > count - bufsz then -- partial fill
                    ovf = count - bufsz - lpos
                end
                --get position in file of the nearest indexed line
                file:seek("set", idx_t[bpos / bufsz + 1])

                -- on really large files if it has been more than 10 minutes
                -- since the user scrolled up the screen wipe out the prior
                -- items to free memory
                if lpos % 5000 == 0 and timer:check() > rb.HZ * 600 then
                    for i = 1, lpos - 100 do
                        file_t[i] = nil
                    end
                end
]]
        end
    end

    co = coroutine.create(print_co)
    _lcd:clear()
    _print.clear()

    if not settings then
        settings = {}
        settings.justify = "left"
        settings.wrap = true
    end

    settings.hasheader = false
    settings.co_routine = co
    settings.msel = false
    settings.ovfl = "manual"
    settings.drawsep = true
    --table.insert(file_t, 1, hstr) --position 1 is header/title
    local sel =
    print_table(file_t, nrows, settings)
    file_t = nil
    --_print.f = oldprintf
    rb.lcd_put_line = o_putline
    return sel
end --print_file_increment



local col1 = {"COL 1", "1:1", "Item 1:2", "Item 1:3", "Item 1:4"}
local col2 = {"COL 2", "Item 2:1", "Item 2:2", "Item 2:3", "Item 2:4"}
local col3 = {"COL 3", "Item 3:1", "Item 3:2", "Item 3:3", "A longer item than the rest"}
print_cells(nil, 5, 3, col1, col2, col3)
