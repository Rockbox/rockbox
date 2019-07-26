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
local _clr = require("color")

local _LCD = rb.lcd_framebuffer()
--------------------------------------------------------------------------------
-- displays text in menu_t calls function in same indice of func_t when selected
function print_menu(menu_t, func_t, selected, settings, copy_screen)

    local i, start, vcur, screen_img   

    if selected then vcur = selected + 1 end
    if vcur and vcur <= 1 then vcur = 2 end

    if not settings then
        settings = {}
        settings.justify = "center"
        settings.wrap    = true
        settings.hfgc    = _clr.set( 0, 000, 000, 000)
        settings.hbgc    = _clr.set(-1, 255, 255, 255)
        settings.ifgc    = _clr.set(-1, 000, 255, 060)
        settings.ibgc    = _clr.set( 0, 000, 000, 000)
        settings.iselc   = _clr.set( 1, 000, 200, 100)
	settings.default = true
    end

    settings.hasheader = true
    settings.co_routine = nil
    settings.msel = false
    settings.start = start
    settings.curpos = vcur

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
            if func_t[i](i, menu_t) == true then break end
        else
            break
        end
    end
    if settings.default == true then settings = nil end
    return screen_img
end
