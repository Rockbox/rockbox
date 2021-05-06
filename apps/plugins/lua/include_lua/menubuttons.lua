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
-- Bilgus 4/2021
local oldrb = rb
local tmploader = require("temploader")

local a_is_loaded = (package.loaded.actions ~= nil)
local rbold = rb

if not a_is_loaded then
    --replace the rb table so we can keep the defines out of the namespace
    rb = {}
end

--require("actions")   -- Contains rb.actions & rb.contexts
local actions, err = tmploader("actions")
if err then
    error(err)
end

-- Menu Button definitions --
local button_t = {
    CANCEL = rb.actions.PLA_CANCEL,
    DOWN = rb.actions.PLA_DOWN,
    DOWNR = rb.actions.PLA_DOWN_REPEAT,
    EXIT = rb.actions.PLA_EXIT,
    LEFT = rb.actions.PLA_LEFT,
    LEFTR = rb.actions.PLA_LEFT_REPEAT,
    RIGHT = rb.actions.PLA_RIGHT,
    RIGHTR = rb.actions.PLA_RIGHT_REPEAT,
    SEL = rb.actions.PLA_SELECT,
    SELREL = rb.actions.PLA_SELECT_REL,
    SELR = rb.actions.PLA_SELECT_REPEAT,
    UP = rb.actions.PLA_UP,
    UPR = rb.actions.PLA_UP_REPEAT,
}

rb = oldrb
return button_t
