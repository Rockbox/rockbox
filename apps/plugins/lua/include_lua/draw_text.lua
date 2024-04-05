--[[ Lua Draw Text function
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
-- draw text onto image if width/height are supplied text is centered

if not rb.lcd_framebuffer then rb.splash(rb.HZ, "No Support!") return nil end
do
    -- Internal Constants
    local rocklib_image = getmetatable(rb.lcd_framebuffer())
    local _LCD = rb.lcd_framebuffer()
    local LCD_W, LCD_H = rb.LCD_WIDTH, rb.LCD_HEIGHT
    local BSAND = 8 -- blits color to dst if src <> 0
    local _NIL = nil -- nil placeholder

    local _clear   = rocklib_image.clear
    local _copy    = rocklib_image.copy
    local _newimg  = rb.new_image


    return function(img, x, y, width, height, font, color, text)
        font = font or rb.FONT_UI


        if rb.lcd_rgbpack ~= _NIL then -- Color target 
            rb.set_viewport(img, {fg_pattern = color, font = font, drawmode = 2})--DRMODE_FG
        else
            if color ~= 0 then color = 3 end--DRMODE_SOLID
            rb.set_viewport(img, {font = font, drawmode = color})
        end

        if width or height then
            local res, w, h = rb.font_getstringsize(text, font)       

            if not width then
                width = 0
            else
                width = (width - w) / 2
            end

            if not height then
                height = 0
            else
                height = (height - h) / 2
            end
            x = width + x
            y = height + y

        end

        rb.lcd_putsxy(x, y, text)

        rb.set_viewport() -- set viewport default
        return res, w, h
    end
end
