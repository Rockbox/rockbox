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
 Copyright (C) 2020 William Wilgus -- Added circles, logic rewrite, hard levels

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 KIND, either express or implied.

]]--
require "actions"
local DEBUG = false

--[[only save the actions we are using]]----------------------------------------
pla = {}
for key, val in pairs(rb.actions) do
    for _, v in ipairs({"PLA_", "TOUCHSCREEN", "_NONE"}) do
        if string.find (key, v) then
            pla[key] = val; break
        end
    end
end
rb.actions, rb.contexts = nil, nil
--------------------------------------------------------------------------------


--[[ Profiling ]]---------------------------------------------------------------
local screen_redraw_count = 0
local screen_redraw_duration = 0
--------------------------------------------------------------------------------


--[[ References ]]--------------------------------------------------------------
local _LCD = rb.lcd_framebuffer()
local rocklib_image = getmetatable(_LCD)
local _ellipse = rocklib_image.ellipse--
local irand = math.random--
local lcd_putsxy = rb.lcd_putsxy--
local strfmt = string.format--
local Cursor_Ref = nil
local Ball_Ref = {}
--------------------------------------------------------------------------------


--[[ 'Constants' ]]-------------------------------------------------------------
local SCORE_MULTIPLY = 10
local BSAND = 0x8--
local HAS_TOUCHSCREEN = rb.action_get_touchscreen_press ~= nil
local Empty_fn = function() end
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
--------------------------------------------------------------------------------


--[[ Other ]]-------------------------------------------------------------------
local Player_Added
local Action_Evt = pla.ACTION_NONE

local levels = {
            --  {GOAL, TOTAL_BALLS},
                {1,  5},
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
--------------------------------------------------------------------------------


--[[ Event Callback ]]----------------------------------------------------------
function action_event(action)
    if action == pla.PLA_CANCEL then
        Action_Evt = pla.PLA_EXIT
    else
        Action_Evt = action
    end
end
--------------------------------------------------------------------------------


--[[ Helper functions ]]--------------------------------------------------------
local function getstringsize(str)
        local _, w, h = rb.font_getstringsize(str, rb.FONT_UI)
        return w, h
end

local function set_foreground(color)
    if rb.lcd_set_foreground ~= nil then
        rb.lcd_set_foreground(color)
    end
end

local function calc_score(total, level, goal, expended)
    local bonus = 0
    local score = (expended * level) * SCORE_MULTIPLY
    if expended < goal then
      score = -(score + (level * SCORE_MULTIPLY))
    elseif expended > goal then
      bonus = (expended - goal) * (level * SCORE_MULTIPLY)
    end
    total = total + score + bonus
    return total, score, bonus
end

local function wait_anykey(to_secs)
    rb.sleep(rb.HZ / 2)
    rb.button_clear_queue()
    rb.button_get_w_tmo(rb.HZ * to_secs)
end

local function disp_msg(to, ...)
    local message = strfmt(...)
    local w, h = getstringsize(message)
    local x, y = (LCD_W - w) / 2, (LCD_H - h) / 2

    rb.lcd_clear_display()
    set_foreground(DEFAULT_FG_CLR)

    if w > LCD_W then
        rb.lcd_puts_scroll(0, (y / h), message)
    else
        lcd_putsxy(x, y, message)
    end

    if to == -1 then
        local msg = "Press button to exit"
        w, h = getstringsize(msg)
        x = (LCD_W - w) / 2
        if x < 0 then
            rb.lcd_puts_scroll(0, (y / h) + 1, msg)
        else
            lcd_putsxy(x, y + h, msg)
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

local function test_speed()
    local test_spd, redraw, dur = start_round(0, 0, 0, 0) -- test speed of target
    --Logic speed, screen redraw, duration
    if DEBUG == true then
        disp_msg(rb.HZ * 5, "Spd: %d, Redraw: %d Dur: %d Ms", test_spd, redraw, dur)
    end
    if test_spd < 25 or redraw < 10 then
        rb.splash(rb.HZ, "Slow Target..")

        if test_spd < 10 then
            MAX_BALL_SPEED = MAX_BALL_SPEED + MAX_BALL_SPEED
        elseif  test_spd < 15 then
            MAX_BALL_SPEED = MAX_BALL_SPEED + MAX_BALL_SPEED / 2
        elseif  test_spd < 25 then
            MAX_BALL_SPEED = MAX_BALL_SPEED + MAX_BALL_SPEED / 4
        end
    end
end
--------------------------------------------------------------------------------


--[[ Ball Functions ]]----------------------------------------------------------
local Ball = {
                -- Ball defaults
                sz = DEFAULT_BALL_SZ,
                next_tick = 0,
                state = B_MOVE,
             }

function Ball:new(o, level, color)
    local function random_color()
        if color == nil then
            if rb.lcd_rgbpack ~= nil then -- color target
                return rb.lcd_rgbpack(irand(1,255), irand(1,255), irand(1,255))
            end
            color = irand(1, rb.LCD_DEPTH)
            color = (rb.LCD_DEPTH == 2) and (3 - color) or color -- invert for 2-bit screens
        end
        return color
    end

    if o == nil then
        level = level or 1
        local maxdelay = (level <= 5) and 15 or level * 2
        o = {
                x = irand(1, LCD_W - self.sz),
                y = irand(1, LCD_H - self.sz),
                color = random_color(),
                xi = self.genSpeed(), -- x increment; x + sz after explode
                yi = self.genSpeed(), -- y increment; y + sz after explode
                step_delay    = irand(3, maxdelay / 2),
                explosion_sz  = irand(2 * self.sz, 4 * self.sz),
                life_ticks = irand(rb.HZ / level, rb.HZ * (maxdelay / 5)),
                draw_fn = nil,
                step_fn = self.step    -- reference to current stepping function
            }
    end
    local Ball = o or {} -- so we can use Ball. instead of self
    
    -- these functions end up in a closure; faster to call, use more RAM
    function Ball.draw()
        _ellipse(_LCD, o.x, o.y, o.x + o.sz, o.y + o.sz, o.color, o.color, true)
    end

    function Ball.draw_exploded()
        _ellipse(_LCD, o.x, o.y, o.xi, o.yi, o.color, nil, true)
    end

    if Ball.draw_fn == nil then
        Ball.draw_fn = Ball.draw -- reference to current drawing function
    end

    setmetatable(o, self)
    self.__index = self
    return o
end

-- these functions are shared by reference, slower to call, use less RAM
function Ball:genSpeed()
    local speed = irand(-MAX_BALL_SPEED, MAX_BALL_SPEED)
    return speed ~= 0 and speed or MAX_BALL_SPEED -- Make sure all balls move
end

function Ball:step(tick)
    local function rndspeed(cur)
        local speed = cur + irand(-2, 2)
        if speed < -MAX_BALL_SPEED then
            speed = -MAX_BALL_SPEED
        elseif speed > MAX_BALL_SPEED then
            speed = MAX_BALL_SPEED
        elseif speed == 0 then
            speed = cur
        end
        return speed
    end

    self.next_tick = tick + self.step_delay
    self.x = self.x + self.xi
    self.y = self.y + self.yi
    local max_x = LCD_W - self.sz
    local max_y = LCD_H - self.sz

    if self.x <= 0 then
        self.xi = -self.xi
        self.x = 1
        self.yi = rndspeed(self.yi)
    elseif self.x >= max_x then
        self.xi = -self.xi
        self.x = max_x
        self.yi = rndspeed(self.yi)
    end

    if self.y <= 0 then
        self.yi = -self.yi
        self.y = 1
        self.xi = rndspeed(self.xi)
    elseif self.y >= max_y then
        self.yi = -self.yi
        self.y = max_y
        self.xi = rndspeed(self.xi)
    end
end

function Ball:step_exploded(tick)
    -- exploding ball state machine
    -- B_EXPLODE >> B_DIE >> BWAIT >> B_IMPLODE >> B_DEAD
    if self.state == B_EXPLODE and self.sz < self.explosion_sz then
        self.sz = self.sz + 2
        self.x  = self.x - 1 -- stay centered
        self.y  = self.y - 1
    elseif self.state == B_DIE then
        self.state = B_WAIT
        self.next_tick = tick + self.life_ticks
        return
    elseif self.state == B_IMPLODE then
        if self.sz > 0 then
            self.sz = self.sz - 2
            self.x  = self.x + 1 -- stay centered
            self.y  = self.y + 1
        else
            self.state = B_DEAD
            self.draw_fn = Empty_fn
            self.step_fn = Empty_fn
            return B_DEAD
        end
    elseif self.next_tick < tick then -- decay to next lower state
        self.state = self.state - 1
        return
    end
    -- fell through, update next_tick and impact region
    self.next_tick = tick + self.step_delay
    self.xi = self.x + self.sz
    self.yi = self.y + self.sz
end

function Ball:checkHit(other)
    local x, y = self.x, self.y
    if (other.xi >= x) and (other.yi >= y) then
        local sz = self.sz
        local xi, yi = x + sz, y + sz
        if (xi >= other.x) and (yi >= other.y) then
            -- update impact region
            self.xi = xi
            self.yi = yi
            -- change to exploded state
            self.state = B_EXPLODE
            self.draw_fn = self.draw_exploded
            self.step_fn = self.step_exploded

            if other.state < B_EXPLODE then -- add duration to the ball that got hit
                other.next_tick = other.next_tick + self.life_ticks
            end
            return true
        end
    end

    return false
end
--------------------------------------------------------------------------------


--[[ Cursor Functions ]]--------------------------------------------------------
local Cursor = {
                sz = (DEFAULT_BALL_SZ * 2),
                x  = (LCD_W / 2),
                y  = (LCD_H / 2),
                color = DEFAULT_FG_CLR
                --image = nil
             }

function Cursor:new()
    if rb.LCD_DEPTH == 2 then -- invert for 2 - bit screens
        self.color = 3 - DEFAULT_FG_CLR
    end
    if not HAS_TOUCHSCREEN and not self.image then
        local function create_image(sz)
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
            return img
        end
        self.image = create_image(DEFAULT_BALL_SZ * 2)
    end

    function Cursor.draw()
        rocklib_image.copy(_LCD, self.image, self.x, self.y, _NIL, _NIL,
                           _NIL, _NIL, true, BSAND, self.color)
    end

    return self
end

function Cursor:do_action(action)
    local function clamp_roll(iVal, iMin, iMax)
        if iVal < iMin then
            iVal = iMax
        elseif iVal > iMax then
            iVal = iMin
        end
        return iVal
    end
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
    else
        Action_Evt = pla.ACTION_NONE
        return false
    end

    self.x = clamp_roll(self.x + xi, 1, LCD_W - self.sz)
    self.y = clamp_roll(self.y + yi, 1, LCD_H - self.sz)
    Action_Evt = pla.ACTION_NONE
    return false
end
--------------------------------------------------------------------------------


--[[ Game function ]]-----------------------------------------------------------
function start_round(level, goal, nrBalls, total)
    Player_Added = false
    --[[ References ]]--
    local current_tick = rb.current_tick
    local lcd_update   = rb.lcd_update
    local lcd_clear_display = rb.lcd_clear_display

    local test_spd = false
    local is_exit = false;
    local score = 0
    local last_expend, nrBalls_expend = 0, 0
    local Balls = {}
    local balls_exploded = 1 -- to keep looping when player_added == false

    local tick_next = 0
    local cursor = nil
    local draw_cursor = Empty_fn
    local refresh = rb.HZ/20

    local function level_stats(level, total)
        return strfmt(FMT_LEVEL, level), strfmt(FMT_TOTPTS, total)
    end

    local str_level, str_totpts = level_stats(level, total) -- static for lvl
    local str_expend, str_lvlpts

    local function update_stats()
        -- we only create a new string when a hit is detected
        str_expend = strfmt(FMT_EXPEND, nrBalls_expend)
        str_lvlpts = strfmt(FMT_LVPTS, score)
    end

    function draw_stats()
        local function draw_pos_str(bottom, right, str)
            local w, h = getstringsize(str)
            local x = (right > 0) and ((LCD_W - w) * right - 1) or 1
            local y = (bottom > 0) and ((LCD_H - h) * bottom - 1) or 1
            lcd_putsxy(x, y, str)
        end
        -- pos(B, R) [B/R]=0 => [y/x]=1, [B/R]=1 => [y/x]=lcd[H/W]-string[H/W]
        draw_pos_str(0, 0, str_expend)
        draw_pos_str(0, 1, str_level)
        draw_pos_str(1, 1, str_lvlpts)
        draw_pos_str(1, 0, str_totpts)
    end

    -- all Balls share same function, will by changed by player_add()
    local checkhit_fn = Empty_fn

    local function checkhit(Ball)
        if Ball.state == B_MOVE then
            for i = #Balls, 1, -1 do
                if Balls[i].state < B_MOVE and
                    Ball:checkHit(Balls[i]) then -- exploded?
                        balls_exploded = balls_exploded + 1
                        nrBalls_expend = nrBalls_expend + 1
                        break
                end
            end
        end
    end

    local function add_player()
        -- cursor becomes exploded ball
        local player = Ball:new({
                            x = cursor.x - cursor.sz,
                            y = cursor.y - cursor.sz,
                            color = cursor.color,
                            step_delay = 3,
                            explosion_sz = (3 * DEFAULT_BALL_SZ),
                            life_ticks = (test_spd) and (rb.HZ) or
                                        irand(rb.HZ * 2, rb.HZ * DEFAULT_BALL_SZ),
                            sz = 10,
                            state = B_EXPLODE
                        })
        -- set x/y impact region -->[]
        player.xi = player.x + player.sz
        player.yi = player.y + player.sz
        player.draw_fn = player.draw_exploded
        player.step_fn = player.step_exploded

        table.insert(Balls, player)
        balls_exploded = 1
        Player_Added = true
        cursor = nil
        draw_cursor = Empty_fn
        -- only need collision detection after player add
        checkhit_fn = checkhit
    end

    local function speedtest()
        -- check speed of target
        set_foreground(DEFAULT_BG_CLR) --hide text during test
        level = 1
        nrBalls = 100
        cursor = {x = LCD_W * 2, y = LCD_H * 2, color = 0, sz = 1}
        table.insert(Balls, Ball:new({
                            x = 1, y = 1, xi = 1, yi = 1,
                            color = 0, step_delay = 0,
                            explosion_sz = 0, life_ticks = 0,
                            draw_fn = Empty_fn,
                            step_fn = function() test_spd = test_spd + 1 end
                        })
                    )
        test_spd = 0
        add_player()
    end

    local function screen_redraw()
        -- (draw_fn changes dynamically at runtime)
        for i = 1, #Balls do
            Balls[i].draw_fn()
        end

        draw_stats()
        draw_cursor()

        lcd_update()
        lcd_clear_display()
    end

    local function game_loop()
        local tick = current_tick()
        for _, Ball in ipairs(Balls) do
            if tick > Ball.next_tick then
                -- (step_fn changes dynamically at runtime)
                if Ball:step_fn(tick) == B_DEAD then
                    balls_exploded = balls_exploded - 1
                else
                    checkhit_fn(Ball)
                end
            end
        end
        return tick
    end

    if level < 1 then
        speedtest()
        local bkcolor = (rb.LCD_DEPTH == 2) and (3) or 0
        -- Initialize the balls
        if DEBUG then bkcolor = nil end
        for i=1, nrBalls do
            table.insert(Balls, Ball:new(nil, level, bkcolor))
        end
    else
        speedtest = nil
        set_foreground(DEFAULT_FG_CLR) -- color for text
        cursor = Cursor:new()
        if not HAS_TOUCHSCREEN then
            draw_cursor = cursor.draw
        end
        -- Initialize the balls
        for i=1, nrBalls do
            table.insert(Balls, Ball:new(nil, level))
        end
    end

    -- Make sure there are no unwanted touchscreen presses
    rb.button_clear_queue()

    Action_Evt = pla.ACTION_NONE

    update_stats() -- load status strings

    if rb.cpu_boost then rb.cpu_boost(true) end
    collectgarbage("collect") -- run gc now to hopefully prevent interruption later

    local duration = current_tick()
     -- Game loop >> Check if the round is over
    while balls_exploded > 0 do
        if Action_Evt == pla.PLA_EXIT then
            is_exit = true
            break
        end

        if game_loop() > tick_next then
            tick_next = current_tick() + refresh
            if not Player_Added then
                if Action_Evt ~= pla.ACTION_NONE and cursor:do_action(Action_Evt) then
                    add_player()
                end
            end

            if nrBalls_expend ~= last_expend then -- hit detected?
                last_expend = nrBalls_expend
                score = (nrBalls_expend * level) * SCORE_MULTIPLY
                update_stats() -- only update stats when not current
                if nrBalls_expend == nrBalls then break end -- round is over?
            end

            screen_redraw_count = screen_redraw_count + 1
            screen_redraw()
        end
        rb.yield() -- yield to other tasks
    end

    screen_redraw_duration = screen_redraw_duration + (rb.current_tick() - duration)
    if rb.cpu_boost then rb.cpu_boost(false) end

    if test_spd and test_spd > 0 then
        return test_spd, screen_redraw_count, screen_redraw_duration *10 --ms
    end

    -- splash the final stats for a moment at end
    lcd_clear_display()
    for _, Ball in ipairs(Balls) do
        -- move balls back to their initial exploded positions
        if Ball.state == B_DEAD then
            Ball.sz = Ball.explosion_sz + 2
            Ball.x  = Ball.x - (Ball.explosion_sz / 2)
            Ball.y  = Ball.y - (Ball.explosion_sz / 2)
        end
        Ball:draw()
    end

    if DEFAULT_BALL_SZ > 3 then -- dither
        _LCD:clear(nil, nil, nil, nil, nil, nil, 2, 2)
    end

    total = calc_score(total, level, goal, nrBalls_expend)
    str_level, str_totpts = level_stats(level, total)
    draw_stats()
    lcd_update()
    wait_anykey(2)

    return is_exit, score, nrBalls_expend
end
--------------------------------------------------------------------------------


--[[MAIN PROGRAM]]--------------------------------------------------------------
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

local eva  = rockev.register("action", action_event, rb.HZ / 10)

math.randomseed(os.time() or 1)

local idx, highscore = 1, 0

disp_msg(rb.HZ, "BoomShine")
test_speed()
rb.sleep(rb.HZ * 2)
test_speed = nil

while levels[idx] ~= nil do
    local goal, nrBalls = levels[idx][1], levels[idx][2]

    collectgarbage("collect") -- run gc now to hopefully prevent interruption later

    disp_msg(rb.HZ * 2, "Level %d: get %d out of %d balls", idx, goal, nrBalls)

    local is_exit, score, nrBalls_expend = start_round(idx, goal, nrBalls, highscore)
    if DEBUG == true then
        local fps = screen_redraw_count * 100 / screen_redraw_duration
        disp_msg(rb.HZ * 5, "Redraw: %d fps", fps)
    end

    if is_exit then
        break -- Exiting..
    else
        highscore, score, bonus = calc_score(highscore, idx, goal, nrBalls_expend)
        if nrBalls_expend >= goal then
            if bonus == 0 then
                disp_msg(rb.HZ * 2, "You won!")
            else
                disp_msg(rb.HZ * 2, "You won BONUS!")
            end
            levels[idx] = nil
            idx = idx + 1
        else
            disp_msg(rb.HZ * 2, "You lost %d points!", -score)
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
if rb.cpu_boost then rb.cpu_boost(false) end

os.exit()
--------------------------------------------------------------------------------
