--[[ Lua Print functions
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

    _print.clear
    _print.f
    _print.opt
    _print.opt.area
    _print.opt.autoupdate
    _print.opt.color
    _print.opt.column
    _print.opt.defaults
    _print.opt.get
    _print.opt.justify
    _print.opt.line
    _print.opt.overflow
    _print.opt.sel_line
    _print.opt.set

]]

if not rb.lcd_framebuffer then rb.splash(rb.HZ, "No Support!") return nil end

local _print = {} do

    -- internal constants
    local _clr = require("color") -- _clr functions required

    local _NIL = nil -- _NIL placeholder
    local _LCD = rb.lcd_framebuffer()
    local WHITE = _clr.set(-1, 255, 255, 255)
    local BLACK = _clr.set(0, 0, 0, 0)
    local DRMODE_SOLID = 3
    local col_buf, s_lines = {}, {}
    local _p_opts = _NIL
    local tabstring = string.rep(" ", 2)
-- print internal helper functions
--------------------------------------------------------------------------------
   -- clamps value to >= min and <= max
    local function clamp(val, min, max)
        -- Warning doesn't check if min < max
        if val < min then
            return min
        elseif val < max then
            return val
        end
        return max
    end

    -- Gets size of text
    local function text_extent(msg, font)
        -- res, w, h
        return rb.font_getstringsize(msg, font or rb.FONT_UI)
    end

    -- Updates a single line on the screen
    local function update_line(enabled, opts, line, h)
        if enabled ~= true then return end
        local o = opts or _p_opts
    -- updates screen in specified rectangle
        rb.lcd_update_rect(o.x - 1, o.y + line * h,
             clamp(o.x + o.width, 1, rb.LCD_WIDTH) - 1,
             clamp(o.y + line * h + 1 + h, 1, rb.LCD_HEIGHT) - 1)
    end

    -- Clears a single line on the screen
    local function clear_line(opts, line, h)
        local o = opts or _p_opts
        _LCD:clear(o.bg_pattern, o.x, o.y + line * h + 1,
                                 o.x + o.width, line * h + h + o.y)
    end

    -- Sets the maximum number of lines on the screen
    local function max_lines(opts)
        local h  = opts.height
        local _, _, th = text_extent("W", opts.font)
        return h / th
    end

    --saves the items displayed for side to side scroll
    local function col_buf_insert(msg, line, _p_opts)
        --if _p_opts.line <= 1 then col_buf = {} end
        if not col_buf[line] then
            table.insert(col_buf, line, msg)
        end
    end

    --replaces / strips tab characters
    local function check_escapes(o, msg)
--[[    --for replacing a variety of escapes
        local tabsz  = 2
        local tabstr = string.rep(" ", tabsz)
        local function repl(esc)
            local ret = ""
            if esc:sub(1,1) == "\t" then ret = string.rep(tabstr, esc:len()) end
            return ret
        end
        msg = msg:gsub("(%c+)", repl)
]]
        msg = msg:gsub("\t", tabstring)
        local res, w, h = text_extent(msg, o.font)
        return w, h, msg
    end
--------------------------------------------------------------------------------
    local function set_linedesc(t_linedesc, opts)
        local o = opts or _print.opt.get(true)
        --local out = function() local t = {} for k, v in pairs(o) do t[#t + 1] = tostring(k) t[#t + 1] = tostring(v) end return table.concat(t, "\n") end
        --rb.splash_scroller(1000, out())
        local linedesc ={
                      --These are the defaults - changes will be made below if you supplied t_linedesc
                      indent = 0, -- internal indent text
                      line = 0, -- line index within group
                      nlines = 1, -- selection grouping
                      offset = 0, -- internal item offset
                      scroll = true,
                      selected = false, --internal
                      separator_height = 0,
                      line_separator = false,
                      show_cursor = false,
                      show_icons = false,
                      icon = -1,
                      icon_fn = function() return -1 end,
                      style = rb.STYLE_COLORBAR,
                      text_color = o.fg_pattern or WHITE,
                      line_color = o.bg_pattern or BLACK,
                      line_end_color= o.bg_pattern or BLACK,
                    }
        if type(t_linedesc) == "table" then

            if not o.linedesc then
                o.linedesc = {}
                for k, v in pairs(linedesc) do
                    o.linedesc[k] = v
                end
            end

            for k, v in pairs(t_linedesc) do
                o.linedesc[k] = v
            end
            if o.linedesc.separator_height > 0 then
                o.linedesc.line_separator = true
            end
            return
        end
        o.linedesc = linedesc
        return o.linedesc
    end

    -- set defaults for print view
    local function set_defaults()
        _p_opts = { x = 1,
                    y = 1,
                    width = rb.LCD_WIDTH - 1,
                    height = rb.LCD_HEIGHT - 1,
                    font = rb.FONT_UI,
                    drawmode = DRMODE_SOLID,
                    fg_pattern = WHITE,
                    bg_pattern = BLACK,
                    sel_pattern = WHITE,
                    line = 1,
                    max_line = _NIL,
                    col = 0,
                    header = false, --Internal use - treats first entry as header
                    ovfl = "auto", -- auto, manual, none
                    justify = "left", --left, center, right
                    autoupdate = true, --updates screen as items are added
                    drawsep = false,   -- separator between items
                  }
        set_linedesc(nil, _p_opts) -- default line display
        _p_opts.max_line = max_lines(_p_opts)

        s_lines, col_buf = {}, {}
        return _p_opts
    end

    -- returns table with settings for print
    -- if bByRef is _NIL or false then a copy is returned
    local function get_settings(bByRef)
        _p_opts = _p_opts or set_defaults()
        if not bByRef then
            -- shallow copy of table
            local copy = {}
            for k, v in pairs(_p_opts) do
                copy[k] = v
            end
            return copy
        end

        return _p_opts
    end

    -- sets the settings for print with your passed table
    local function set_settings(t_opts)
        _p_opts = t_opts or set_defaults()
        if t_opts then
            _p_opts.max_line = max_lines(_p_opts)
            col_buf = {}
        end
    end

    -- sets colors for print
    local function set_color(fgclr, bgclr, selclr)
        local o = get_settings(true)

        if fgclr ~= _NIL then
            o.fg_pattern, o.sel_pattern = fgclr, fgclr
        end
        o.sel_pattern = selclr or o.sel_pattern
        o.bg_pattern = bgclr or o.bg_pattern
    end

    -- helper function sets up colors/marker for selected items
    local function show_selected(iLine, msg)
        if rb.LCD_DEPTH  == 2 then -- invert 2-bit screens
            local o = get_settings() -- using a copy of opts so changes revert
            if not o then rb.set_viewport() return end
            o.fg_pattern = 3 - o.fg_pattern
            o.bg_pattern = 3 - o.bg_pattern
            rb.set_viewport(o)
            o = _NIL
        else
            show_selected = function() end -- no need to check again
        end
    end

    -- sets line explicitly or increments line if line is _NIL
    local function set_line(iLine)
        local o = get_settings(true)

        o.line = iLine or o.line + 1

        if(o.line < 1 or o.line > o.max_line) then
            o.line = 1
        end
    end

    -- clears the set print area
    local function clear()
        local o = get_settings(true)
        _LCD:clear(o.bg_pattern, o.x, o.y, o.x + o.width, o.y + o.height)
        if o.autoupdate == true then rb.lcd_update() end
        rb.lcd_scroll_stop()
        set_line(1)
        for i=1, #col_buf do col_buf[i] = _NIL end
        s_lines = {}
        collectgarbage("collect")
    end

    -- screen update after each call to print.f
    local function set_update(bAutoUpdate)
        local o = get_settings(true)
        o.autoupdate = bAutoUpdate or false
    end

    -- sets print area
    local function set_area(x, y, w, h)
        local o = get_settings(true)
        o.x, o.y = clamp(x, 1, rb.LCD_WIDTH), clamp(y, 1, rb.LCD_HEIGHT)
        o.width, o.height = clamp(w, 1, rb.LCD_WIDTH - o.x), clamp(h, 1, rb.LCD_HEIGHT - o.y)
        o.max_line = max_lines(_p_opts)

        clear()
        return o.line, o.max_line
    end

    -- when string is longer than print width scroll -- "auto", "manual", "none"
    local function set_overflow(str_mode)
        -- "auto", "manual", "none"
        local str_mode = str_mode or "auto"
        local o = get_settings(true)
        o.ovfl = str_mode:lower()
        col_buf = {}
    end

    -- aligns text to: "left", "center", "right"
    local function set_justify(str_mode)
        -- "left", "center", "right"
        local str_mode = str_mode or "left"
        local o = get_settings(true)
        o.justify = str_mode:lower()
    end

    -- selects line
    local function select_line(iLine)
        s_lines[iLine] = true
    end

   -- Internal print function
    local function print_internal(t_opts, x, w, h, msg)

        local linedesc
        local line_separator = false
        local o = t_opts
        local ld = o.linedesc or set_linedesc()
        local show_cursor = ld.show_cursor or 0
        local line_indent = 0

        local linestyle = ld.style or rb.STYLE_COLORBAR

        if o.justify ~= "left" then
            line_indent = (o.width - w) --"right"
            if o.justify == "center" then
                line_indent = line_indent / 2
            end
        end

        local line = o.line - 1 -- rb is 0-based lua is 1-based

        if o.ovfl == "manual" then --save msg for later side scroll
            col_buf_insert(msg, o.line, o)
        end

        -- bit of a pain to set the fields this way but its much more efficient than iterating a table to set them
        local function set_desc(tld, scroll, separator_height, selected, style, indent, text_color, line_color, line_end_color)
            tld.scroll = scroll
            tld.separator_height = separator_height
            tld.selected = selected
            tld.style = style
            tld.indent = indent
            tld.text_color = text_color
            tld.line_color = line_color
            tld.line_end_color = line_end_color
        end

        line_separator = ld.line_separator or o.drawsep
        local indent = line_indent < 0 and 0 or line_indent --rb scroller doesn't like negative offset!
        if o.line == 1 and o.header then
            set_desc(ld, true, 1, false, rb.STYLE_DEFAULT,
                     indent, o.fg_pattern, o.bg_pattern, o.bg_pattern)
            ld.show_cursor = false
        elseif s_lines[o.line] then
            --/* Display line selector */
            local style = show_cursor == true and rb.STYLE_DEFAULT or linestyle
            local ovfl = (o.ovfl == "auto" and w >= o.width and x == 0)
            set_desc(ld, ovfl, 0, true, style, indent,
                     o.bg_pattern, o.sel_pattern, o.sel_pattern)
        else
            set_desc(ld, false, 0, false, rb.STYLE_DEFAULT,line_indent,
                     o.fg_pattern, o.bg_pattern, o.bg_pattern)
        end

        if ld.show_icons then
            ld.icon = ld.icon_fn(line, ld.icon or -1)
        end

        rb.lcd_put_line(x, line *h, msg, ld)

        ld.show_cursor = show_cursor
        ld.style = linestyle
        if  line_separator then
            if ld.selected == true then
                rb.set_viewport(o) -- revert drawmode if selected
            end
            if not o.header then
                rb.lcd_drawline(0, line * h, o.width, line * h)
            end
            rb.lcd_drawline(0, line * h + h, o.width, line * h + h) --only to add the last line
            -- but we don't have an idea which line is the last line here so every line is the last line!
        end

        --only update the line we changed
        update_line(o.autoupdate, o, line, h)

        set_line(_NIL) -- increments line counter
    end

    -- Helper function that acts mostly like a normal printf() would
    local function printf(fmt, v1, ...)
        local o = get_settings(true)
        local w, h, msg, rep
        local line = o.line - 1 -- rb is 0-based lua is 1-based

        if not (fmt) or (fmt) == "\n" then -- handles blank line / single '\n'
             local res, w, h = text_extent(" ", o.font)

            clear_line(o, line, h)
            update_line(o.autoupdate, o, line, h)

            if (fmt) then set_line(_NIL) end

            return o.line, o.max_line, o.width, h
        end

        fmt, rep = fmt.gsub(fmt or "", "%%h", "%%s")
        o.header = (rep == 1)

        msg = string.format(fmt, v1, ...)

        show_selected(o.line, msg)

        w, h, msg = check_escapes(o, msg)

        print_internal(o, o.col, w, h, msg)

        return o.line, o.max_line, w, h
    end

    -- x > 0 scrolls right x < 0 scrolls left
    local function set_column(x)
        local o = get_settings()
        if o.ovfl ~= "manual" then return end -- no buffer stored to scroll
        rb.lcd_scroll_stop()

        local res, w, h, str, line

        for key, value in pairs(col_buf) do
            line = key - 1 -- rb is 0-based lua is 1-based
            o.line = key

            if value then
                show_selected(key, value)
                res, w, h = text_extent(value, o.font)
                clear_line(o, line, h)

                print_internal(o, x + o.col, w, h, value)
                update_line(o.autoupdate, o, line, h)
            end
        end
        o = _NIL
    end

    --expose functions to the outside through _print table
    _print.opt            = {}
    _print.opt.column     = set_column
    _print.opt.color      = set_color
    _print.opt.area       = set_area
    _print.opt.set        = set_settings
    _print.opt.get        = get_settings
    _print.opt.defaults   = set_defaults
    _print.opt.overflow   = set_overflow
    _print.opt.justify    = set_justify
    _print.opt.sel_line   = select_line
    _print.opt.line       = set_line
    _print.opt.linedesc   = set_linedesc
    _print.opt.autoupdate = set_update
    _print.selected       = function() return s_lines end
    _print.clear = clear
    _print.f     = printf

end --_print functions

return _print

