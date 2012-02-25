--[[
             __________               __   ___.
   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
                     \/            \/     \/    \/            \/

 Port of Picross (aka. Picture Crossword, Nonograms, Paint By Numbers)
 Copyright (c) 2012 by Nathan Korth
 
 See http://en.wikipedia.org/wiki/Nonogram for strategy tips, and
 http://nkorth.com/picross for more puzzles.

]]--

require "actions"

-- set colors if available
if rb.lcd_rgbpack ~= nil then
    rb.lcd_set_background(rb.lcd_rgbpack(255, 255, 255))
    rb.lcd_set_foreground(rb.lcd_rgbpack(0, 0, 0))
end

-- load images
local img_cursor = rb.read_bmp_file(rb.current_path().."picross_cursor.bmp")
local img_squares = rb.read_bmp_file(rb.current_path().."picross_squares.bmp")
local img_numbers = rb.read_bmp_file(rb.current_path().."picross_numbers.bmp")

-- image helper function
function draw_image(img, x, y, w, h, tilenum)
    
    local func = rb.lcd_bitmap_transparent_part
              or rb.lcd_bitmap_part      -- Fallback version for grayscale targets
              or rb.lcd_mono_bitmap_part -- Fallback version for mono targets
    
    func(img, 0, (tilenum * h), w, x, y, w, h)
end

local State = {
    board = {},
    boardwidth = 0,
    boardheight = 0,
    solution = {},
    cursor = {x = 0, y = 0}
}

--[[

Notes on how puzzles are represented:

The "board" array is actually twice the width and height of the actual puzzle,
so the numbers above and to the left of it can be stored. (The "solution"
array is only the size of the puzzle, so as not to waste space.) This means
the numbers do not need to be recalculated for every draw, but makes a lot
of the math less readable. ("boardwidth" and "boardheight" hold the actual size
of the puzzle.)

The values in "board" map exactly to the tiles to be drawn, because everything
drawn can be represented as tiles. The values used are:

    ' ': empty space
    '.': unfilled square
    '*': filled square
    [number]: numbers shown at the sides of the board

This is also basically the format for the .picross files in which puzzles are
stored, with two distinctions:

  - puzzle files should not contain the numbers, as they will be generated
    based on the puzzle at runtime
  - blank lines and lines starting with '#' are ignored, like in most text
    formats

]]--

function State:initBoard()
    -- clear board (set the puzzle area to '.' and everything else to ' ')
    for i = 1,(self.boardheight*2) do
        self.board[i] = {}
        for j = 1,(self.boardwidth*2) do
            if i > self.boardheight and j > self.boardwidth then
                self.board[i][j] = '.'
            else
                self.board[i][j] = ' '
            end
        end
    end
    
    -- reset cursor
    self.cursor.x = self.boardheight + 1
    self.cursor.y = self.boardwidth + 1
    
    -- calculate row numbers
    local rownums = {}
    for r = 1,self.boardheight do
        rownums[r] = {}
        local count = 0
        for c = 1,self.boardwidth do
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
    for c = 1,self.boardwidth do
        columnnums[c] = {}
        local count = 0
        for r = 1,self.boardheight do
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
    for r = 1,self.boardheight do
        for i,num in ipairs(rownums[r]) do
            self.board[self.boardheight + r][self.boardwidth - #rownums[r] + i] = num
        end
    end
    
    -- add column numbers to board
    for c = 1,self.boardwidth do
        for i,num in ipairs(columnnums[c]) do
            self.board[self.boardheight - #columnnums[c] + i][self.boardwidth + c] = num
        end
    end
end

function State:loadDefault()
    self.solution = {
        {'*','*','*','*','.'},
        {'*','.','.','.','*'},
        {'*','*','*','*','.'},
        {'*','.','.','.','.'},
        {'*','.','.','.','.'}
    }
    self.boardwidth = 5
    self.boardheight = 5
    self:initBoard()
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
                    os.exit()
                end
                
                for char in line:gmatch('.') do
                    if char == '*' or char == '.' then
                        table.insert(board[#board], char)
                    else
                        -- invalid character in puzzle area
                        rb.splash(rb.HZ * 2, "Invalid puzzle file! (invalid character)")
                        os.exit()
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
           os.exit()
        end
        
        file:close()
        
        self.solution = board
        self.boardwidth = #(board[1])
        self.boardheight = #board
        if self.boardheight < 100 and self.boardwidth < 100 then
            self:initBoard()
        else
            -- puzzle too big
            rb.splash(rb.HZ * 2, "Invalid puzzle file! (too big)")
            os.exit()
        end
    else
        -- file open failed
        rb.splash(rb.HZ * 2, "Failed to open file!")
        os.exit()
    end
end

function State:drawBoard()
    local ofsx = rb.LCD_WIDTH/2 - 4 - (State.cursor.x * 8)
    local ofsy = rb.LCD_HEIGHT/2 - 4 - (State.cursor.y * 8)
    
    rb.lcd_clear_display()
    
    -- cursor
    draw_image(img_cursor, ofsx + (State.cursor.x * 8) - 1, ofsy + (State.cursor.y * 8) - 1, 9, 9, 0)
    
    for r = 1,(self.boardheight*2) do
        for c = 1,(self.boardwidth*2) do
            local x,y = ofsx + (c * 8), ofsy + (r * 8)
            
            if self.board[r][c] == '.' then
                -- unfilled square
                draw_image(img_squares, x, y, 7, 7, 0)
            elseif self.board[r][c] == '*' then
                -- filled square
                draw_image(img_squares, x, y, 7, 7, 1)
            elseif self.board[r][c] == 'x' then
                -- eliminated square
                draw_image(img_squares, x, y, 7, 7, 2)
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
    for r = 1,self.boardheight do
        for c = 1,self.boardwidth do
            if self.solution[r][c] == '*' and self.board[self.boardheight + r][self.boardwidth + c] ~= '*' then
                return false
            end
        end
    end
    
    return true
end

function State:moveCursor(dir)
    -- The cursor shouldn't move in the top-left quadrant of the board, because
    -- it will always be empty. This has to be checked in up and left moves.

    if dir == 'up' then
        if (self.cursor.y > (self.boardheight + 1) or self.cursor.x > self.boardwidth)
        and self.cursor.y > 0 then
            self.cursor.y = self.cursor.y - 1
        end
    elseif dir == 'down' then
        if self.cursor.y < (self.boardheight * 2) then
            self.cursor.y = self.cursor.y + 1
        end
    elseif dir == 'left' then
        if (self.cursor.x > (self.boardwidth + 1) or self.cursor.y > self.boardheight)
        and self.cursor.x > 0 then
            self.cursor.x = self.cursor.x - 1
        end
    elseif dir == 'right' then
        if self.cursor.x < (self.boardwidth * 2) then
            self.cursor.x = self.cursor.x + 1
        end
    end
end

function State:fillSquare()
    if self.cursor.x > self.boardwidth and self.cursor.y > self.boardheight then
        if self.board[self.cursor.y][self.cursor.x] == '*' then
            -- clear square
            self.board[self.cursor.y][self.cursor.x] = '.'
        else -- '.' or 'x'
            -- fill square
            if self.solution[self.cursor.y - self.boardheight][self.cursor.x - self.boardwidth] == '*' then
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
        viewPicture()
    end
end

function State:eliminateSquare()
    if self.cursor.x > self.boardwidth and self.cursor.y > self.boardheight then
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
        "View Picture",
        "Restart Puzzle",
        "Load Puzzle",
        "Exit"
    }
    
    while true do
        local s = rb.do_menu("Picross", menu, nil, false)
        
        if s == 0 then
            -- resume
            return
        elseif s == 1 then
            -- view picture
            viewPicture()
            return
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
            -- exit
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
    if rb.opendir == nil then
        rb.splash(rb.HZ * 2, "Directory browsing not implemented")
        return false
    end
    rb.opendir('/picross/')
    local files = {}
    repeat
        local file = rb.readdir()
        if file ~= '.' and file ~= '..' then
            table.insert(files, file)
        end
    until not file
    rb.closedir()
    
    table.sort(files)
    
    if #files > 0 then
        local s = rb.do_menu("Puzzles", files, nil, false)
        if s >= 0 and s < #files then
            State:loadFile('/picross/'..files[s+1])
            return true -- return to puzzle screen
        elseif s == -2 then
            -- back button pressed
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
end

function viewPicture()
    rb.lcd_clear_display()
    
    for r = State.boardheight,State.boardheight*2 do
        for c = State.boardwidth,State.boardwidth*2 do
            if State.board[r][c] == '*' then
                rb.lcd_drawpixel(c - State.boardwidth, r - State.boardheight)
            end
        end
    end
    
    rb.lcd_update()
    
    while true do
        local action = rb.get_plugin_action(0)
        
        if action == rb.actions.PLA_EXIT
        or action == rb.actions.PLA_CANCEL
        or action == rb.actions.PLA_SELECT then
            return
        end
    end
end

State:loadDefault()

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
