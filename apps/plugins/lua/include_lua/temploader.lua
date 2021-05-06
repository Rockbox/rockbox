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
--
temp_loader allows some (pure) lua requires to be loaded and later garbage collected
unfortunately the 'required' module needs to be formatted in such a way to
pass back a reference to a function or call table in order to keep the functions
within from being garbage collected too early

modules that add things to _G table are unaffected by using this function
except if later you use require or temp_loader those tables will again
be reloaded with fresh data since nothing was recorded about the module being loaded

modulename - same as require()
newinstance == true -- get a new copy (from disk) of the module
... other args for the module

BE AWARE this bypasses the module loader
which would allow code reuse so if you aren't careful this memory saving tool
could spell disaster for free RAM if you load the same code multiple times
--]]

local function tempload(modulename, newinstance, ...)
  --http://lua-users.org/wiki/LuaModulesLoader
  local errmsg = ""
  -- Is there current a loaded module by this name?
  if package.loaded[modulename] ~= nil and not newinstance then
    return require(modulename)
  end
  -- Find source
  local modulepath = string.gsub(modulename, "%.", "/")
  for path in string.gmatch(package.path, "([^;]+)") do
    local filename = string.gsub(path, "%?", modulepath)
    --attempt to open and compile module
    local file, err = loadfile(filename)
    if file then
      -- execute the compiled chunk
      return file(... or modulename)
    end
    errmsg = table.concat({errmsg, "\n\tno file '", filename, "' (temp loader)"})
  end
  return nil, errmsg
end

return tempload
