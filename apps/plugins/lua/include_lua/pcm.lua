--[[ Lua RB pcm Operations
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

-- [[ conversion to old style pcm_ functions ]]
if not rb.pcm then rb.splash(rb.HZ, "No Support!") return nil end

rb.pcm_apply_settings = function() rb.pcm("applysettings") end
rb.pcm_set_frequency = function(freq) rb.pcm("setfrequency", freq) end
rb.pcm_play_pause = function(bplay) rb.pcm("playpause", bplay) end
rb.pcm_play_stop = function() rb.pcm("playstop") end
rb.pcm_play_lock = function() rb.pcm("playlock") end
rb.pcm_play_unlock = function() rb.pcm("playunlock") end
rb.pcm_is_playing = function() return rb.pcm("isplaying") end
rb.pcm_is_paused = function() return rb.pcm("ispaused") end
rb.pcm_calculate_peaks = function() return rb.pcm("calculatepeaks") end
rb.pcm_get_bytes_waiting = function() return rb.pcm("getbyteswaiting") end
