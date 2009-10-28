--[[
             __________               __   ___.
   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
                     \/            \/     \/    \/            \/
 $Id$

 Port of Chain Reaction (which is based on Boomshine) to Rockbox in Lua.
 See http://www.yvoschaap.com/chainrxn/ and http://www.k2xl.com/games/boomshine/

 Copyright (C) 2009 by Maurus Cuelenaere

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 KIND, either express or implied.

]]--

require "actions"

local CYCLETIME = rb.HZ / 50

local levels = {
            --  {GOAL, TOTAL_BALLS},
                {1,   5},
                {2,  10},
                {4,  15},
                {6,  20},
                {10, 25},
                {15, 30},
                {18, 35},
                {22, 40},
                {30, 45},
                {37, 50},
                {48, 55},
                {55, 60}
           }

local Ball = {
                size = 10,
                exploded = false,
                implosion = false
             }

function Ball:new(o)
    if o == nil then
        o = {
                x = math.random(self.size, rb.LCD_WIDTH - self.size),
                y = math.random(self.size, rb.LCD_HEIGHT - self.size),
                color = rb.lcd_rgbpack(math.random(0,255), math.random(0,255), math.random(0,255)),
                up_speed = math.random(-3, 3),
                right_speed = math.random(-3, 3),
                explosion_size = math.random((3*self.size)/2, (5*self.size)/2),
                life_duration = math.random(rb.HZ, rb.HZ*5)
            }

        -- Make sure all balls move
        if o.up_speed == 0 then o.up_speed = 1 end
        if o.right_speed == 0 then o.right_speed = 1 end
    end

    setmetatable(o, self)
    self.__index = self
    return o
end

function Ball:draw()
    --[[
         I know these aren't circles, but as there's no current circle
         implementation in Rockbox, rectangles will just do fine (drawing
         circles from within Lua is far too slow).
    ]]--
    rb.lcd_set_foreground(self.color)
    rb.lcd_fillrect(self.x, self.y, self.size, self.size)
end

function Ball:step()
    if self.exploded then
        if self.implosion and self.size > 0 then
            self.size = self.size - 2
            self.x = self.x + 1 -- We do this because we want to stay centered
            self.y = self.y + 1
        elseif self.size < self.explosion_size then
            self.size = self.size + 2
            self.x = self.x - 1 -- We do this for the same reasons as above
            self.y = self.y - 1
        end
        return
    end

    self.x = self.x + self.right_speed
    self.y = self.y + self.up_speed
    if (self.x + self.size) >= rb.LCD_WIDTH or self.x <= self.size then
        self.right_speed = self.right_speed * (-1)
    elseif (self.y + self.size) >= rb.LCD_HEIGHT or self.y <= self.size then
        self.up_speed = self.up_speed * (-1)
    end
end

function Ball:checkHit(other)
    local x_dist = math.abs(other.x - self.x)
    local y_dist = math.abs(other.y - self.y)
    local x_size = self.x > other.x and other.size or self.size
    local y_size = self.y > other.y and other.size or self.size

    if (x_dist <= x_size) and (y_dist <= y_size) then
        assert(not self.exploded)
        self.exploded = true
        self.death_time = rb.current_tick() + self.life_duration
        if not other.exploded then
            other.exploded = true
            other.death_time = rb.current_tick() + other.life_duration
        end
        return true
    end

    return false
end

function draw_positioned_string(bottom, right, str)
    local _, w, h = rb.font_getstringsize(str, rb.FONT_UI)
    rb.lcd_putsxy((rb.LCD_WIDTH-w)*right, (rb.LCD_HEIGHT-h)*bottom, str)
end

function start_round(level, goal, nrBalls, total)
    local player_added, score, exit, nrExpandedBalls = false, 0, false, 0
    local balls, explodedBalls = {}, {}

    -- Initialize the balls
    for _=1,nrBalls do
        table.insert(balls, Ball:new())
    end

    -- Make sure there are no unwanted touchscreen presses
    rb.button_clear_queue()

    while true do
        local endtick = rb.current_tick() + CYCLETIME

        -- Check if the round is over
        if #explodedBalls == 0 and player_added then
            break
        end

        -- Check for actions
        local action = rb.get_action(rb.contexts.CONTEXT_STD, 0)
        if (action == rb.actions.ACTION_TOUCHSCREEN) then
            local _, x, y = rb.action_get_touchscreen_press()
            if not player_added then
                local player = Ball:new({
                                    x = x,
                                    y = y,
                                    color = rb.lcd_rgbpack(255, 255, 255),
                                    size = 10,
                                    explosion_size = 30,
                                    exploded = true,
                                    death_time = rb.current_tick() + rb.HZ * 3
                                })
                table.insert(explodedBalls, player)
                player_added = true
            end
        elseif(action == rb.actions.ACTION_STD_CANCEL) then
            exit = true
            break
        end

        -- Check for hits
        for i, ball in ipairs(balls) do
            for _, explodedBall in ipairs(explodedBalls) do
                if ball:checkHit(explodedBall) then
                    score = score + 100*level
                    nrExpandedBalls = nrExpandedBalls + 1
                    table.insert(explodedBalls, ball)
                    table.remove(balls, i)
                    break
                end
            end
        end

        -- Check if we're dead yet
        for i, explodedBall in ipairs(explodedBalls) do
            if rb.current_tick() >= explodedBall.death_time then
                if explodedBall.size > 0 then
                    explodedBall.implosion = true -- We should be dying
                else
                    table.remove(explodedBalls, i) -- We're imploded!
                end
            end
        end

        -- Drawing phase
        rb.lcd_clear_display()
        rb.lcd_set_foreground(rb.lcd_rgbpack(255, 255, 255))
        draw_positioned_string(0, 0, string.format("%d balls expanded", nrExpandedBalls))
        draw_positioned_string(0, 1, string.format("Level %d", level))
        draw_positioned_string(1, 1, string.format("%d level points", score))
        draw_positioned_string(1, 0, string.format("%d total points", total+score))

        for _, ball in ipairs(balls) do
            ball:step()
            ball:draw()
        end

        for _, explodedBall in ipairs(explodedBalls) do
            explodedBall:step()
            explodedBall:draw()
        end

        rb.lcd_update()

        if rb.current_tick() < endtick then
            rb.sleep(endtick - rb.current_tick())
        else
            rb.yield()
        end
    end

    return exit, score
end

-- Helper function to display a message
function display_message(message)
    local _, w, h = rb.font_getstringsize(message, rb.FONT_UI)
    local x, y = (rb.LCD_WIDTH - w) / 2, (rb.LCD_HEIGHT - h) / 2

    rb.lcd_clear_display()
    rb.lcd_set_foreground(rb.lcd_rgbpack(255, 255, 255))
    if w > rb.LCD_WIDTH then
        rb.lcd_puts_scroll(x/w, y/h, message)
    else
        rb.lcd_putsxy(x, y, message)
    end
    rb.lcd_update()

    rb.sleep(rb.HZ * 2)

    rb.lcd_stop_scroll() -- Stop our scrolling message
end

rb.touchscreen_set_mode(rb.TOUCHSCREEN_POINT)
rb.backlight_force_on()

local idx, highscore = 1, 0
while true do
    local level = levels[idx]
    local goal, nrBalls = level[1], level[2]

    if level == nil then
        break -- No more levels to play
    end

    display_message(string.format("Level %d: get %d out of %d balls", idx, goal, nrBalls))

    local exit, score = start_round(idx, goal, nrBalls, highscore)
    if exit then
        break -- Exiting..
    else
        if score >= goal then
            display_message("You won!")
            idx = idx + 1
            highscore = highscore + score
        else
            display_message("You lost!")
        end
    end
end

display_message(string.format("You made it till level %d with %d points!", idx, highscore))

-- Restore user backlight settings
rb.backlight_use_settings()

