--[[ Lua RB Compatibility Operations
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2018 William Wilgus
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

-- [[ compatibility with old functions ]]
if rb.strncasecmp then rb.strcasecmp = function(s1, s2) return rb.strncasecmp(s1, s2) end end

if rb.backlight_brightness_set then
    rb.backlight_set_brightness = function(brightness) rb.backlight_brightness_set(brightness) end
    rb.backlight_brightness_use_setting = function() rb.backlight_brightness_set(nil) end
end

if rb.buttonlight_brightness_set then
    rb.buttonlight_set_brightness = function(brightness) rb.buttonlight_brightness_set(brightness) end
    rb.buttonlight_brightness_use_setting = function() rb.buttonlight_brightness_set(nil) end
end

if rb.mixer_frequency then
    rb.mixer_set_frequency = function(freq) rb.mixer_frequency(freq) end
    rb.mixer_get_frequency = function() return rb.mixer_frequency(nil) end
end

if rb.backlight_onoff then
    rb.backlight_on = function() rb.backlight_onoff(true) end
    rb.backlight_off = function() rb.backlight_onoff(false) end
end

if rb.buttonlight_brightness_set then
    rb.buttonlight_set_brightness = function(brightness) rb.buttonlight_brightness_set(brightness) end
    rb.buttonlight_brightness_use_setting = function() rb.buttonlight_brightness_set(nil) end
end

if rb.touchscreen_mode then
    rb.touchscreen_set_mode = function(mode) rb.touchscreen_mode(mode) end
    rb.touchscreen_get_mode = function() return rb.touchscreen_mode(nil) end
end

if rb.schedule_cpu_boost then
    rb.trigger_cpu_boost = function() rb.schedule_cpu_boost(true) end
    rb.cancel_cpu_boost = function() rb.schedule_cpu_boost(false) end
end
