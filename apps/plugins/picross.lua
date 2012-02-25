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

-- set colors on color targets
if rb.lcd_rgbpack ~= nil then
    rb.lcd_set_background(rb.lcd_rgbpack(255, 255, 255))
    rb.lcd_set_foreground(rb.lcd_rgbpack(0, 0, 0))
end

-- load images
local img_numbers = rb.read_bmp_file(rb.current_path().."picross_numbers.bmp")

-- image helper function
function draw_image(img, x, y, w, h, tilenum)

    local func = rb.lcd_bitmap_transparent_part
              or rb.lcd_bitmap_part      -- Fallback version for grayscale targets
              or rb.lcd_mono_bitmap_part -- Fallback version for mono targets

    func(img, 0, (tilenum * h), w, x, y, w, h)
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
    cursor = {x = 0, y = 0}
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
    -- metrics
    self.puzzleh = #self.solution
    self.puzzlew = #self.solution[1]
    self.numbersh = math.floor(self.puzzleh / 2) + 1
    self.numbersw = math.floor(self.puzzlew / 2) + 1
    self.boardh = self.puzzleh + self.numbersh
    self.boardw = self.puzzlew + self.numbersw

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
    self.cursor.x = self.numbersh + 1
    self.cursor.y = self.numbersw + 1

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
    local file = io.open('/.rockbox/rocks/games/picross.sav', 'w')
    if file then
        file:write("#"..self.filename.."\n")
        for r = self.numbersh + 1, self.boardh do
            for c = self.numbersw + 1, self.boardw do
                file:write(self.board[r][c])
            end
            file:write("\n")
        end
        file:close()
        rb.splash(rb.HZ, "Game saved.")
        return true
    else
        rb.splash(rb.HZ * 2, "Failed to open save file")
        return false
    end
end

function State:loadSave()
    local file = io.open('/.rockbox/rocks/games/picross.sav')
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
    self:loadFile('/.rockbox/rocks/games/picross_default.picross')
end

function State:loadFile(path)
    local file = io.open(path)
    if file then
        local board = {}
        local boardwidth = 0

        contents = file:read('*all')

        for line in contents:gmatch("[^\r\n]+") do
            -- ignore blank lines and comments
            if line ~= '' and line:sub(1, 1) ~= '#' then
                table.insert(board, {}) -- add a new row

                -- ensure all lines are the same width
                if boardwidth == 0 then
                    boardwidth = #line
                elseif #line ~= boardwidth then
                    -- a line was the wrong width
                    rb.splash(rb.HZ * 2, "Invalid puzzle file! (wrong line width)")
                    return false
                end

                for char in line:gmatch('.') do
                    if char == '*' or char == '.' then
                        table.insert(board[#board], char)
                    else
                        -- invalid character in puzzle area
                        rb.splash(rb.HZ * 2, "Invalid puzzle file! (invalid character)")
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
    local tw, th = 10, 10 -- tile width and height (including bottom+right padding)

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
    rb.lcd_drawrect(ofsx + (self.cursor.x * tw) - 1, ofsy + (self.cursor.y * th) - 1, 11, 11)

    -- tiles
    for r = 1, self.boardh do
        for c = 1, self.boardw do
            local x, y = ofsx + (c * tw) + 1, ofsy + (r * th) + 1

            if self.board[r][c] == '.' then
                -- unfilled square
            elseif self.board[r][c] == '*' then
                -- filled square
                rb.lcd_fillrect(x, y, 7, 7)
            elseif self.board[r][c] == 'x' then
                -- eliminated square
                rb.lcd_drawline(x + 1, y + 1, x + 5, y + 5)
                rb.lcd_drawline(x + 5, y + 1, x + 1, y + 5)
            elseif self.board[r][c] == ' ' then
                -- empty space
            elseif self.board[r][c] > 0 and self.board[r][c] < 100 then
                -- number
                local num = self.board[r][c]
                if num < 10 then
                    draw_image(img_numbers, x + 2, y, 3, 7, num)
                else
                    draw_image(img_numbers, x, y, 3, 7, math.floor(num / 10)) -- tens place
                    draw_image(img_numbers, x + 4, y, 3, 7, num % 10) -- ones place
                end
            end
        end
    end

    rb.lcd_update()
end

function State:isComplete()
    for r = 1,self.puzzleh do
        for c = 1,self.puzzlew do
            if self.solution[r][c] == '*' and self.board[self.numbersh + r][self.numbersw + c] ~= '*' then
                return false
            end
        end
    end

    return true
end

function State:moveCursor(dir)
    -- The cursor isn't allowed to move in the top-left quadrant of the board.
    -- This has to be checked in up and left moves.
    if dir == 'up' then
        if (self.cursor.y > (self.numbersh + 1) or self.cursor.x > self.numbersw)
        and self.cursor.y > 1 then
            self.cursor.y = self.cursor.y - 1
        end
    elseif dir == 'down' then
        if self.cursor.y < self.boardh then
            self.cursor.y = self.cursor.y + 1
        end
    elseif dir == 'left' then
        if (self.cursor.x > (self.numbersw + 1) or self.cursor.y > self.numbersh)
        and self.cursor.x > 1 then
            self.cursor.x = self.cursor.x - 1
        end
    elseif dir == 'right' then
        if self.cursor.x < self.boardw then
            self.cursor.x = self.cursor.x + 1
        end
    end
end

function State:fillSquare()
    if self.cursor.x > self.numbersw and self.cursor.y > self.numbersh then
        if self.board[self.cursor.y][self.cursor.x] == '*' then
            -- clear square
            self.board[self.cursor.y][self.cursor.x] = '.'
        else -- '.' or 'x'
            -- fill square
            if self.solution[self.cursor.y - self.numbersh][self.cursor.x - self.numbersw] == '*' then
                self.board[self.cursor.y][self.cursor.x] = '*'
            else
                rb.splash(rb.HZ * 2, "Invalid move!")
                -- "x" square for convenience
                self.board[self.cursor.y][self.cursor.x] = 'x'
            end
        end
    end

    if self:isComplete() then
        rb.splash(rb.HZ * 2, "Puzzle complete!")
    end
end

function State:eliminateSquare()
    if self.cursor.x > self.numbersw and self.cursor.y > self.numbersh then
        if self.board[self.cursor.y][self.cursor.x] == 'x' then
            -- clear square
            self.board[self.cursor.y][self.cursor.x] = '.'
        else -- '.' or '*'
            -- "x" square
            self.board[self.cursor.y][self.cursor.x] = 'x'
        end
    end
end

-- main code ------------------------------------------------------------------

function mainMenu()
    local menu = {
        "Resume",
        "View picture",
        "Restart puzzle",
        "Load puzzle",
        "Save progress",
        "Save and quit",
        "Quit without saving"
    }

    while true do
        local s = rb.do_menu("Picross", menu, nil, false)

        if s == 0 then
            -- resume
            return
        elseif s == 1 then
            -- view picture
            viewPicture()
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
            -- save
            if State:saveGame() then
                return
            end
        elseif s == 5 then
            -- save and quit
            if State:saveGame() then
                os.exit()
            end
        elseif s == 6 then
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
    if rb.dir_exists('/picross') then
        local files = {}
        for file in luadir.dir('/picross') do
            if file ~= '.' and file ~= '..' then
                table.insert(files, file)
            end
        end

        table.sort(files)

        if #files > 0 then
            local s = rb.do_menu("Puzzles", files, nil, false)
            if s >= 0 and s < #files then
                if State:loadFile('/picross/'..files[s+1]) then
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
            rb.splash(rb.HZ * 2, "No puzzles found! Put .picross files in /picross")
            return false
        end
    else
        rb.splash(rb.HZ * 2, "Put .picross files in /picross")
        return false
    end
end

function viewPicture()
    rb.lcd_clear_display()

    -- draw filled squares as pixels (scaled 2x)
    for r = State.numbersh + 1, State.boardh do
        for c = State.numbersw + 1, State.boardw do
            if State.board[r][c] == '*' then
                rb.lcd_drawpixel(c - State.numbersw, r - State.numbersh)
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
    State:loadDefault()
end

while true do
    local action = rb.get_plugin_action(0)

    if action == rb.actions.PLA_EXIT then
        mainMenu()
    elseif action == rb.actions.PLA_UP or action == rb.actions.PLA_UP_REPEAT then
        State:moveCursor('up')
    elseif action == rb.actions.PLA_DOWN or action == rb.actions.PLA_DOWN_REPEAT then
        State:moveCursor('down')
    elseif action == rb.actions.PLA_LEFT or action == rb.actions.PLA_LEFT_REPEAT then
        State:moveCursor('left')
    elseif action == rb.actions.PLA_RIGHT or action == rb.actions.PLA_RIGHT_REPEAT then
        State:moveCursor('right')
    elseif action == rb.actions.PLA_SELECT then
        State:fillSquare()
    elseif action == rb.actions.PLA_CANCEL then
        State:eliminateSquare()
    end

    State:drawBoard()

    rb.yield()
end
