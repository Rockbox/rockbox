--[[
             __________               __   ___.
   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
                     \/            \/     \/    \/            \/
 $Id$

 Port of test_viewports.c to Lua

 Copyright (C) 2009 by Maurus Cuelenaere

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 KIND, either express or implied.

]]--

-- TODO: outsource this
rb.DRMODE_SOLID = 3
rb.LCD_BLACK = rb.lcd_rgbpack(0, 0, 0)
rb.LCD_WHITE = rb.lcd_rgbpack(255, 255, 255)
rb.LCD_DEFAULT_FG = rb.LCD_WHITE
rb.LCD_DEFAULT_BG = rb.LCD_BLACK

BGCOLOR_1 = rb.lcd_rgbpack(255,255,0)
BGCOLOR_2 = rb.lcd_rgbpack(0,255,0)
FGCOLOR_1 = rb.lcd_rgbpack(0,0,255)

local vp0 =
{
    x        = 0,
    y        = 0,
    width    = rb.LCD_WIDTH,
    height   = 20,
    font     = rb.FONT_UI,
    drawmode = rb.DRMODE_SOLID,
    fg_pattern = rb.LCD_DEFAULT_FG,
    bg_pattern = BGCOLOR_1
}

local vp1 =
{
    x        = rb.LCD_WIDTH / 10,
    y        = 20,
    width    = rb.LCD_WIDTH / 3,
    height   = rb.LCD_HEIGHT / 2,
    font     = rb.FONT_SYSFIXED,
    drawmode = rb.DRMODE_SOLID,
    fg_pattern = rb.LCD_DEFAULT_FG,
    bg_pattern = rb.LCD_DEFAULT_BG
};

local vp2 =
{
    x        = rb.LCD_WIDTH / 2,
    y        = 40,
    width    = rb.LCD_WIDTH / 3,
    height   = (rb.LCD_HEIGHT / 2),
    font     = rb.FONT_UI,
    drawmode = rb.DRMODE_SOLID,
    fg_pattern = FGCOLOR_1,
    bg_pattern = BGCOLOR_2
};


local vp3 =
{
    x        = rb.LCD_WIDTH / 4,
    y        = (5 * rb.LCD_HEIGHT) / 8,
    width    = rb.LCD_WIDTH / 2,
    height   = (rb.LCD_HEIGHT / 4),
    font     = rb.FONT_SYSFIXED,
    drawmode = rb.DRMODE_SOLID,
    fg_pattern = rb.LCD_BLACK,
    bg_pattern = rb.LCD_WHITE
};

rb.set_viewport(vp0)
rb.clear_viewport()
rb.lcd_puts_scroll(0,0,"Viewport testing plugin - this is a scrolling title")

rb.set_viewport(vp1);
rb.clear_viewport();

for i = 0, 3 do
    rb.lcd_puts_scroll(0,i,string.format("Left text, scrolling_line %d",i));
end

rb.set_viewport(vp2);
rb.clear_viewport();
for i = 0, 3 do
    rb.lcd_puts_scroll(1,i,string.format("Right text, scrolling line %d",i));
end

local y = -10
for i = -10, vp2.width + 10, 5 do
    rb.lcd_drawline(i, y, i, vp2.height - y);
end

rb.set_viewport(vp3);
rb.clear_viewport();
for i = 1, 2 do
    rb.lcd_puts_scroll(2,i,string.format("Bottom text, a scrolling line %d",i));
end
rb.lcd_puts_scroll(4,3,"Short line")
rb.lcd_update()

rb.button_get(true)

rb.set_viewport(nil)
