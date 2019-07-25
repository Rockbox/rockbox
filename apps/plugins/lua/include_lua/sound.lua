--[[ Lua RB sound Operations
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2019 William Wilgus
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

-- [[ conversion to old style sound_ functions ]]
if not rb.sound then rb.splash(rb.HZ, "No Support!") return nil end

require "sound_defines"

rb.sound_set = function(s, v) return rb.sound("set", s, v) end
rb.sound_current = function(s) return rb.sound("current", s) end
rb.sound_default = function(s) return rb.sound("default", s) end
rb.sound_min = function(s) return rb.sound("min", s) end
rb.sound_max = function(s) return rb.sound("max", s) end
rb.sound_unit = function(s) return rb.sound("unit", s) end
rb.sound_pitch = function(s) return rb.sound("pitch", s) end
rb.sound_val2phys = function(s, v) return rb.sound("val2phys", s, v) end
