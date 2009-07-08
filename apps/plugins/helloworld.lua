--[[
             __________               __   ___.
   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
                     \/            \/     \/    \/            \/
 $Id$

 Example Lua script

 Copyright (C) 2009 by Maurus Cuelenaere

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 KIND, either express or implied.

]]--

require("actions")   -- Contains rb.actions & rb.contexts
--require("buttons") -- Contains rb.buttons; not needed in this example

-- Example function which splashes a message for x seconds
function sayhello(seconds)
    local message = string.format("Hello world from LUA for %d seconds", seconds)
    rb.splash(seconds * rb.HZ, message)
end

-- Helper function which draws a (transparent) image at the center of the screen
function draw_image(img)
    local x, y = (rb.LCD_WIDTH - img:width()) / 2, (rb.LCD_HEIGHT - img:height()) / 2

    local func = rb.lcd_bitmap_transparent_part
              or rb.lcd_bitmap_part      -- Fallback version for grayscale targets
              or rb.lcd_mono_bitmap_part -- Fallback version for mono targets

    func(img, 0, 0, img:width(), x, y, img:width(), img:height())
    rb.lcd_update()
end

-- Helper function that acts like a normal printf() would do
local line = 0
function printf(...)
    local msg = string.format(...)
    local res, w, h = rb.font_getstringsize(msg, rb.FONT_UI)

    if(w >= rb.LCD_WIDTH) then
        rb.lcd_puts_scroll(0, line, msg)
    else
        rb.lcd_puts(0, line, msg)
    end
    rb.lcd_update()

    line = line + 1

    if(h * line >= rb.LCD_HEIGHT) then
        line = 0
    end
end

-- Helper function which reads the contents of a file
function file_get_contents(filename)
    local file = io.open(filename, "r")
    if not file then
        return nil
    end

    local contents = file:read("*all") -- See Lua manual for more information
    file:close() -- GC takes care of this if you would've forgotten it

    return contents
end

-- Helper function which saves contents to a file
function file_put_contents(filename, contents)
    local file = io.open(filename, "w+") -- See Lua manual for more information
    if not file then
        return false
    end

    local ret = file:write(contents) == true
    file:close() -- GC takes care of this if you would've forgotten it
    return ret
end

-- Clear the screen
rb.lcd_clear_display()

-- Draw an X on the screen
rb.lcd_drawline(0, 0, rb.LCD_WIDTH, rb.LCD_HEIGHT)
rb.lcd_drawline(rb.LCD_WIDTH, 0, 0, rb.LCD_HEIGHT)

if(rb.lcd_rgbpack ~= nil) then -- Only do this when we're on a color target, i.e. when LCD_RGBPACK is available
    local rectangle = rb.new_image(10, 15) -- Create a new image with width 10 and height 15
    for i=1, 10 do
        for j=1, 15 do
            rectangle:set(i, j, rb.lcd_rgbpack(200, i*20, j*20)) -- Set the pixel at position i, j to the specified color
        end
    end

    -- rb.lcd_bitmap_part(src, src_x, src_y, stride, x, y, width, height)
    rb.lcd_bitmap_part(rectangle, 0, 0, 10, rb.LCD_WIDTH/2-5, rb.LCD_HEIGHT/10-1, 10, 10) -- Draws our rectangle at the top-center of the screen
end

-- Load a BMP file in the variable backdrop
local backdrop = rb.read_bmp_file("/.rockbox/icons/tango_small_viewers.bmp")      -- This image should always be present...
              or rb.read_bmp_file("/.rockbox/icons/tango_small_viewers_mono.bmp") -- ... if not, try using the mono version...
              or rb.read_bmp_file("/.rockbox/icons/viewers.bmp")                  -- ... or try using the builtin version

-- Draws the image using our own draw_image() function; see up
draw_image(backdrop)

-- Flush the contents from the framebuffer to the LCD
rb.lcd_update()

-- Sleep for 2 seconds
local seconds = 2
rb.sleep(seconds * rb.HZ) -- rb.HZ equals to the amount of ticks that fit in 1 second

-- Call the function sayhello() with arguments seconds
sayhello(seconds)

-- Clear display
rb.lcd_clear_display()

-- Construct a pathname using the current path and a file name
local pathname = rb.current_path() .. "test.txt"

-- Put the string 'Hello from Lua!' in pathname
file_put_contents(pathname, "Hello from Lua!")

-- Splash the contents of pathname
printf(file_get_contents(pathname))

line = line + 1 -- Add empty line

-- Some examples of how bitlib works
printf("1 | 2 = %d <> 7 & 3 = %d", bit.bor(1, 2), bit.band(7, 3))
printf("~8 = %d <> 5 ^ 1 = %d", bit.bnot(8), bit.bxor(5, 1))

line = line + 1 -- Add empty line

-- Display some long lines
for i=0, 4 do
    printf("This is a %slong line!", string.rep("very very very ", i))
    rb.yield() -- Always try to yield to give other threads some breathing space!
end

-- Loop until the user exits the program
repeat
    local action = rb.get_action(rb.contexts.CONTEXT_STD, rb.HZ)
until action == rb.actions.ACTION_STD_CANCEL

-- For a reference list of all functions available, see apps/plugins/lua/rocklib.c (and apps/plugin.h)
