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
 Copyright (C) 2018 William Wilgus -- Added circles, logic rewrite, hard levels

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 KIND, either express or implied.

]]--
require "actions"
--[[only save the actions we are using]]
pla = {}
for key, val in pairs(rb.actions) do
    for _, v in ipairs({"PLA_", "TOUCHSCREEN", "_NONE"}) do
        if string.find (key, v) then
            pla[key] = val; break
        end
    end
end
rb.actions, rb.contexts = nil, nil
--------------------------------------

local _LCD = rb.lcd_framebuffer()
local rocklib_image = getmetatable(_LCD)
local _ellipse = rocklib_image.ellipse
local BSAND = 0x8
local irand = math.random

local HAS_TOUCHSCREEN = rb.action_get_touchscreen_press ~= nil
local LCD_H, LCD_W = rb.LCD_HEIGHT, rb.LCD_WIDTH
local DEFAULT_BALL_SZ = LCD_H > LCD_W and LCD_W  / 30 or LCD_H / 30
      DEFAULT_BALL_SZ = DEFAULT_BALL_SZ - bit.band(DEFAULT_BALL_SZ, 1)
local MAX_BALL_SPEED = DEFAULT_BALL_SZ / 2 + 1
-- Ball states
local B_DEAD, B_MOVE, B_EXPLODE, B_DIE, B_WAIT, B_IMPLODE = 5, 4, 3, 2, 1, 0

local DEFAULT_FG_CLR = 1
local DEFAULT_BG_CLR = 0
if rb.lcd_get_foreground ~= nil then
    DEFAULT_FG_CLR = rb.lcd_get_foreground()
    DEFAULT_BG_CLR = rb.lcd_get_background()
elseif rb.LCD_DEFAULT_FG ~= nil then
    DEFAULT_FG_CLR = rb.LCD_DEFAULT_FG
    DEFAULT_BG_CLR = rb.LCD_DEFAULT_BG
end

local FMT_EXPEND, FMT_LEVEL = "%d balls expended", "Level %d"
local FMT_TOTPTS, FMT_LVPTS = "%d total points", "%d level points"

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
                {58, 60},
                {28, 30},
                {23, 25},
                {18, 20},
                {13, 15},
                {8,  10},
                {9,  10},
                {4,   5},
                {5,   5}
            }

local function getstringsize(str)
        local _, w, h = rb.font_getstringsize(str, rb.FONT_UI)
        return w, h
end

local function set_foreground(color)
    if rb.lcd_set_foreground ~= nil then
        rb.lcd_set_foreground(color)
    end
end

local function random_color()
    if rb.lcd_rgbpack ~= nil then -- color target
        return rb.lcd_rgbpack(irand(1,255), irand(1,255), irand(1,255))
    end

    local color = irand(1, rb.LCD_DEPTH)
    color = (rb.LCD_DEPTH == 2) and (3 - color) or color -- invert for 2-bit screens

    return color
end

local Ball = {
                -- Ball defaults
                sz = DEFAULT_BALL_SZ,
                next_tick = 0,
                state = B_MOVE
             }

function Ball:new(o, level)
    if o == nil then
        level = level or 1
        local maxdelay = (level <= 5) and 10 or level
        o = {
                x = irand(1, LCD_W - self.sz),
                y = irand(1, LCD_H - self.sz),
                color = random_color(),
                xi = Ball:genSpeed(),
                yi = Ball:genSpeed(),
                step_delay    = irand(3, maxdelay / 2),
                explosion_sz  = irand(2 * self.sz, 4 * self.sz),
                life_ticks = irand(rb.HZ / level, rb.HZ * (maxdelay / 5))
            }
    end
    o.life_ticks = o.life_ticks + DEFAULT_BALL_SZ -- extra time for larger screens
    setmetatable(o, self)
    self.__index = self
    return o
end

function Ball:genSpeed()
    local speed = irand(-MAX_BALL_SPEED, MAX_BALL_SPEED)
    return speed ~= 0 and speed or MAX_BALL_SPEED -- Make sure all balls move
end

function Ball:draw()
    _ellipse(_LCD, self.x, self.y,
             self.x + self.sz, self.y + self.sz , self.color, self.color, true)
end

function Ball:step(tick)
    self.next_tick = tick + self.step_delay
    self.x = self.x + self.xi
    self.y = self.y + self.yi

    if (self.x <= 0 or self.x >= (LCD_W - self.sz)) then
        self.xi = -self.xi
        self.x  = self.x + self.xi
    end

    if (self.y <= 0 or self.y >= (LCD_H - self.sz)) then
        self.yi = -self.yi
        self.y  = self.y + self.yi
    end
end

function Ball:checkHit(other)
    if (other.xi >= self.x) and (other.yi >= self.y) and
        (self.x + self.sz >= other.x) and (self.y + self.sz >= other.y) then
        self.state = B_EXPLODE
        -- x/y increment no longer needed it is now impact region
        self.xi = self.x + self.sz
        self.yi = self.y + self.sz

        if other.state < B_EXPLODE then -- add duration to the ball that got hit
            other.next_tick = other.next_tick + self.life_ticks
        end
        return true
    end

    return false
end

function Ball:draw_exploded()
    _ellipse(_LCD, self.x, self.y, self.xi, self.yi, self.color, nil, true)
end

function Ball:step_exploded(tick)
    -- exploding ball state machine
    -- B_EXPLODE >> B_DIE >> BWAIT >> B_IMPLODE >> B_DEAD
    if self.state == B_EXPLODE and self.sz < self.explosion_sz then
        self.sz = self.sz + 2
        self.x  = self.x - 1 -- We do this because we want to stay centered
        self.y  = self.y - 1
    elseif self.state == B_DIE then
        self.state = B_WAIT
        self.next_tick = tick + self.life_ticks
        return
    elseif self.state == B_IMPLODE and self.sz > 0 then
        self.sz = self.sz - 2
        self.x  = self.x + 1 -- We do this because we want to stay centered
        self.y  = self.y + 1
    elseif self.state <= B_IMPLODE then
        self.state = B_DEAD
        return
    elseif self.next_tick < tick then
        self.state = self.state - 1
        return
    end
    -- fell through, update next_tick and impact region
    self.next_tick = tick + self.step_delay
    self.xi = self.x + self.sz
    self.yi = self.y + self.sz
end

local Cursor = {
                sz = (DEFAULT_BALL_SZ * 2),
                x  = (LCD_W / 2),
                y  = (LCD_H / 2),
                color = DEFAULT_FG_CLR
             }

function Cursor:new()
    if rb.LCD_DEPTH == 2 then -- invert for 2 - bit screens
        self.color = 3 - DEFAULT_FG_CLR
    end
    self:create_image(DEFAULT_BALL_SZ * 2)
    return self
end

function Cursor:create_image(sz)
    if not HAS_TOUCHSCREEN then
        sz = sz + 1
        local img = rb.new_image(sz, sz)
        local sz2 = (sz / 2) + 1
        local sz4 = (sz / 4)

        img:clear(0)
        img:line(1, 1, sz4 + 1, 1, 1)
        img:line(1, 1, 1, sz4 + 1, 1)

        img:line(1, sz, sz4 + 1, sz, 1)
        img:line(1, sz, 1, sz - sz4, 1)

        img:line(sz, sz, sz - sz4, sz, 1)
        img:line(sz, sz, sz, sz - sz4, 1)

        img:line(sz, 1, sz - sz4, 1, 1)
        img:line(sz, 1, sz, sz4 + 1, 1)

        -- crosshairs
        img:line(sz2 - sz4, sz2, sz2 + sz4, sz2, 1)
        img:line(sz2, sz2 - sz4, sz2, sz2 + sz4, 1)
        self.image = img
    end
end

local function clamp_roll(iVal, iMin, iMax)
    if iVal < iMin then
        iVal = iMax
    elseif iVal > iMax then
        iVal = iMin
    end

    return iVal
end

function Cursor:do_action(action)
    local xi, yi = 0, 0

    if HAS_TOUCHSCREEN and action == pla.ACTION_TOUCHSCREEN then
        _, self.x, self.y = rb.action_get_touchscreen_press()
        return true
    elseif action == pla.PLA_SELECT then
        return true
    elseif (action == pla.PLA_RIGHT or action == pla.PLA_RIGHT_REPEAT) then
        xi = self.sz
    elseif (action == pla.PLA_LEFT or action == pla.PLA_LEFT_REPEAT) then
        xi = -self.sz
    elseif (action == pla.PLA_UP or action == pla.PLA_UP_REPEAT) then
        yi = -self.sz
    elseif (action == pla.PLA_DOWN or action == pla.PLA_DOWN_REPEAT) then
        yi = self.sz
    end

    self.x = clamp_roll(self.x + xi, 1, LCD_W - self.sz)
    self.y = clamp_roll(self.y + yi, 1, LCD_H - self.sz)

    return false
end

function Cursor:draw()
    rocklib_image.copy(_LCD, self.image, self.x, self.y, _NIL, _NIL,
                       _NIL, _NIL, true, BSAND, self.color)
end

local function calc_score(total, level, goal, expended)
    local score = (expended * level) * 100
    if expended < goal then
      score = -(score + (level * 100))
    end
    total = total + score
    return total
end

local function draw_pos_str(bottom, right, str)
    local w, h = getstringsize(str)
    local x = (right > 0) and ((LCD_W - w) * right - 1) or 1
    local y = (bottom > 0) and ((LCD_H - h) * bottom - 1) or 1
    rb.lcd_putsxy(x, y, str)
end

local function wait_anykey(to_secs)
    rb.sleep(rb.HZ / 2)
    rb.button_clear_queue()
    rb.button_get_w_tmo(rb.HZ * to_secs)
end

local function start_round(level, goal, nrBalls, total)
    local player_added, score = false, 0
    local last_expend, nrBalls_expend = 0, 0
    local balls_exploded = 1 -- keep looping when player_added == false
    local action = 0
    local Balls = {}
    local str_level = string.format(FMT_LEVEL, level) -- static
    local str_totpts = string.format(FMT_TOTPTS, total) -- static
    local str_expend, str_lvlpts
    local tick, cursor
    local test_spd = false

    local function update_stats()
        -- we only create a new string when a hit is detected
        str_expend = string.format(FMT_EXPEND, nrBalls_expend)
        str_lvlpts = string.format(FMT_LVPTS, score)
    end

    local function draw_stats()
        draw_pos_str(0, 0, str_expend)
        draw_pos_str(0, 1, str_level)
        draw_pos_str(1, 1, str_lvlpts)
        draw_pos_str(1, 0, str_totpts)
    end

    local function add_player()
        -- cursor becomes exploded ball
        local player = Ball:new({
                            x = cursor.x,
                            y = cursor.y,
                            color = cursor.color,
                            step_delay = 3,
                            explosion_sz = (3 * DEFAULT_BALL_SZ),
                            life_ticks = (test_time == true) and (100) or 
                                        irand(rb.HZ * 2, rb.HZ * DEFAULT_BALL_SZ),
                            sz = 10,
                            state = B_EXPLODE
                        })
        -- set x/y impact region
        player.xi = player.x + player.sz
        player.yi = player.y + player.sz

        table.insert(Balls, player)
        balls_exploded = 1
        player_added = true
        cursor = nil
    end

    if level < 1 then
        -- check speed of target
        set_foreground(DEFAULT_BG_CLR) --hide text during test
        local bkcolor = (rb.LCD_DEPTH == 2) and (3) or 0
        level = 1
        nrBalls = 20
        cursor = { x = LCD_W * 2, y = LCD_H * 2, color = bkcolor}
        table.insert(Balls, Ball:new({
                            x = 1, y = 1, xi = 1, yi = 1,
                            color = bkcolor, step_delay = 1,
                            explosion_sz = 0, life_ticks = 0,
                            step = function() test_spd = test_spd + 1 end
                        })
                    )
        add_player()
        test_spd = 0
    else
        set_foreground(DEFAULT_FG_CLR) -- color for text
        cursor = Cursor:new()
    end

    -- Initialize the balls
    for i=1, nrBalls do
        table.insert(Balls, Ball:new(nil, level))
    end

    -- Make sure there are no unwanted touchscreen presses
    rb.button_clear_queue()

    update_stats() -- load status strings

     -- Check if the round is over
    while balls_exploded > 0 do
        tick = rb.current_tick()

        if action ~= pla.ACTION_NONE and (action == pla.PLA_EXIT or
                                          action == pla.PLA_CANCEL) then
            action = pla.PLA_EXIT
            break
        end

        rb.lcd_clear_display()

        if not player_added then
            if action ~= pla.ACTION_NONE and cursor:do_action(action) then
                add_player()
            elseif not HAS_TOUCHSCREEN then
                cursor:draw()
            end
        end

        for _, Ball in ipairs(Balls) do
            if Ball.state == B_MOVE then
                if tick > Ball.next_tick then
                    Ball:step(tick)
                    for i = #Balls, 1, -1 do
                        if Balls[i].state < B_MOVE and
                            Ball:checkHit(Balls[i]) then -- exploded?
                                balls_exploded = balls_exploded + 1
                                nrBalls_expend = nrBalls_expend + 1
                                break
                        end
                    end
                end
                -- check if state changed draw ball if still moving
                if Ball.state == B_MOVE then
                    Ball:draw()
                end
            elseif Ball.state ~= B_DEAD then
                if tick > Ball.next_tick then
                    Ball:step_exploded(tick)
                end
                if Ball.state ~= B_DEAD then
                    Ball:draw_exploded()
                else
                    balls_exploded = balls_exploded - 1
                end
            end
        end

        draw_stats() -- stats redrawn every frame
        rb.lcd_update() -- Push framebuffer to the LCD

        if nrBalls_expend ~= last_expend then -- hit detected?
            last_expend = nrBalls_expend
            score = (nrBalls_expend * level) * 100
            update_stats() -- only update stats when not current
            if nrBalls_expend == nrBalls then break end -- round is over?
        end

        rb.yield() -- yield to other tasks

        action = rb.get_plugin_action(0) -- Check for actions
    end

    if test_spd and test_spd > 0 then return test_spd end

    -- splash the final stats for a moment at end
    rb.lcd_clear_display()
    for _, Ball in ipairs(Balls) do
        -- move balls back to their initial exploded positions
        if Ball.state == B_DEAD then
            Ball.sz = Ball.explosion_sz + 2
            Ball.x  = Ball.x - (Ball.explosion_sz / 2)
            Ball.y  = Ball.y - (Ball.explosion_sz / 2)
        end
        Ball:draw()
    end

    if DEFAULT_BALL_SZ > 3 then
        _LCD:clear(nil, nil, nil, nil, nil, nil, 2, 2)
    end

    total = calc_score(total, level, goal, nrBalls_expend)
    str_totpts = string.format(FMT_TOTPTS, total)
    draw_stats()
    rb.lcd_update()
    wait_anykey(2)

    return action == pla.PLA_EXIT, score, nrBalls_expend
end

-- Helper function to display a message
local function disp_msg(to, ...)
    local message = string.format(...)
    local w, h = getstringsize(message)
    local x, y = (LCD_W - w) / 2, (LCD_H - h) / 2

    rb.lcd_clear_display()
    set_foreground(DEFAULT_FG_CLR)

    if w > LCD_W then
        rb.lcd_puts_scroll(0, (y / h), message)
    else
        rb.lcd_putsxy(x, y, message)
    end

    if to == -1 then
        local msg = "Press button to exit"
        w, h = getstringsize(msg)
        x = (LCD_W - w) / 2
        if x < 0 then
            rb.lcd_puts_scroll(0, (y / h) + 1, msg)
        else
            rb.lcd_putsxy(x, y + h, msg)
        end
    end
    rb.lcd_update()

    if to == -1 then
        wait_anykey(60)
    else
        rb.sleep(to)
    end

    rb.lcd_scroll_stop() -- Stop our scrolling message
end

--[[MAIN PROGRAM]]
do  -- attempt to get stats to fit on screen
    local function getwidth(str)
        local w, _ = getstringsize(str)
        return w
    end
    local w0, w1 = getwidth(FMT_EXPEND), getwidth(FMT_LEVEL)
    if (w0 + w1) > LCD_W then
        FMT_EXPEND, FMT_LEVEL = "%d balls", "Lv %d"
    end
    w0, w1 = getwidth(FMT_TOTPTS), getwidth(FMT_LVPTS) 
    if (w0 + w1 + getwidth("0000000")) > LCD_W then
        FMT_TOTPTS, FMT_LVPTS = "%d total", "%d lv"
    end
end

if HAS_TOUCHSCREEN then
    rb.touchscreen_mode(rb.TOUCHSCREEN_POINT)
end

rb.backlight_force_on()

math.randomseed(os.time())

local idx, highscore = 1, 0

local test_spd = start_round(0, 0, 0, 0) -- test speed of target

if test_spd < 100 then 
    rb.splash(100, "Slow Target..")

    if test_spd < 25 then
        MAX_BALL_SPEED = MAX_BALL_SPEED + MAX_BALL_SPEED
    elseif  test_spd < 50 then
        MAX_BALL_SPEED = MAX_BALL_SPEED + MAX_BALL_SPEED / 2
    elseif  test_spd < 100 then
        MAX_BALL_SPEED = MAX_BALL_SPEED + MAX_BALL_SPEED / 4
    end
end

while levels[idx] ~= nil do
    local goal, nrBalls = levels[idx][1], levels[idx][2]

    collectgarbage("collect") -- run gc now to hopefully prevent interruption later

    disp_msg(rb.HZ * 2, "Level %d: get %d out of %d balls", idx, goal, nrBalls)

    local exit, score, nrBalls_expend = start_round(idx, goal, nrBalls, highscore)

    if exit then
        break -- Exiting..
    else
        highscore = calc_score(highscore, idx, goal, nrBalls_expend)
        if nrBalls_expend >= goal then
            disp_msg(rb.HZ * 2, "You won!")
            levels[idx] = nil
            idx = idx + 1
        else
            disp_msg(rb.HZ * 2, "You lost %d points!", score + (idx * 100))
            if highscore < 0 then break end
        end
    end
end

if highscore <= 0 then
    disp_msg(-1, "You lost at level %d", idx)
elseif idx > #levels then
    disp_msg(-1, "You finished the game with %d points!", highscore)
else
    disp_msg(-1, "You made it till level %d with %d points!", idx, highscore)
end

-- Restore user backlight settings
rb.backlight_use_settings()
