--[[
             __________               __   ___.
   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
                     \/            \/     \/    \/            \/
 Example Lua File Viewer script
 Copyright (C) 2020 William Wilgus
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 KIND, either express or implied.
]]--

require("actions")   -- Contains rb.actions & rb.contexts
-- require("buttons") -- Contains rb.buttons -- not needed for this example

--local _timer = require("timer")
--local _clr   = require("color") -- clrset, clrinc provides device independent colors
local _lcd   = require("lcd")   -- lcd helper functions
--local _print = require("print") -- advanced text printing
--local _img   = require("image") -- image manipulation save, rotate, resize, tile, new, load
--local _blit  = require("blit") -- handy list of blit operations
--local _draw  = require("draw") -- draw all the things (primitives)
--local _math  = require("math_ex") -- missing math sine cosine, sqrt, clamp functions


local scrpath = rb.current_path()--rb.PLUGIN_DIR .. "/demos/lua_scripts/"

package.path = scrpath .. "/?.lua;" .. package.path --add lua_scripts directory to path

require("printmenus") --menu
require("filebrowse") -- file browser
require("file_attrs")

rb.actions = nil
package.loaded["actions"] = nil

-- uses print_table to display a menu
function main_menu()
    local mt =  {
                [1] = "Rocklua File Browser Example",
                [2] = "Sort by Name",
                [3] = "Sort by Size",
                [4] = "Sort by Date",
                [5] = "Sort by Type",
                [6] = "Exit"
                }

    local ft =  {
                [0] = exit_now, --if user cancels do this function
                [1] = function(TITLE) return true end, -- shouldn't happen title occupies this slot
                [2]  = function(SBNAME)
                            _lcd:splashf(rb.HZ, "%s", file_choose("/", "", "name") or "None")
                        end,
                [3]  = function(SBSIZE)
                            local file, attr = file_choose("/", "", "size", true, true)
                            if file and attr then
                                local sz = tonumber(string.match(attr, ".+Sz:(%d+)") or 0)
                                if sz > 1024 then
                                    _lcd:splashf(rb.HZ, "%s", file .. " " .. sz / 1024 .. " kiB")
                                else
                                    _lcd:splashf(rb.HZ, "%s", file .. " " .. sz .. " B")
                                end
                            else
                                _lcd:splashf(rb.HZ, "%s", file or "None")
                            end
                            --_lcd:splashf(rb.HZ, "%s", file_choose("/", "", "size") or "None")
                        end,
                [4]  = function(SBDATE)
                            local file, attr = file_choose("/", "", "date", true, true)
                            if file and attr then
                                local tm = tonumber(string.match(attr, ".+Tm:(%d+)") or 0) 
                                _lcd:splashf(rb.HZ, "%s", file .. " " .. attr)--" Tm:" .. tm)
                            else
                                _lcd:splashf(rb.HZ, "%s", file or "None")
                            end
                            --_lcd:splashf(rb.HZ, "%s", file_choose("/", "", "date") or "None")
                        end,
                [5]  = function(SBTYPE)
                            local file = file_choose("/", "", "type")
                            if file then
                                local known = false
                                local attr = rb.filetype_get_attr(file)
                                for k, v in pairs(rb) do
                                    if nil ~= string.find (k, "^FILE_ATTR_(.+)") then
                                        if v == attr then
                                            file = file .. " " .. k
                                            known = true
                                            break
                                        end
                                    end
                                end
                                if not known then 
                                    file = file .. " Unknown Type " .. string.match(file, "%.[^%.]+$") or ""
                                end
                            end
                            _lcd:splashf(rb.HZ, "%s", file or "None")
                        end,
                [6] = function(EXIT_) return true end
                }

    print_menu(mt, ft)

end

function exit_now()
    _lcd:update()
    os.exit()
end -- exit_now

main_menu()
exit_now()
