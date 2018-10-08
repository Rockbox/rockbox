--[[ Lua LCD Wrapper functions
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

    _lcd.clear
    _lcd.duplicate
    _lcd.image
    _lcd.set_viewport
    _lcd.splashf
    _lcd.text_extent
    _lcd.update
    _lcd.update_rect

-- Exposed Constants
    _lcd.CX
    _lcd.CY
    _lcd.DEPTH
    _lcd.W
    _lcd.H

    _lcd
    _LCD

]]
if not rb.lcd_framebuffer then rb.splash(rb.HZ, "No Support!") return nil end

_LCD = rb.lcd_framebuffer()

local _lcd = {} do

    --internal constants
    local _NIL = nil -- _NIL placeholder

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

    -- return a copy of lcd screen
    _lcd.duplicate = function(t, screen_img)
        screen_img = screen_img or rb.new_image()
        screen_img:copy(rb.lcd_framebuffer())
        return screen_img
    end

    -- updates screen in specified rectangle
    _lcd.update_rect = function(t, x, y, w, h)
        rb.lcd_update_rect(x - 1, y - 1,
             clamp(x + w, 1, rb.LCD_WIDTH) - 1,
             clamp(y + h, 1, rb.LCD_HEIGHT) - 1)
    end

    -- clears lcd, optional.. ([color, x1, y1, x2, y2, clip])
    _lcd.clear = function(t, clr, ...)
        rb.lcd_scroll_stop() --rb really doesn't like bg change while scroll
        if clr == _NIL and ... == _NIL then
            rb.lcd_clear_display()
        else
            _LCD:clear(clr, ...)
        end
    end

    -- loads an image to the screen
    _lcd.image = function(t, src, x, y)
        if not src then --make sure an image was passed, otherwise bail
            rb.splash(rb.HZ, "No Image!")
            return _NIL
        end
        _LCD:copy(src,x,y,1,1)
    end

    -- Formattable version of splash
    _lcd.splashf = function(t, timeout, ...)
        rb.splash(timeout, string.format(...))
    end

    -- Gets size of text
    _lcd.text_extent = function(t, msg, font)
        font = font or rb.FONT_UI

        return rb.font_getstringsize(msg, font)
    end

    -- Sets viewport size
    _lcd.set_viewport = function(t, vp)
        if not vp then rb.set_viewport() return end
        if rb.LCD_DEPTH  == 2 then -- invert 2-bit screens
            --vp.drawmode = bit.bxor(vp.drawmode, 4)
            vp.fg_pattern = 3 - vp.fg_pattern
            vp.bg_pattern = 3 - vp.bg_pattern
        end
        rb.set_viewport(vp)
    end

    -- allows the use of _lcd() as a identifier for the screen
    local function index(k, v)
        return function(x, ...)
            _LCD[v](_LCD, ...)
        end
    end

    -- allows the use of _lcd() as a identifier for the screen
    local function call()
        return rb.lcd_framebuffer()
    end

    --expose functions to the outside through _lcd table
    _lcd.update       = rb.lcd_update
    _lcd.DEPTH        = rb.LCD_DEPTH
    _lcd.W            = rb.LCD_WIDTH
    _lcd.H            = rb.LCD_HEIGHT
    _lcd.CX           = (rb.LCD_WIDTH / 2)
    _lcd.CY           = (rb.LCD_HEIGHT / 2)
    _lcd = setmetatable(_lcd,{__index = index, __call = call})

end -- _lcd functions

return _lcd

