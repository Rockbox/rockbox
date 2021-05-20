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
 * Copyright (C) 2021 William Wilgus
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
--menu core settings loaded from rockbox user settings
--Bilgus 4/2021

local function get_core_settings()
    local tmploader = require("temploader")
    -- rbsettings is a large module to have sitting in RAM
    -- if user already has it in RAM then use that
    local rbs_is_loaded = (package.loaded.rbsettings ~= nil)
    local s_is_loaded = (package.loaded.settings ~= nil)
    local rbold = rb

    if not rbs_is_loaded then
        --replace the rb table so we can keep the defines out of the namespace
        rb = { global_settings = rb.global_settings,
               global_status = rb.global_status}
    end

    tmploader("rbsettings")
    tmploader("settings")
    -- these are exact matches color and talk are wildcard matches
    local list_settings = "cursor_style|show_icons|statusbar|scrollbar|scrollbar_width|list_separator_height|backdrop_file|"
    local function filterfn(struct, k)
        k = k or ""
        --rbold.splash(100, struct .. " " .. k)
        return (k:find("color") or k:find("talk") or list_settings:find(k))
    end
    local rb_settings = rb.settings.dump('global_settings', "system", nil, nil, filterfn)

    local color_table = {}
    local talk_table = {}
    local list_settings_table = {}

    for key, value in pairs(rb_settings) do
            key = key or ""
            if (key:find("color")) then
                color_table[key]=value
            elseif (key:find("talk")) then
                talk_table[key]=value
            else --if (list_settings:find(key)) then
                list_settings_table[key]=value
            end
    end

    if not s_is_loaded then
        rb.settings = nil
    end

    rb = rbold
    rb.core_color_table = color_table
    rb.core_talk_table = talk_table
    rb.core_list_settings_table = list_settings_table
end
get_core_settings()
get_core_settings = nil
package.loaded.menucoresettings = nil
collectgarbage("collect")
