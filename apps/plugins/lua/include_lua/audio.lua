--[[ Lua RB Audio Operations
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

-- [[ conversion to old style audio_ functions ]]
if not rb.audio then rb.splash(rb.HZ, "No Support!") return nil end

rb.audio_status = function() return rb.audio("status") end
rb.audio_play = function (elapsed, offset) rb.audio("play", elapsed, offset) end
rb.audio_stop = function() rb.audio("stop") end
rb.audio_pause = function() rb.audio("pause") end
rb.audio_resume = function() rb.audio("resume") end
rb.audio_next = function() rb.audio("next") end
rb.audio_prev = function() rb.audio("prev") end
rb.audio_ff_rewind = function (newtime) rb.audio("ff_rewind", newtime) end
rb.audio_flush_and_reload_tracks = function() rb.audio("flush_and_reload_tracks") end
rb.audio_get_file_pos = function() return rb.audio("get_file_pos") end
