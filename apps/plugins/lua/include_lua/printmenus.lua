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
require("menucoresettings") --loads user settings from rockbox

local _clr = require("color")


local _LCD = rb.lcd_framebuffer()

--[[ -- dpad requires:
local BUTTON = require("menubuttons")
local _timer = require("timer")
]]
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
--[[
local function dpad(x, xi, xir, y, yi, yir, timeout, overflow)
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
]]
--------------------------------------------------------------------------------
-- displays text in menu_t calls function in same indice of func_t when selected
function print_menu(menu_t, func_t, selected, settings, copy_screen)

    local i, start, vcur, screen_img

    if selected then vcur = selected + 1 end
    if vcur and vcur <= 1 then vcur = 2 end

    local c_table = rb.core_color_table or {}

    if not settings then
        settings = {}
        settings.default = true
    end

    settings.justify = settings.justify or "center"
    settings.wrap    = settings.wrap or true
    settings.hfgc    = settings.hfgc or c_table.lst_color or _clr.set( 0, 000, 000, 000)
    settings.hbgc    = settings.hbgc or c_table.bg_color or _clr.set(-1, 255, 255, 255)
    settings.ifgc    = settings.ifgc or c_table.fg_color or _clr.set(-1, 000, 255, 060)
    settings.ibgc    = settings.ibgc or c_table.bg_color or _clr.set( 0, 000, 000, 000)
    settings.iselc   = settings.iselc or c_table.lss_color or _clr.set( 1, 000, 200, 100)

    if not settings.linedesc or rb.core_list_settings_table then
        settings.linedesc = settings.linedesc or {}
        local t_l = rb.core_list_settings_table
        local linedesc = {
                                separator_height = t_l.list_separator_height or 0,
                                show_cursor = (t_l.cursor_style or 0) == 0,
                                style = t_l.cursor_style or 0xFFFF, --just a random non used index
                                show_icons = t_l.show_icons or false,
                                text_color = c_table.fg_color or _clr.set(-1, 000, 255, 060),
                                line_color = c_table.bg_color or _clr.set( 0, 000, 000, 000),
                                line_end_color= c_table.bg_color or _clr.set( 0, 000, 000, 000),
                            }
        local styles = {rb.STYLE_NONE, rb.STYLE_INVERT, rb.STYLE_GRADIENT, rb.STYLE_COLORBAR, rb.STYLE_DEFAULT}
        linedesc.style = styles[linedesc.style + 1] or rb.STYLE_COLORBAR

        for k, v in pairs(linedesc) do
            --dont overwrite supplied settings
            settings.linedesc[k] = settings.linedesc[k] or v
        end
    end

    settings.hasheader = true
    settings.co_routine = nil
    settings.msel = false
    settings.start = start
    settings.curpos = vcur
    --settings.dpad_fn = dpad

    while not i or i > 0 do
        if copy_screen == true then
		--make a copy of screen for restoration
		screen_img = screen_img or rb.new_image()
		screen_img:copy(_LCD)
	else
		screen_img = nil
	end

        _LCD:clear(settings.ibgc)

        settings.start = start
        settings.curpos = vcur

        i, start, vcur = print_table(menu_t, #menu_t, settings)
        --vcur = vcur + 1
	    collectgarbage("collect")
        if copy_screen == true then _LCD:copy(screen_img) end

        if func_t and func_t[i] then
            if func_t[i](i, menu_t, func_t) == true then break end
        else
            break
        end
    end
    if settings.default == true then settings = nil end
    return screen_img, i
end
