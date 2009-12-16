--[[
             __________               __   ___.
   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
                     \/            \/     \/    \/            \/
 $Id$

 Port of Stopwatch to Lua for touchscreen targets.
 Original copyright: Copyright (C) 2004 Mike Holden

 Copyright (C) 2009 by Maurus Cuelenaere

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 KIND, either express or implied.

]]--

require "actions"
require "buttons"

STOPWATCH_FILE = "/.rockbox/rocks/apps/stopwatch.dat"


local LapsView = {
    lapTimes = {},
    timer = {
        counting = false,
        prevTotal = 0,
        startAt = 0,
        current = 0
    },
    vp = {
        x = 80,
        y = 0,
        width = rb.LCD_WIDTH - 80,
        height = rb.LCD_HEIGHT,
        font = rb.FONT_UI,
        fg_pattern = rb.lcd_get_foreground()
    },
    scroll = {
        prevY = 0,
        cursorPos = 0
    }
}

function LapsView:init()
    local _, _, h = rb.font_getstringsize("", self.vp.font)

    self.vp.maxLaps = self.vp.height / h
    self.vp.lapHeight = h

    self:loadState()
end

function LapsView:display()
    rb.set_viewport(self.vp)
    rb.clear_viewport()

    local nrOfLaps =  math.min(self.vp.maxLaps, #self.lapTimes)
    rb.lcd_puts_scroll(0, 0, ticksToString(self.timer.current))

    for i=1, nrOfLaps do
        local idx = #self.lapTimes - self.scroll.cursorPos - i + 1
        if self.lapTimes[idx] ~= nil then
            rb.lcd_puts_scroll(0, i, ticksToString(self.lapTimes, idx))
        end
    end

    rb.set_viewport(nil)
end

function LapsView:checkForScroll(btn, x, y)
    if x > self.vp.x and x < self.vp.x + self.vp.width and
       y > self.vp.y and y < self.vp.y + self.vp.height then

        if bit.band(btn, rb.buttons.BUTTON_REL) == rb.buttons.BUTTON_REL then
            self.scroll.prevY = 0
        else
            if #self.lapTimes > self.vp.maxLaps and self.scroll.prevY ~= 0 then
                self.scroll.cursorPos = self.scroll.cursorPos -
                                        (y - self.scroll.prevY) / self.vp.lapHeight

                local maxLaps =  math.min(self.vp.maxLaps, #self.lapTimes)
                if self.scroll.cursorPos < 0 then
                    self.scroll.cursorPos = 0
                elseif self.scroll.cursorPos >= maxLaps then
                    self.scroll.cursorPos = maxLaps
                end
            end

            self.scroll.prevY = y
        end

       return true
    else
        return false
    end
end

function LapsView:incTimer()
    if self.timer.counting then
        self.timer.current = self.timer.prevTotal + rb.current_tick()
                                                  - self.timer.startAt
    else
        self.timer.current = self.timer.prevTotal
    end
end

function LapsView:startTimer()
    self.timer.startAt = rb.current_tick()
    self.timer.currentLap = self.timer.prevTotal
    self.timer.counting = true
end

function LapsView:stopTimer()
    self.timer.prevTotal = self.timer.prevTotal + rb.current_tick()
                                                - self.timer.startAt
    self.timer.counting = false
end

function LapsView:newLap()
    table.insert(self.lapTimes, self.timer.current)
end

function LapsView:resetTimer()
    self.lapTimes = {}
    self.timer.counting = false
    self.timer.current, self.timer.prevTotal, self.timer.startAt = 0, 0, 0
    self.scroll.cursorPos = 0
end

function LapsView:saveState()
    local fd = assert(io.open(STOPWATCH_FILE, "w"))

    for _, v in ipairs({"current", "startAt", "prevTotal", "counting"}) do
        assert(fd:write(tostring(self.timer[v]) .. "\n"))
    end
    for _, v in ipairs(self.lapTimes) do
        assert(fd:write(tostring(v) .. "\n"))
    end

    fd:close()
end

function LapsView:loadState()
    local fd = io.open(STOPWATCH_FILE, "r")
    if fd == nil then return end

    for _, v in ipairs({"current", "startAt", "prevTotal"}) do
        self.timer[v] = tonumber(fd:read("*line"))
    end
    self.timer.counting = toboolean(fd:read("*line"))

    local line = fd:read("*line")
    while line do
        table.insert(self.lapTimes, tonumber(line))
        line = fd:read("*line")
    end

    fd:close()
end

local Button = {
    x = 0,
    y = 0,
    width = 80,
    height = 50,
    label = ""
}

function Button:new(o)
    local o = o or {}

    if o.label then
        local _, w, h = rb.font_getstringsize(o.label, LapsView.vp.font)
        o.width = 5 * w / 4
        o.height = 3 * h / 2
    end

    setmetatable(o, self)
    self.__index = self
    return o
end

function Button:draw()
    local _, w, h = rb.font_getstringsize(self.label, LapsView.vp.font)
    local x, y = (2 * self.x + self.width - w) / 2, (2 * self.y + self.height - h) / 2

    rb.lcd_drawrect(self.x, self.y, self.width, self.height)
    rb.lcd_putsxy(x, y, self.label)
end

function Button:isPressed(x, y)
    return x > self.x and x < self.x + self.width and
           y > self.y and y < self.y + self.height
end

-- Helper function
function ticksToString(laps, lap)
    local ticks = type(laps) == "table" and laps[lap] or laps
    lap = lap or 0

    local hours = ticks / (rb.HZ * 3600)
    ticks = ticks - (rb.HZ * hours * 3600)
    local minutes = ticks / (rb.HZ * 60)
    ticks = ticks - (rb.HZ * minutes * 60)
    local seconds = ticks / rb.HZ
    ticks = ticks - (rb.HZ * seconds)
    local cs = ticks

    if (lap == 0) then
        return string.format("%2d:%02d:%02d.%02d", hours, minutes, seconds, cs)
    else
        if (lap > 1) then
            local last_ticks = laps[lap] - laps[lap-1]
            local last_hours = last_ticks / (rb.HZ * 3600)
            last_ticks = last_ticks - (rb.HZ * last_hours * 3600)
            local last_minutes = last_ticks / (rb.HZ * 60)
            last_ticks = last_ticks - (rb.HZ * last_minutes * 60)
            local last_seconds = last_ticks / rb.HZ
            last_ticks = last_ticks - (rb.HZ * last_seconds)
            local last_cs = last_ticks

            return string.format("%2d %2d:%02d:%02d.%02d [%2d:%02d:%02d.%02d]",
                                 lap, hours, minutes, seconds, cs, last_hours,
                                 last_minutes, last_seconds, last_cs)
        else
            return string.format("%2d %2d:%02d:%02d.%02d", lap, hours, minutes, seconds, cs)
        end
    end
end

-- Helper function
function toboolean(v)
    return v == "true"
end

function arrangeButtons(btns)
    local totalWidth, totalHeight, maxWidth, maxHeight, vp = 0, 0, 0, 0
    for _, btn in pairs(btns) do
        totalWidth  = totalWidth  + btn.width
        totalHeight = totalHeight + btn.height
        maxHeight = math.max(maxHeight, btn.height)
        maxWidth  = math.max(maxWidth, btn.width)
    end

    if totalWidth <= rb.LCD_WIDTH then
        local temp = 0
        for _, btn in pairs(btns) do
            btn.y = rb.LCD_HEIGHT - maxHeight
            btn.x = temp

            temp = temp + btn.width
        end

        vp = {
               x = 0,
               y = 0,
               width = rb.LCD_WIDTH,
               height = rb.LCD_HEIGHT - maxHeight
             }
    elseif totalHeight <= rb.LCD_HEIGHT then
        local temp = 0
        for _, btn in pairs(btns) do
            btn.x = rb.LCD_WIDTH - maxWidth
            btn.y = temp

            temp = temp + btn.height
        end

        vp = {
              x = 0,
              y = 0,
              width = rb.LCD_WIDTH - maxWidth,
              height = rb.LCD_HEIGHT
             }
    else
        error("Can't arrange the buttons according to your screen's resolution!")
    end

    for k, v in pairs(vp) do
        LapsView.vp[k] = v
    end
end

rb.touchscreen_set_mode(rb.TOUCHSCREEN_POINT)

LapsView:init()

local btns = {
                Button:new({name = "startTimer", label = "Start"}),
                Button:new({name = "stopTimer", label = "Stop"}),
                Button:new({name = "newLap", label = "New Lap"}),
                Button:new({name = "resetTimer", label = "Reset"}),
                Button:new({name = "exitApp", label = "Quit"})
             }

arrangeButtons(btns)

for _, btn in pairs(btns) do
    btn:draw()
end

repeat
    LapsView:incTimer()

    local action = rb.get_action(rb.contexts.CONTEXT_STD, 0)

    if (action == rb.actions.ACTION_TOUCHSCREEN) then
        local btn, x, y = rb.action_get_touchscreen_press()

        if LapsView:checkForScroll(btn, x, y) then
            -- Don't do anything
        elseif btn == rb.buttons.BUTTON_REL then
            for _, btn in pairs(btns) do
                local name = btn.name
                if (btn:isPressed(x, y)) then
                    if name == "exitApp" then
                        action = rb.actions.ACTION_STD_CANCEL
                    else
                        LapsView[name](LapsView)
                    end
                end
            end
        end
    end

    LapsView:display()
    rb.lcd_update()
    rb.sleep(rb.HZ/50)
until action == rb.actions.ACTION_STD_CANCEL

LapsView:saveState()

