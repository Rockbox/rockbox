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

        local opts = {x = 0, y = 0, width = LCD_W - 1, height = LCD_H - 1,
               font = font, drawmode = 3, fg_pattern = 0x1, bg_pattern = 0}

        if rb.LCD_DEPTH  == 2 then -- invert 2-bit screens
            --vp.drawmode = bit.bxor(vp.drawmode, 4)
            opts.fg_pattern = 3 - opts.fg_pattern
            opts.bg_pattern = 3 - opts.bg_pattern
        end
        rb.set_viewport(opts)

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

        -- make a copy of the current screen for later
        --local screen_img = _newimg(LCD_W, LCD_H)
        local screen_img = _newimg(LCD_W, h * 2)
        _copy(screen_img, _LCD)

        -- check if the screen buffer is supplied image if so set img to the copy
        if img == _LCD then
            img = screen_img
        end

        -- we will be printing the text to the screen then blitting into img
        --rb.lcd_clear_display()
        _clear(_LCD, opts.bg_pattern or 0, 1, 1, LCD_W, h * 2)

        if w > LCD_W then -- text is too long for the screen do it in chunks
            local l = 1
            local resp, wp, hp
            local lenr = text:len()

            while lenr > 1 do
                l = lenr
                resp, wp, hp = rb.font_getstringsize(text:sub(1, l), font)

                while wp >= LCD_W and l > 1 do
                    l = l - 1
                    resp, wp, hp = rb.font_getstringsize(text:sub( 1, l), font)
                end

                rb.lcd_putsxy(0, 0, text:sub(1, l))
                text = text:sub(l)

                if x + width > img:width() or y + height > img:height() then
                    break
                end

                -- using the mask we made blit color into img
                _copy(img, _LCD, x + width, y + height, _NIL, _NIL, _NIL, _NIL, false, BSAND, color)
                x = x + wp
                --rb.lcd_clear_display()
                _clear(_LCD, opts.bg_pattern or 0, 1, 1, LCD_W, h * 2)

                lenr = text:len()
            end
        else --w <= LCD_W
            rb.lcd_putsxy(0, 0, text)

            -- using the mask we made blit color into img
            _copy(img, _LCD, x + width, y + height, _NIL, _NIL, _NIL, _NIL, false, BSAND, color)
        end

        _copy(_LCD, screen_img) -- restore screen
        rb.set_viewport() -- set viewport default
        return res, w, h
    end
end
