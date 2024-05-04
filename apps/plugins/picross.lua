--[[
             __________               __   ___.
   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
                     \/            \/     \/    \/            \/

Port of Picross (aka. Picture Crossword, Nonograms, Paint By Numbers)
Copyright (c) 2012 by Nathan Korth

See http://en.wikipedia.org/wiki/Nonogram for details on how to play, and
http://nkorth.com/picross for more puzzles.

]]--

require "actions"
require "luadir"
require("rbsettings")
require("settings")

local _nums = require("draw_num")
local _clr   = require("color") -- clrset, clrinc provides device independent colors
local _lcd   = require("lcd")   -- lcd helper functions

local plugindir = rb.PLUGIN_GAMES_DATA_DIR
local userdir = plugindir .. "/.picross"

local wrap = rb.settings.read('global_settings', rb.system.global_settings.list_wraparound)
wrap = (wrap or 1) == 1

do     -- free up some ram by removing items we don't need
    local function strip_functions(t, ...)
        local t_keep = {...}
        local keep
        for key, val in pairs(t) do
            keep = false
            for _, v in ipairs(t_keep) do
                if string.find (key, v) then
                    keep = true; break
                end
            end
            if keep ~= true then
                t[key] = nil
            end
        end
    end

    strip_functions(rb.actions, "PLA_", "TOUCHSCREEN", "_NONE")
    rb.contexts = nil

    _clr.inc = nil
    rb.metadata = nil -- remove metadata settings
    rb.system = nil -- remove system settings
    rb.settings = nil --remove setting read/write fns
end

--colors for fg/bg ------------------------
local WHITE = _clr.set(-1, 255, 255, 255)
local BLACK = _clr.set(0, 0, 0, 0)
-------------------------------------------

-- set colors on color targets
if rb.lcd_rgbpack ~= nil then
    rb.lcd_set_background(rb.lcd_rgbpack(255, 255, 255))
    rb.lcd_set_foreground(rb.lcd_rgbpack(0, 0, 0))
end

local TEXT_COLOR = BLACK

if rb.LCD_DEPTH == 2 then TEXT_COLOR = bit.bnot(TEXT_COLOR) end

--[[
-- load images
local img_numbers = rb.read_bmp_file(rb.current_path().."picross_numbers.bmp")

-- image helper function
function draw_image(img, x, y, w, h, tilenum)

    local func = rb.lcd_bitmap_transparent_part
              or rb.lcd_bitmap_part      -- Fallback version for grayscale targets
              or rb.lcd_mono_bitmap_part -- Fallback version for mono targets

    func(img, 0, (tilenum * h), w, x, y, w, h)
end
]]

function draw_number(x, y, w, tilenum, scale)
    scale = scale or 1
    _nums.print(_LCD, tilenum, x, y, w, TEXT_COLOR, nil, nil, true, scale, scale)
end

function showComplete(self)
    if self:isComplete() then
        rb.splash(rb.HZ * 2, "Puzzle complete!")
        self:saveGame()
        self.puzzleh = 50
        self.puzzlew = 50
        local old_boardh, old_boardw = self.boardh, self.boardw
        local old_numbersw, old_numbersh = self.numbersw, self.numbersh
        while self.numbersh > 0 do -- remove the number rows
            table.remove (self.board, 1)
            self.numbersh = self.numbersh - 1
        end
        self.numbersh = 0
        self.numbersw = 0
        self.solution = nil
        self.boardh = self.puzzleh
        self.boardw = self.puzzlew
        self.freedraw = 0
        -- find a free number
        while rb.file_exists(string.format("%s/user_freedraw_%d.picross",
                                                userdir, self.freedraw)) do
            self.freedraw = self.freedraw + 1
        end

        for r = 1, self.boardh do
            local old_row = self.board[r] or {}

            self.board[r] = {}
            -- copy over the last drawing
            for c = 1, self.boardw do
                local ch = old_row[c + old_numbersw]
                if not ch or ch ~= '*' then
                    self.board[r][c] = '.'
                else
                    self.board[r][c] = '*'
                end
            end
        end
        rb.splash(rb.HZ * 3, "Free Draw!")
        self.cursor.x = 1
        self.cursor.y = 1
        self.showComplete = function() end --show once, then remove the reference
    end
end

local State = {
    puzzlew = 0,
    puzzleh = 0,
    numbersw = 0,
    numbersh = 0,
    boardw = 0,
    boardh = 0,
    board = {},
    solution = {},
    filename = '',
    cursor = {x = 0, y = 0},
    scale = 1,
    freedraw = false
}

--[[

Notes on how puzzles work in the code:

The "board" array is bigger than the actual puzzle, so the numbers
above and to the left of it can be stored and do not need to be recalculated
for every draw. (The "solution" array is the same size as the puzzle, however.)
The various width/height variables help keep track of where everything is.
(they should be fairly self-explanatory)

The width/height of the numbers section is always half the width/height of the
puzzle. For odd-number-sized puzzles, the value must be rounded up. This is
because strings of squares must have at least one blank space in between. For
example, on a board 5 wide, the maximum set of row numbers is "1 1 1".

Here are the values used in the "board" array:
    ' ': empty space
    '.': unfilled square
    '*': filled square
    [number]: number (not a string)

The .picross puzzle files are text files which look pretty much the same as the
"board" array, with two differences:

  - puzzle files should not contain numbers, because they will be generated
    based on the puzzle at runtime
  - blank lines and lines starting with '#' are ignored

]]--

function State:initBoard()
    if self.freedraw then
        -- clear board (set the puzzle area to '.' and everything else to ' ')
        self.board = {}
        for r = 1,self.boardh do
            self.board[r] = {}
            for c = 1,self.boardw do
                if r > self.numbersh and c > self.numbersw then
                    self.board[r][c] = '.'
                else
                    self.board[r][c] = ' '
                end
            end
        end

        -- reset cursor
        self.cursor.x = 1
        self.cursor.y = 1
        return
    end --freedraw

    -- metrics
    self.puzzleh = #self.solution
    self.puzzlew = #self.solution[1]
    self.numbersh = math.floor(self.puzzleh / 2) + 1
    self.numbersw = math.floor(self.puzzlew / 2) + 1
    self.boardh = self.puzzleh + self.numbersh
    self.boardw = self.puzzlew + self.numbersw
    self.showComplete = showComplete

    -- clear board (set the puzzle area to '.' and everything else to ' ')
    self.board = {}
    for r = 1,self.boardh do
        self.board[r] = {}
        for c = 1,self.boardw do
            if r > self.numbersh and c > self.numbersw then
                self.board[r][c] = '.'
            else
                self.board[r][c] = ' '
            end
        end
    end

    -- reset cursor
    self.cursor.x = self.numbersw + 1
    self.cursor.y = self.numbersh + 1

    -- calculate row numbers
    local rownums = {}
    for r = 1,self.puzzleh do
        rownums[r] = {}
        local count = 0
        for c = 1,self.puzzlew do
            if self.solution[r][c] == '*' then
                -- filled square
                count = count + 1
            else
                -- empty square
                if count > 0 then
                    table.insert(rownums[r], count)
                    count = 0
                end
            end
        end
        -- if there were no empty squares
        if count > 0 then
            table.insert(rownums[r], count)
            count = 0
        end
    end

    -- calculate column numbers
    local columnnums = {}
    for c = 1,self.puzzlew do
        columnnums[c] = {}
        local count = 0
        for r = 1,self.puzzleh do
            if self.solution[r][c] == '*' then
                -- filled square
                count = count + 1
            else
                -- empty square
                if count > 0 then
                    table.insert(columnnums[c], count)
                    count = 0
                end
            end
        end
        -- if there were no empty squares
        if count > 0 then
            table.insert(columnnums[c], count)
            count = 0
        end
    end

    -- add row numbers to board
    for r = 1,self.puzzleh do
        for i,num in ipairs(rownums[r]) do
            self.board[self.numbersh + r][self.numbersw - #rownums[r] + i] = num
        end
    end

    -- add column numbers to board
    for c = 1,self.puzzlew do
        for i,num in ipairs(columnnums[c]) do
            self.board[self.numbersh - #columnnums[c] + i][self.numbersw + c] = num
        end
    end
end

function State:saveGame()
    local file
    local boardw, boardh = self.boardw, self.boardh

    if self.freedraw then
        self.filename = string.format("%s/user_freedraw_%d.picross", userdir, self.freedraw)


        --remove blank lines from the end
        while boardh > 1 and not string.find(table.concat(self.board[boardh]), "\*") do
            boardh = boardh - 1
        end
        --remove blank lines from right
        local max_w = 0
        for r = self.numbersh + 1, boardh do
            for c = max_w + 1, boardw do
                if self.board[r][c] == '*' then
                    max_w = c
                end
            end
        end
        boardw = max_w
        if max_w == 0 then return end--nothing to save

        file = io.open(self.filename, 'w')
    else
        file = io.open(plugindir .. '/picross.sav', 'w')
    end

    if file then
        file:write("#"..self.filename.."\n")
        for r = self.numbersh + 1, boardh do
            for c = self.numbersw + 1, boardw do
                file:write(self.board[r][c])
            end
            file:write("\n")
        end
        file:close()
        if self.freedraw then
            rb.splash(rb.HZ, "Freedraw saved.")
        else
            rb.splash(rb.HZ, "Game saved.")
        end
        return true
    else
        rb.splash(rb.HZ * 2, "Failed to open save file")
        return false
    end
end

function State:loadSave()
    local file = io.open(plugindir .. '/picross.sav')
    if file then
        -- first line is commented path of original puzzle
        path = file:read('*l')
        path = path:sub(2,-1)
        -- prepare original puzzle
        if self:loadFile(path) then
            -- load saved board
            contents = file:read('*all')
            file:close()
            local r = 1
            for line in contents:gmatch("[^\r\n]+") do
                local c = 1
                for char in line:gmatch('.') do
                    self.board[self.numbersh + r][self.numbersw + c] = char
                    c = c + 1
                end
                r = r + 1
            end

            return true
        else
            return false
        end
    else
        return false
    end
end

function State:loadDefault()
    return self:loadFile(userdir .. '/picross_default.picross')
end

function State:loadFile(path)
    local file = io.open(path)
    if file then
        self.freedraw = false
        local board = {}
        local boardwidth = 0
        local count = 0
        contents = file:read('*all')

        for line in contents:gmatch("[^\r\n]+") do
            count = count + 1
            -- ignore blank lines and comments
            if line ~= '' and line:sub(1, 1) ~= '#' then
                table.insert(board, {}) -- add a new row

                -- ensure all lines are the same width
                if boardwidth == 0 then
                    boardwidth = #line
                elseif #line ~= boardwidth then
                    -- a line was the wrong width
                    local err = "Invalid puzzle file!"
                    local msg =
                     string.format("%s (wrong line width ln: %d w: %d)", err, count, #line)
                    rb.splash(rb.HZ * 2, msg)
                    return false
                end
                local pos = 0
                for char in line:gmatch('.') do
                    pos = pos + 1
                    if char == '*' or char == '.' then
                        table.insert(board[#board], char)
                    else
                        local err = "Invalid puzzle file!"
                        local msg = string.format("%s (invalid character ln: %d '%s' @ %d)",
                                                                       err, count, char, pos)
                        -- invalid character in puzzle area
                        rb.splash(rb.HZ * 2, msg)
                        return false
                    end
                end
            else
                -- display puzzle comments
                --rb.splash(rb.HZ, line:sub(2,#line))
            end
        end

        if #board == 0 then
           -- empty file
           rb.splash(rb.HZ * 2, "Invalid puzzle file! (empty)")
           return false
        end

        file:close()

        self.solution = board
        self.filename = path
        if self.puzzleh < 100 and self.puzzlew < 100 then
            self:initBoard()
            return true
        else
            -- puzzle too big
            rb.splash(rb.HZ * 2, "Invalid puzzle file! (too big)")
            return false
        end
    else
        -- file open failed
        rb.splash(rb.HZ * 2, "Failed to open file!")
        return false
    end
end

function State:drawBoard()
    local tw, th = 10 * self.scale, 10 * self.scale -- tile width and height (including bottom+right padding)

    local ofsx = rb.LCD_WIDTH/2 - 4 - (self.cursor.x * tw)
    local ofsy = rb.LCD_HEIGHT/2 - 4 - (self.cursor.y * th)

    rb.lcd_clear_display()

    -- guide lines
    for r = 0, self.puzzleh do
        local x1, x2, y =
            ofsx + tw - 1,
            ofsx + ((self.boardw + 1) * tw) - 1,
            ofsy + ((self.numbersh + 1 + r) * th) - 1
        if r % 5 == 0 or r == self.puzzleh then
            rb.lcd_hline(x1, x2, y)
        else
            for x = x1, x2, 2 do
                rb.lcd_drawpixel(x, y)
            end
        end
    end
    for c = 0, self.puzzlew do
        local x, y1, y2 =
        ofsx + ((self.numbersw + 1 + c) * tw) - 1,
        ofsy + th - 1,
        ofsy + ((self.boardh + 1) * th) - 1
        if c % 5 == 0 or c == self.puzzlew then
            rb.lcd_vline(x, y1, y2)
        else
            for y = y1,y2, 2 do
                rb.lcd_drawpixel(x, y)
            end
        end
    end

    -- cursor
    local cx, cy = ofsx + (self.cursor.x * tw) - 1, ofsy + (self.cursor.y * th) - 1
    rb.lcd_drawrect(cx, cy, tw + 1, th + 1)
    local n_width = tw / self.scale / 2 - 1
    local xc = (tw - 5 * self.scale) / 2
    -- tiles
    for r = 1, self.boardh do
        for c = 1, self.boardw do
            local x, y = ofsx + (c * tw) + 1, ofsy + (r * th) + 1

            if self.board[r][c] == '.' then
                -- unfilled square
            elseif self.board[r][c] == '*' then
                -- filled square
                rb.lcd_fillrect(x, y, tw - 3, th - 3)
            elseif self.board[r][c] == 'x' then
                -- eliminated square
                rb.lcd_drawline(x + 1, y + 1, x + tw - 5, y + th - 5)
                rb.lcd_drawline(x + tw - 5, y + 1, x + 1, y + th - 5)
            elseif self.board[r][c] == ' ' then
                -- empty space
            elseif self.board[r][c] > 0 and self.board[r][c] < 100 then
                -- number
                local num = self.board[r][c]
                if num < 10 then
                    draw_number(x + xc, y, n_width, num, self.scale)
                    draw_number(x + xc + 1, y, n_width, num, self.scale)
                else
                    draw_number(x, y, n_width, num, self.scale)
                    draw_number(x + 1, y, n_width, num, self.scale)
                end
            end
        end
    end

    rb.lcd_update()
end

function State:isComplete()
    for r = 1,self.puzzleh do
        for c = 1,self.puzzlew do
            if self.solution[r][c] == '*' and
               self.board[self.numbersh + r][self.numbersw + c] ~= '*' then
                return false
            end
        end
    end

    return true
end

function State:moveCursor(dir)
    -- The cursor isn't allowed to move in the top-left quadrant of the board.
    -- This has to be checked in up and left moves.
    local in_board_area = (self.cursor.y > (self.numbersh + 1)
                          and self.cursor.x > self.numbersw + 1)

    if dir == 'left' then
        if (self.cursor.x > (self.numbersw + 1) or self.cursor.y > self.numbersh)
        and self.cursor.x > 1 then
            self.cursor.x = self.cursor.x - 1
        elseif wrap == true then
            if in_board_area then
                self.cursor.x = 1
            else
                self.cursor.x = self.boardw
            end
            dir = 'up'
        end
    elseif dir == 'right' then
        if self.cursor.x < self.boardw then
            self.cursor.x = self.cursor.x + 1
        elseif wrap == true then
            if in_board_area then
                self.cursor.x = 1
            else
                self.cursor.x = self.numbersw + 1
            end
            dir = 'down'
        end
    end

    if dir == 'up' then
        if (self.cursor.y > (self.numbersh + 1) or self.cursor.x > self.numbersw)
        and self.cursor.y > 1 then
                self.cursor.y = self.cursor.y - 1
        elseif wrap == true then
            if in_board_area then
                self.cursor.y = 1
            else
                self.cursor.y = self.boardh
            end
        end
    elseif dir == 'down' then
        if self.cursor.y < self.boardh then
            self.cursor.y = self.cursor.y + 1
        elseif wrap == true then
            if in_board_area then
                self.cursor.y = 1
            else
                self.cursor.y = self.numbersh + 1
            end
        end
    end
end

function State:fillSquare(mode)
    mode = mode or 0
    if self.cursor.x > self.numbersw and self.cursor.y > self.numbersh then
        if self.board[self.cursor.y][self.cursor.x] == '*' and mode ~= 2 then
            -- clear square
            self.board[self.cursor.y][self.cursor.x] = '.'
        elseif mode ~= 1 then -- '.' or 'x'
            -- fill square
            local x, y = self.cursor.x - self.numbersw, self.cursor.y - self.numbersh
            if not self.solution or  self.solution[y][x] == '*' then
                self.board[self.cursor.y][self.cursor.x] = '*'
            else
                rb.splash(rb.HZ * 2, "Invalid move!")
                -- "x" square for convenience
                self.board[self.cursor.y][self.cursor.x] = 'x'
            end
        end
    end

    self:showComplete()
end

function State:eliminateSquare()
    if not self.freedraw
       and self.cursor.x > self.numbersw
       and self.cursor.y > self.numbersh then
        if self.board[self.cursor.y][self.cursor.x] == 'x' then
            -- clear square
            self.board[self.cursor.y][self.cursor.x] = '.'
        else-- '.' or '*'
            -- "x" square
            self.board[self.cursor.y][self.cursor.x] = 'x'
        end
    else
        self.board[self.cursor.y][self.cursor.x] = '.'
    end
end

-- main code ------------------------------------------------------------------

local function mainMenu()
    local menu = {
        "Resume",
        "View picture",
        "Restart puzzle",
        "Load puzzle",
        "Zoom " .. State.scale - 1,
        "Save progress",
        "Save and quit",
        "Quit without saving"
    }
    local start

    if State.freedraw then
        menu[6] = "Save freedraw " .. State.freedraw --Save Progress
    end
    while true do
        local s = rb.do_menu("Picross", menu, start, false)
        start = s
        if s == 0 then
            -- resume
            return
        elseif s == 1 then
            -- view picture
            viewPicture()
            start = 0 --resume
        elseif s == 2 then
            -- restart
            State:initBoard()
            return
        elseif s == 3 then
            -- choose puzzle
            if puzzleList() then
                return
            end
        elseif s == 4 then
            -- zoom
            State.scale = State.scale + 1
            if State.scale > 4 then
                State.scale = 1
            end
            menu[5] = "Zoom " .. State.scale - 1
        elseif s == 5 then
            -- save
            if State:saveGame() then
                return
            end
        elseif s == 6 then
            -- save and quit
            if State:saveGame() then
                os.exit()
            end
        elseif s == 7 then
            -- quit
            os.exit()
        elseif s == -2 then
            -- back button pressed
            return
        else
            -- something strange happened
            rb.splash(rb.HZ * 2, "Invalid menu index: "..s)
        end
    end
end

function puzzleList()
    if rb.dir_exists(userdir) then
        local files = {}
        for file in luadir.dir(userdir) do
            if file ~= '.' and file ~= '..' then
                table.insert(files, file)
            end
        end

        table.sort(files)
        local udir = userdir .. "/"
        if #files > 0 then
            local s = rb.do_menu("Puzzles", files, nil, false)
            if s >= 0 and s < #files then
                if State:loadFile(udir..files[s+1]) then
                    return true -- return to puzzle screen
                else
                    -- puzzle failed to load, return to main menu
                    return false
                end
            elseif s == -2 then
                -- back button pressed, return to main menu
                return false
            else
                -- something strange happened
                rb.splash(rb.HZ * 2, "Invalid menu index: "..s)
                return false
            end
        else
            rb.splash(rb.HZ * 2, "No puzzles found! Put .picross files in " .. userdir)
            return false
        end
    else
        rb.splash(rb.HZ * 2, "Put .picross files in " .. userdir)
        return false
    end
end

function viewPicture()
    rb.lcd_clear_display()

    -- draw filled squares as pixels (scaled 2x)
    for r = State.numbersh + 1, State.boardh do
        for c = State.numbersw + 1, State.boardw do
            if State.board[r][c] == '*' then
                --rb.lcd_drawpixel(c - State.numbersw, r - State.numbersh)
                local px = (c - State.numbersw) * State.scale - State.scale + 1
                local py = (r - State.numbersh) * State.scale - State.scale + 1

                rb.lcd_fillrect(px, py, State.scale, State.scale)
            end
        end
    end

    rb.lcd_update()

    -- exit on button press
    while true do
        local action = rb.get_plugin_action(0)

        if action == rb.actions.PLA_EXIT
        or action == rb.actions.PLA_CANCEL
        or action == rb.actions.PLA_SELECT then
            return
        end

        rb.yield()
    end
end

if not State:loadSave() then
    if not State:loadDefault() then
    return;
    end
end

local act = rb.actions
local action = act.ACTION_NONE
local lockdraw = false

while true do
    action = rb.get_plugin_action(0)
    if action == rb.actions.PLA_EXIT then
        lockdraw = false
        mainMenu()
    elseif action == act.PLA_UP or action == act.PLA_UP_REPEAT then
        State:moveCursor('up')
    elseif action == act.PLA_DOWN or action == act.PLA_DOWN_REPEAT then
        State:moveCursor('down')
    elseif action == act.PLA_LEFT or action == act.PLA_LEFT_REPEAT then
        State:moveCursor('left')
    elseif action == act.PLA_RIGHT or action == act.PLA_RIGHT_REPEAT then
        State:moveCursor('right')
    elseif action == act.PLA_SELECT then
        if lockdraw then
            lockdraw = lockdraw - 1
            if lockdraw < 0 then
                lockdraw = false
            elseif lockdraw == 1 then
                rb.splash(50, "clear")
            else
                rb.splash(50, "invert")
            end
        else
            State:fillSquare()
        end
        action = act.ACTION_NONE
    elseif action == act.PLA_SELECT_REPEAT then
        if State.freedraw and not lockdraw then
            lockdraw = 2
            rb.splash(50, "draw")
            action = act.ACTION_NONE
        end
    elseif action == act.PLA_CANCEL then
        State:eliminateSquare()
        action = act.ACTION_NONE
    else
        action = act.ACTION_NONE
    end

    if lockdraw and action ~= act.ACTION_NONE then
        State:fillSquare(lockdraw)
    end

    State:drawBoard()

    rb.yield()
end
