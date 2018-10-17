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
 Copyright (C) 2018 William Wilgus -- Added circles, blit cursor, hard levels

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 KIND, either express or implied.

]]--
require "actions"
-- [[only save the actions we are using]]
actions_pla = {}
for key, val in pairs(rb.actions) do
    for _, v in ipairs({"PLA_", "TOUCHSCREEN"}) do
        if string.find (key, v) then
            actions_pla[key] = val
            break
        end
    end
end
rb.actions = nil
rb.contexts = nil
-------------------------------------

local _LCD = rb.lcd_framebuffer()
local BSAND = 0x8
local rocklib_image = getmetatable(rb.lcd_framebuffer())
local _ellipse = rocklib_image.ellipse

local CYCLETIME = rb.HZ / 50
local HAS_TOUCHSCREEN = rb.action_get_touchscreen_press ~= nil
local DEFAULT_BALL_SIZE = rb.LCD_HEIGHT > rb.LCD_WIDTH and rb.LCD_WIDTH  / 30
                                                       or  rb.LCD_HEIGHT / 30
local MAX_BALL_SPEED = DEFAULT_BALL_SIZE / 2
local DEC_BALL_SPEED = DEFAULT_BALL_SIZE / 8
local DEFAULT_FOREGROUND_COLOR = rb.lcd_get_foreground ~= nil
                                                     and rb.lcd_get_foreground()
                                                     or  0

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
                {59, 60},
                {29, 30},
                {24, 25},
                {19, 20},
                {14, 15},
                {9,  10},
                {10, 10},
                {4,   5},
                {5,   5}
           }

local Ball = {
                size = DEFAULT_BALL_SIZE,
                exploded = false,
                implosion = false
             }

local function create_cursor(size)
    local cursor
    if not HAS_TOUCHSCREEN then
        cursor = rb.new_image(size, size)
        cursor:clear(0)
        local sz2 = size / 2
        local sz4 = size / 4

        cursor:line(1, 1, sz4, 1, 1)
        cursor:line(1, 1, 1, sz4, 1)

        cursor:line(1, size, sz4, size, 1)
        cursor:line(1, size, 1, size - sz4, 1)

        cursor:line(size, size, size - sz4, size, 1)
        cursor:line(size, size, size, size - sz4, 1)

        cursor:line(size, 1, size - sz4, 1, 1)
        cursor:line(size, 1, size, sz4, 1)

        --crosshairs
        cursor:line(sz2 - sz4, sz2, sz2 + sz4, sz2, 1)
        cursor:line(sz2, sz2 - sz4, sz2, sz2 + sz4, 1)
    end
    return cursor
end

function Ball:new(o, level)
    level = level or 1
    if o == nil then
        o = {
                x = math.random(0, rb.LCD_WIDTH - self.size),
                y = math.random(0, rb.LCD_HEIGHT - self.size),
                color = random_color(),
                up_speed = Ball:generateSpeed(level),
                right_speed = Ball:generateSpeed(level),
                explosion_size = math.random(2*self.size, 4*self.size),
                life_duration = math.random(rb.HZ / level, rb.HZ*5),
            }
    end

    setmetatable(o, self)
    self.__index = self
    return o
end

function Ball:generateSpeed(level)
    local ballspeed = MAX_BALL_SPEED - (DEC_BALL_SPEED * (level - 1))
    if ballspeed < 2 then ballspeed = 2 end
    local speed = math.random(-ballspeed, ballspeed)
    if speed == 0 then
        speed = 1       -- Make sure all balls move
    end

    return speed
end

function Ball:draw_exploded()
    --[[
    --set_foreground(self.color)
    --rb.lcd_fillrect(self.x, self.y, self.size, self.size)
    ]]

    _ellipse(_LCD, self.x, self.y,
             self.x + self.size, self.y + self.size , self.color, nil, true)
end

function Ball:draw()
    --[[
    --set_foreground(self.color)
    --rb.lcd_fillrect(self.x, self.y, self.size, self.size)
    ]]

    _ellipse(_LCD, self.x, self.y,
             self.x + self.size, self.y + self.size , self.color, self.color, true)
end

function Ball:step_exploded()
    if self.implosion and self.size > 0 then
        self.size = self.size - 2
        self.x = self.x + 1 -- We do this because we want to stay centered
        self.y = self.y + 1
    elseif self.size < self.explosion_size then
        self.size = self.size + 2
        self.x = self.x - 1 -- We do this for the same reasons as above
        self.y = self.y - 1
    end
end

function Ball:step()
    self.x = self.x + self.right_speed
    self.y = self.y + self.up_speed
    if (self.x <= 0 or self.x >= rb.LCD_WIDTH - self.size) then
        self.right_speed = -self.right_speed
        self.x = self.x + self.right_speed
    end
    if (self.y <= 0 or self.y >= rb.LCD_HEIGHT - self.size ) then
        self.up_speed = -self.up_speed
        self.y = self.y + self.up_speed
    end
end

function Ball:checkHit(other)
    if (other.x + other.size >= self.x) and (self.x + self.size >= other.x) and
       (other.y + other.size >= self.y) and (self.y + self.size >= other.y) then
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

local Cursor = {
                size = DEFAULT_BALL_SIZE*2,
                x = rb.LCD_WIDTH/2,
                y = rb.LCD_HEIGHT/2,
                image = create_cursor(DEFAULT_BALL_SIZE*2)
             }

function Cursor:new()
    return self
end

function Cursor:do_action(action)
    if action == actions_pla.ACTION_TOUCHSCREEN and HAS_TOUCHSCREEN then
        _, self.x, self.y = rb.action_get_touchscreen_press()
        return true
    elseif action == actions_pla.PLA_SELECT then
        return true
    elseif (action == actions_pla.PLA_RIGHT or action == actions_pla.PLA_RIGHT_REPEAT) then
        self.x = self.x + self.size
    elseif (action == actions_pla.PLA_LEFT or action == actions_pla.PLA_LEFT_REPEAT) then
        self.x = self.x - self.size
    elseif (action == actions_pla.PLA_UP or action == actions_pla.PLA_UP_REPEAT) then
        self.y = self.y - self.size
    elseif (action == actions_pla.PLA_DOWN or action == actions_pla.PLA_DOWN_REPEAT) then
        self.y = self.y + self.size
    end

    if self.x > rb.LCD_WIDTH then
        self.x = 0
    elseif self.x < 0 then
        self.x = rb.LCD_WIDTH
    end

    if self.y > rb.LCD_HEIGHT then
        self.y = 0
    elseif self.y < 0 then
        self.y = rb.LCD_HEIGHT
    end

    return false
end

function Cursor:draw()

    rocklib_image.copy(_LCD, self.image, self.x - self.size/2, self.y - self.size/2,
                       _NIL, _NIL, _NIL, _NIL, true, BSAND, DEFAULT_FOREGROUND_COLOR)
end

function draw_positioned_string(bottom, right, str)
    local _, w, h = rb.font_getstringsize(str, rb.FONT_UI)
    local x = not right or (rb.LCD_WIDTH-w)*right - 1
    local y = not bottom or (rb.LCD_HEIGHT-h)*bottom - 1
    rb.lcd_putsxy(x, y, str)
end

function set_foreground(color)
    if rb.lcd_set_foreground ~= nil then
        rb.lcd_set_foreground(color)
    end
end

function random_color()
    if rb.lcd_rgbpack ~= nil then --color target
        return rb.lcd_rgbpack(math.random(1,255), math.random(1,255), math.random(1,255))
    end

    return math.random(1, rb.LCD_DEPTH)
end

function start_round(level, goal, nrBalls, total)
    local player_added, score, exit, nrExpendedBalls = false, 0, false, 0
    local Balls, explodedBalls = {}, {}
    local ball_ct, ball_el
    local tick, endtick
    local action = 0
    local hit_detected = false
    local cursor = Cursor:new()

    -- Initialize the balls
    for _=1,nrBalls do
        table.insert(Balls, Ball:new(nil, level))
    end

    local function draw_stats()
        draw_positioned_string(0, 0, string.format("%d balls expended", nrExpendedBalls))
        draw_positioned_string(0, 1, string.format("Level %d", level))
        draw_positioned_string(1, 1, string.format("%d level points", score))
        draw_positioned_string(1, 0, string.format("%d total points", total + score))
    end

    -- Make sure there are no unwanted touchscreen presses
    rb.button_clear_queue()

    set_foreground(DEFAULT_FOREGROUND_COLOR) -- color for text

    while true do
        endtick = rb.current_tick() + CYCLETIME

        -- Check if the round is over
        if player_added and #explodedBalls == 0 then
            break
        end

        if action == actions_pla.PLA_EXIT or action == actions_pla.PLA_CANCEL then
            exit = true
            break
        end

        if not player_added and cursor:do_action(action) then
            local player = Ball:new({
                                x = cursor.x,
                                y = cursor.y,
                                color = DEFAULT_FOREGROUND_COLOR,
                                size = 10,
                                explosion_size = 3*DEFAULT_BALL_SIZE,
                                exploded = true,
                                death_time = rb.current_tick() + rb.HZ * 3
                            })
            explodedBalls[1] = player
            player_added = true
            cursor = nil
        end

       -- Check for hits
        for i, Ball in ipairs(Balls) do
            for _, explodedBall in ipairs(explodedBalls) do
                if Ball:checkHit(explodedBall) then
                    explodedBalls[#explodedBalls + 1] = Ball
                    --table.remove(Balls, i)
                    Balls[i] = false
                    hit_detected = true
                    break
                end
            end
        end

        -- Check if we're dead yet
        tick = rb.current_tick()
        for i, explodedBall in ipairs(explodedBalls) do
            if explodedBall.death_time < tick then
                if explodedBall.size > 0 then
                    explodedBall.implosion = true -- We should be dying
                else
                    table.remove(explodedBalls, i) -- We're imploded!
                end
            end
        end

        -- Drawing phase
        if hit_detected then
            hit_detected = false
            -- same as table.remove(Balls, i) but more efficient
            ball_el = 1
            ball_ct = #Balls
            for i = 1, ball_ct do
                if Balls[i] then
                    Balls[ball_el] = Balls[i]
                    ball_el = ball_el + 1
                end
            end
            -- remove any remaining exploded balls
            for i = ball_el, ball_ct do
                Balls[i] = nil
            end
            -- Calculate score
            nrExpendedBalls = nrBalls - ball_el + 1
            score = nrExpendedBalls * level * 100
        end

        rb.lcd_clear_display()
        draw_stats()

        if not (player_added or HAS_TOUCHSCREEN) then
            cursor:draw()
        end

        for _, Ball in ipairs(Balls) do
            Ball:step()
            Ball:draw()
        end

        for _, explodedBall in ipairs(explodedBalls) do
            explodedBall:step_exploded()
            explodedBall:draw_exploded()
        end

        -- Push framebuffer to the LCD
        rb.lcd_update()

        -- Check for actions
        if rb.current_tick() < endtick then
            action = rb.get_plugin_action(endtick - rb.current_tick())
        else
            rb.yield()
            action = rb.get_plugin_action(0)
        end

    end

    --splash the final stats for a moment at end
    rb.lcd_clear_display()
    for _, Ball in ipairs(Balls) do Ball:draw() end
    _LCD:clear(nil, nil, nil, nil, nil, nil, 2, 2)
    draw_stats()
    rb.lcd_update()
    rb.sleep(rb.HZ * 2)

    return exit, score, nrExpendedBalls
end

-- Helper function to display a message
function display_message(to, ...)
    local message = string.format(...)
    local _, w, h = rb.font_getstringsize(message, rb.FONT_UI)
    local x, y = (rb.LCD_WIDTH - w) / 2, (rb.LCD_HEIGHT - h) / 2

    rb.lcd_clear_display()
    set_foreground(DEFAULT_FOREGROUND_COLOR)

    if w > rb.LCD_WIDTH then
        rb.lcd_puts_scroll(0, y/h, message)
    else
        rb.lcd_putsxy(x, y, message)
    end
    if to == -1 then
        local msg = "Press button to exit"
        w = rb.font_getstringsize(msg, rb.FONT_UI)
        x = (rb.LCD_WIDTH - w) / 2
        if x < 0 then
            rb.lcd_puts_scroll(0, y/h + 1, msg)
        else
            rb.lcd_putsxy(x, y + h, msg)
        end
    end
    rb.lcd_update()

    if to == -1 then
        rb.sleep(rb.HZ/2)
        rb.button_clear_queue()
        rb.button_get(rb.HZ * 60)
    else
        rb.sleep(to)
    end

    rb.lcd_scroll_stop() -- Stop our scrolling message
end

if HAS_TOUCHSCREEN then
    rb.touchscreen_set_mode(rb.TOUCHSCREEN_POINT)
end

--[[MAIN PROGRAM]]
rb.backlight_force_on()

math.randomseed(os.time())

local idx, highscore = 1, 0
while levels[idx] ~= nil do
    local goal, nrBalls = levels[idx][1], levels[idx][2]

    collectgarbage("collect") --run gc now to hopefully prevent interruption later

    display_message(rb.HZ*2, "Level %d: get %d out of %d balls", idx, goal, nrBalls)

    local exit, score, nrExpendedBalls = start_round(idx, goal, nrBalls, highscore)

    if exit then
        break -- Exiting..
    else
        if nrExpendedBalls >= goal then
            display_message(rb.HZ*2, "You won!")
            idx = idx + 1
            highscore = highscore + score
        else
            display_message(rb.HZ*2, "You lost!")
            highscore = highscore - score - idx * 100
            if highscore < 0 then break end
        end
    end
end
if highscore <= 0 then
    display_message(-1, "You lost at level %d", idx)
elseif idx > #levels then
    display_message(-1, "You finished the game with %d points!", highscore)
else
    display_message(-1, "You made it till level %d with %d points!", idx, highscore)
end

-- Restore user backlight settings
rb.backlight_use_settings()

