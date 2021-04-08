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

require("actions")   -- Contains rb.actions & rb.contexts

local _clr   = require("color")
local _print = require("print")
local _timer = require("timer")

-- Button definitions --
local EXIT_BUTTON = rb.PLA_EXIT
local CANCEL_BUTTON = rb.actions.PLA_CANCEL
local DOWN_BUTTON = rb.actions.PLA_DOWN
local DOWNR_BUTTON = rb.actions.PLA_DOWN_REPEAT
local EXIT_BUTTON = rb.actions.PLA_EXIT
local LEFT_BUTTON = rb.actions.PLA_LEFT
local LEFTR_BUTTON = rb.actions.PLA_LEFT_REPEAT
local RIGHT_BUTTON = rb.actions.PLA_RIGHT
local RIGHTR_BUTTON = rb.actions.PLA_RIGHT_REPEAT
local SEL_BUTTON = rb.actions.PLA_SELECT
local SELREL_BUTTON = rb.actions.PLA_SELECT_REL
local SELR_BUTTON = rb.actions.PLA_SELECT_REPEAT
local UP_BUTTON = rb.actions.PLA_UP
local UPR_BUTTON = rb.actions.PLA_UP_REPEAT

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

local function dpad(x, xi, xir, y, yi, yir, timeout)
    _timer("dpad") -- start a persistant timer; keeps time between button events
    if timeout == nil then timeout = -1 end
    local cancel, select = 0, 0
    local x_chg, y_chg = 0, 0
    local button
    while true do
        button = rb.get_plugin_action(timeout)

        if button == CANCEL_BUTTON then
            cancel = 1
            break;
        elseif button == EXIT_BUTTON then
            cancel = 1
            break;
        elseif button == SEL_BUTTON then
            select = 1
            timeout = timeout + 1
        elseif button == SELR_BUTTON then
            select = 2
            timeout = timeout + 1
        elseif button == SELREL_BUTTON then
            select = -1
            timeout = timeout + 1
        elseif button == LEFT_BUTTON then
            x_chg = x_chg - xi
        elseif button == LEFTR_BUTTON then
            x_chg = x_chg - xir
        elseif button == RIGHT_BUTTON then
            x_chg = x_chg + xi
        elseif button == RIGHTR_BUTTON then
            x_chg = x_chg + xir
        elseif button == UP_BUTTON then
            y_chg = y_chg + yi
        elseif button == UPR_BUTTON then
            y_chg = y_chg + yir
        elseif button == DOWN_BUTTON then
            y_chg = y_chg - yi
        elseif button == DOWNR_BUTTON then
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
-- table is passed along with the final count t_count
--------------------------------------------------------------------------------

function print_table(t, t_count, settings)
-- (table, t_count, {hasheader, wrap, m_sel, start, curpos, justify, co_routine})

    if type(t) ~= "table" then
        rb.splash(rb.HZ * 5, "table expected got ".. type(t))
        return
    end

    local wrap, justify, start, curpos, co_routine, hasheader, m_sel
    local header_fgc, header_bgc, item_fgc, item_bgc, item_selc, drawsep
    do
        local s = settings or _print.get_settings()
        wrap, justify = s.wrap, s.justify
        start, curpos = s.start, s.curpos
        co_routine    = s.co_routine
        hasheader     = s.hasheader
        drawsep       = s.drawsep
        m_sel         = false
        if co_routine == nil then
            --no multi select in incremental mode
            m_sel = s.msel
        end
        header_fgc = s.hfgc  or _clr.set( 0, 000, 000, 000)
        header_bgc = s.hbgc  or _clr.set(-1, 255, 255, 255)
        item_fgc   = s.ifgc  or _clr.set(-1, 000, 255, 060)
        item_bgc   = s.ibgc  or _clr.set( 0, 000, 000, 000)
        item_selc  = s.iselc or _clr.set( 1, 000, 200, 100)
    end

    local table_p, line, maxline

    local function set_vsb() end -- forward declaration; initialized below

    local function init_position(acc_ticks, acc_steps)
        if not acc_ticks then acc_ticks = 15 end-- accelerate scroll every this many ticks
        if not acc_steps then acc_steps = 5 end -- default steps for an accelerated scroll

        return {row = 1, row_scrl= acc_steps,
                col = 0, col_scrl = acc_steps,
                vcursor = 1, vcursor_min = 1,
                acc_ticks = acc_ticks, 
                acc_steps = acc_steps}
    end

    local function set_accel(time, scrl, t_p)
        if time < t_p.acc_ticks then -- accelerate scroll
            scrl = scrl + 1
        else
            scrl = t_p.acc_steps
        end
        return scrl
    end

    --adds or removes \0 from end of table entry to mark selected items
    local function select_item(item)
        if item < 1 then item = 1 end
        if not t[item] then return end
        if t[item]:sub(-1) == "\0" then
            t[item] = t[item]:sub(1, -2) -- de-select
        else
            t[item] = t[item] .. "\0" -- select
        end
    end

    -- displays header text at top
    local function disp_header(hstr)
        local header = header or hstr
        local opts = _print.opt.get()
        _print.opt.overflow("none") -- don't scroll header; colors change
        _print.opt.color(header_fgc, header_bgc)
        _print.opt.line(1)

        _print.f()
        local line = _print.f(header)

        _print.opt.set(opts)
        _print.opt.line(2)
        return 2
    end

    -- gets user input to select items, quit, scroll
    local function get_input(t_p)
        set_vsb(t_p.row + t_p.vcursor - 1)--t_p.row)
        rb.lcd_update()

        local quit, select, x_chg, xi, y_chg, yi, timeb =
                          dpad(t_p.col, -1, -t_p.col_scrl, t_p.row, -1, -t_p.row_scrl)

        t_p.vcursor = t_p.vcursor + y_chg

        if t_p.vcursor > maxline or t_p.vcursor < t_p.vcursor_min then
            t_p.row = yi
        end

        if wrap == true and (y_chg == 1 or y_chg == -1) then

            -- wraps list, stops at end if accelerated
            if t_p.row < t_p.vcursor_min - 1 then
                t_p.row  = t_count - maxline + 1
                t_p.vcursor = maxline
            elseif t_p.row + maxline - 1 > t_count then
                t_p.row, t_p.vcursor = t_p.vcursor_min - 1, t_p.vcursor_min - 1
            end
        end

        t_p.row  = clamp(t_p.row, 1, math.max(t_count - maxline + 1, 1))
        t_p.vcursor = clamp(t_p.vcursor, t_p.vcursor_min, maxline)

        if x_chg ~= 0 then

            if x_chg ~= 1 and x_chg ~= -1 then --stop at the center if accelerated
                if (t_p.col <= 0 and xi > 0) or (t_p.col >= 0 and xi < 0) then
                    xi = 0
                end
            end
            t_p.col = xi

            t_p.col_scrl = set_accel(timeb, t_p.col_scrl, t_p)

        elseif y_chg ~= 0 then
            --t_p.col = 0 -- reset column to the beginning
            _print.clear()
            _print.opt.sel_line(t_p.vcursor)

            t_p.row_scrl = set_accel(timeb, t_p.row_scrl, t_p)

        end

        if select > 0 and timeb > 15 then --select may be sent multiple times
            if m_sel == true then
                select_item(t_p.row + t_p.vcursor - 1)
            else
                return -1, 0, 0, (t_p.row + t_p.vcursor - 1)
            end
        end
        if quit > 0 then return -2, 0, 0, 0 end
        return t_p.row, x_chg, y_chg, 0
    end

    -- displays the actual table
    local function display_table(table_p, col_c, row_c, sel)
        local i = table_p.row
        while i >= 1 and i <= t_count do

            -- only print if beginning or user scrolled up/down
            if row_c ~= 0 then

                if t[i] == nil and co_routine then
                    --value has been garbage collected or not created yet
                    coroutine.resume(co_routine, i)
                end

                if t[i] == nil then
                    rb.splash(1, string.format("ERROR %d is nil", i))
                    t[i] = "???"
                    if rb.get_plugin_action(10) == CANCEL_BUTTON then return 0 end
                end

                if m_sel == true and t[i]:sub(-1) == "\0" then
                    _print.opt.sel_line(line)
                end

                if i == 1 and hasheader == true then
                    line = disp_header(t[1])
                else
                    line = _print.f("%s", tostring(t[i]))
                end

            end

            i = i + 1 -- important!

            if line == 1 or i > t_count or col_c ~= 0 then
                _print.opt.column(table_p.col)
                i, col_c, row_c, sel = get_input(table_p)
            end

            rb.button_clear_queue() -- keep the button queue from overflowing
        end
        return sel
    end -- display_table
--============================================================================--

    _print.opt.defaults()
    _print.opt.autoupdate(false)
    _print.opt.color(item_fgc, item_bgc, item_selc)

    table_p = init_position(15, 5)
    line, maxline = _print.opt.area(5, 1, rb.LCD_WIDTH - 10, rb.LCD_HEIGHT - 2)
    maxline = math.min(maxline, t_count)

    -- allow user to start at a position other than the beginning
    if start ~= nil then table_p.row = clamp(start, 1, t_count + 1) end

    if hasheader == true then
        table_p.vcursor_min = 2  -- lowest selectable item
        table_p.vcursor     = 2
    end

    table_p.vcursor = curpos or table_p.vcursor_min

    if table_p.vcursor < 1 or table_p.vcursor > maxline then
        table_p.vcursor = table_p.vcursor_min
    end

    _print.opt.sel_line(table_p.vcursor)
    _print.opt.overflow("manual")
    _print.opt.justify(justify)

    local opts = _print.opt.get()
    opts.drawsep = drawsep
    _print.opt.set(opts)


    -- initialize vertical scrollbar
    set_vsb(); do
        local vsb =_print.opt.get()
        if rb.LCD_DEPTH  == 2 then -- invert 2-bit screens
            vsb.fg_pattern = 3 - vsb.fg_pattern
            vsb.bg_pattern = 3 - vsb.bg_pattern
        end

        set_vsb = function (item)
            if t_count > (maxline or t_count) then
                rb.set_viewport(vsb)
                item = item or 0
                local m = maxline / 2 + 1
                rb.gui_scrollbar_draw(vsb.width - 5, vsb.y, 5, vsb.height,
                                      t_count, math.max(0, item - m), 
                                      math.min(item + m, t_count), 0)
            end
        end
    end -- set_vsb
    local selected = display_table(table_p, 0, 1, 0)

    _print.opt.defaults()

    if m_sel == true then -- walk the table to get selected items
        selected = {}
        for i = 1, t_count do
            if t[i]:sub(-1) == "\0" then table.insert(selected, i) end
        end
    end
    --rb.splash(100, string.format("#1 %d, %d, %d", row, vcursor_pos, sel))
   return selected, table_p.row, table_p.vcursor
end --print_table
