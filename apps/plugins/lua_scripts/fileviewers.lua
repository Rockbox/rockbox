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
 * Copyright (C) 2017 William Wilgus
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
if ... == nil then rb.splash(rb.HZ * 3, "use 'require'") end
require("printtable")
local _clr = require("color")
local _lcd = require("lcd")
local _print = require("print")
local _timer = require("timer")

require("actions")
local CANCEL_BUTTON = rb.actions.PLA_CANCEL
--------------------------------------------------------------------------------
-- builds an index of byte position of every line at each bufsz increment
-- in filename; bufsz == 1 would be every line; saves to filename.ext.idx_ext
-- lnbyte should be nil for text files and number of bytes per line for binary
local function build_file_index(filename, idx_ext, bufsz, lnbyte)

    if not filename then return end
    local file = io.open('/' .. filename, "r") --read
    if not file then _lcd:splashf(100, "Can't open %s", filename) return end
    local fsz = file:seek("end")
    local fsz_kb = fsz / 1024
    local count
    local ltable = {0} --first index is the beginning of the file
    local timer = _timer()
    local fread
    _lcd:splashf(100, "Indexing file %d Kb", (fsz / 1024))

    if lnbyte then
        fread = function(f) return f:read(lnbyte) end
    else
        lnbyte = -1
        fread = function(f) return f:read("*l") end
    end

    file:seek("set", 0)
    for i = 1, fsz do
        if i % bufsz == 0 then
            local loc = file:seek()
            ltable[#ltable + 1] = loc
            _lcd:splashf(1, "Parsing %d of %d Kb", loc / 1024, fsz_kb)
        end
        if rb.get_plugin_action(0) == CANCEL_BUTTON then
            return
        end
        if not fread(file) then
            count = i
            break
        end
    end

    local fileidx = io.open('/' .. filename .. idx_ext, "w+") -- write/erase
    if fileidx then
        fileidx:write(fsz .. "\n")
        fileidx:write(count .. "\n")
        fileidx:write(bufsz .. "\n")
        fileidx:write(lnbyte .. "\n")
        fileidx:write(table.concat(ltable, "\n"))
        fileidx:close()
        _lcd:splashf(100, "Finished in %d seconds", timer.stop() / rb.HZ)
        collectgarbage("collect")
    else
        error("unable to save index file")
    end
end -- build_file_index
--------------------------------------------------------------------------------

--- returns size of original file, total lines buffersize, and table filled
-- with line offsets in index file -> filename
local function load_index_file(filename)
    local filesz, count, bufsz, lnbyte
    local ltable
    local fileidx = io.open('/' .. filename, "r") --read
    if fileidx then
        local idx = -3
        ltable = {}
        fileidx:seek("set", 0)
        for line in fileidx:lines() do
            if idx == -3 then
                filesz = tonumber(line)
            elseif idx == -2 then
                count = tonumber(line)
            elseif idx == -1 then
                bufsz = tonumber(line)
            elseif idx == 0 then
                lnbyte = tonumber(line)
            else
                ltable[idx] = tonumber(line)
            end
            idx = idx + 1
        end
        fileidx:close()
    end
    return lnbyte, filesz, count, bufsz, ltable
end -- load_index_file
--------------------------------------------------------------------------------

-- creates a fixed index with fixed line lengths, perfect for viewing hex files
-- not so great for reading text files but works as a fallback
local function load_fixed_index(bytesperline, filesz, bufsz)
    local lnbyte = bytesperline
    local count = (filesz + lnbyte - 1) / lnbyte + 1
    local idx_t = {} -- build index
    for i = 0, filesz, bufsz do
        idx_t[#idx_t + 1] = lnbyte * i
    end
    return lnbyte, filesz, count, bufsz, idx_t
end -- load_fixed_index
--------------------------------------------------------------------------------

-- uses print_table to display a whole file
function print_file(filename, maxlinelen, settings)

    if not filename then return end
    local file = io.open('/' .. filename or "", "r") --read
    if not file then _lcd:splashf(100, "Can't open %s", filename) return end
    maxlinelen = 33
    local hstr = filename
    local ftable = {}
    table.insert(ftable, 1, hstr)

    local tline = #ftable + 1
    local remln = maxlinelen
    local posln = 1

    for line in file:lines() do
        if line then
            if maxlinelen then
                if line == "" then
                    ftable[tline] = ftable[tline] or ""
                    tline = tline + 1
                    remln = maxlinelen
                else
                    line = line:match("%w.+") or ""
                end
                local linelen = line:len()
                while linelen > 0 do

                    local fsp = line:find("%s", posln + remln - 5) or 0x0
                    fsp = fsp - (posln + remln)
                    if fsp >= 0 then
                        local fspr = fsp
                        fsp = line:find("%s", posln + remln) or linelen
                        fsp = fsp - (posln + remln)
                        if math.abs(fspr) < fsp then fsp = fspr end
                    end
                    if fsp > 5 or fsp < -5 then fsp = 0 end

                    local str = line:sub(posln, posln + remln + fsp)
                    local slen = str:len()
                    ftable[tline] = ftable[tline] or ""
                    ftable[tline] = ftable[tline] .. str
                    linelen = linelen - slen
                    if linelen > 0 then
                        tline = tline + 1
                        posln = posln + slen
                        remln = maxlinelen
                        --loop continues
                    else
                        ftable[tline] = ftable[tline] .. " "
                        remln = maxlinelen - slen
                        posln = 1
                        --loop ends
                    end

                end
            else
                ftable[#ftable + 1] = line
            end


        end
    end

    file:close()

    _lcd:clear()
    _print.clear()

    if not settings then
        settings = {}
        settings.justify = "center"
        settings.wrap = true
        settings.msel = true
    end
    settings.hasheader = true
    settings.co_routine = nil
    settings.ovfl = "manual"

    local sel =
        print_table(ftable, #ftable, settings)

    _lcd:splashf(rb.HZ * 2, "%d items {%s}", #sel, table.concat(sel, ", "))
    ftable = nil
end -- print_file
--------------------------------------------------------------------------------

-- uses print_table to display a portion of a file
function print_file_increment(filename, settings)

    if not filename then return end
    local file = io.open('/' .. filename, "r") --read
    if not file then _lcd:splashf(100, "Can't open %s", filename) return end
    local fsz = file:seek("end")
    local bsz = 1023
    --if small file do it the easier way and load whole file to table
    if fsz < 60 * 1024 then
        file:close()
        print_file(filename, settings)
        return
    end

    local ext = ".idx"
    local lnbyte, filesz, count, bufsz, idx_t = load_index_file(filename .. ext)

    if not idx_t or fsz ~= filesz then -- build file index
        build_file_index(filename, ext, bsz)
        lnbyte, filesz, count, bufsz, idx_t = load_index_file(filename .. ext)
    end

    -- if invalid or user canceled creation fallback to a fixed index
    if not idx_t or fsz ~= filesz or count <= 0 then
        _lcd:splashf(rb.HZ * 5, "Unable to read file index %s", filename .. ext)
        lnbyte, filesz, count, bufsz, idx_t = load_fixed_index(32, fsz, bsz)
    end

    if not idx_t or fsz ~= filesz or count <= 0 then
        _lcd:splashf(rb.HZ * 5, "Unable to load file %s", filename)
        return
    end

    local hstr = filename
    local file_t = setmetatable({},{__mode = "kv"}) --weak keys and values
    -- this allows them to be garbage collected as space is needed
    -- rebuilds when needed
    local ovf = 0
    local lpos = 1
    local timer = _timer()
    file:seek("set", 0)

    function print_co()
        while true do
            collectgarbage("step")
            file_t[1] = hstr --position 1 is ALWAYS header/title

            for i = 1, bufsz + ovf do
                file_t[lpos + i] = file:read ("*l")
            end
                ovf = 0
                lpos = lpos + bufsz

                local bpos = coroutine.yield()

                if bpos <= lpos then -- roll over or scroll up
                    bpos = (bpos - bufsz) + bpos % bufsz
                    timer:check(true)
                end

                lpos = bpos - bpos % bufsz

                if lpos < 1 then
                    lpos = 1
                elseif lpos > count - bufsz then -- partial fill
                    ovf = count - bufsz - lpos
                end
                --get position in file of the nearest indexed line
                file:seek("set", idx_t[bpos / bufsz + 1])

                -- on really large files if it has been more than 10 minutes
                -- since the user scrolled up the screen wipe out the prior
                -- items to free memory
                if lpos % 5000 == 0 and timer:check() > rb.HZ * 600 then
                    for i = 1, lpos - 100 do
                        file_t[i] = nil
                    end
                end

        end
    end

    co = coroutine.create(print_co)
    _lcd:clear()
    _print.clear()

    if not settings then
        settings = {}
        settings.justify = "center"
        settings.wrap = true
    end
    settings.hasheader = true
    settings.co_routine = co
    settings.msel = false
    settings.ovfl = "manual"

    table.insert(file_t, 1, hstr) --position 1 is header/title
    local sel =
    print_table(file_t, count, settings)
    file:close()
    idx_t = nil
    file_t = nil
    return sel
end --print_file_increment
--------------------------------------------------------------------------------
function print_file_hex(filename, bytesperline, settings)

    if not filename then return end
    local file = io.open('/' .. filename, "r") --read
    if not file then _lcd:splashf(100, "Can't open %s", filename) return end
    local hstr = filename
    local bpl = bytesperline
    local fsz = file:seek("end")
--[[
    local filesz = file:seek("end")
    local bufsz = 1023
    local lnbyte = bytesperline
    local count = (filesz + lnbyte - 1) / lnbyte + 1

    local idx_t = {} -- build index
    for i = 0, filesz, bufsz do
        idx_t[#idx_t + 1] = lnbyte * i
    end]]

    local lnbyte, filesz, count, bufsz, idx_t = load_fixed_index(bpl, fsz, 1023)

    local file_t = setmetatable({},{__mode = "kv"}) --weak keys and values
    -- this allows them to be garbage collected as space is needed
    -- rebuilds when needed
    local ovf = 0
    local lpos = 1
    local timer = _timer()
    file:seek("set", 0)

    function hex_co()
        while true do
            collectgarbage("step")
            file_t[1] = hstr --position 1 is ALWAYS header/title

            for i = 1, bufsz + ovf do
                local pos = file:seek()
                local s = file:read (lnbyte)
                if not s then -- EOF
                    file_t[lpos + i] = ""
                    break;
                end
                local s_len = s:len()

                if s_len > 0 then
                    local fmt = "0x%04X: " .. string.rep("%02X ", s_len)
                    local schrs = "     " .. s:gsub("(%c)", " . ")
                    file_t[lpos + i] = string.format(fmt, pos, s:byte(1, s_len)) ..
                                       schrs
                else
                    file_t[lpos + i] = string.format("0x%04X: ", pos)
                end
            end
                ovf = 0
                lpos = lpos + bufsz

                local bpos = coroutine.yield()

                if bpos < lpos then -- roll over or scroll up
                    bpos = (bpos - bufsz) + bpos % bufsz
                    timer:check(true)
                end

                lpos = bpos - bpos % bufsz

                if lpos < 1 then
                    lpos = 1
                elseif lpos > count - bufsz then -- partial fill
                    ovf = count - bufsz - lpos
                end
                --get position in file of the nearest indexed line
                file:seek("set", idx_t[bpos / bufsz + 1])

                -- on really large files if it has been more than 10 minutes
                -- since the user scrolled up the screen wipe out the prior
                -- items to free memory
                if lpos % 10000 == 0 and timer:check() > rb.HZ * 600 then
                    for i = 1, lpos - 100 do
                        file_t[i] = nil
                    end
                end

        end
    end

    co = coroutine.create(hex_co)

    local function repl(char)
            local ret = ""
            if char:sub(1,2) == "0x" then
                return string.format("%dd:", tonumber(char:sub(3, -2), 16))
            else
                return string.format("%03d ", tonumber(char, 16))
            end
    end


    _lcd:clear()
    _print.clear()

    local sel, start, vcur = 1
    table.insert(file_t, 1, hstr) --position 1 is header/title

    if not settings then
        settings = {}
        settings.justify = "left"
        settings.wrap    = true
        settings.msel    = false
        settings.hfgc    = _clr.set( 0, 000, 000, 000)
        settings.hbgc    = _clr.set(-1, 255, 255, 255)
        settings.ifgc    = _clr.set(-1, 255, 255, 255)
        settings.ibgc    = _clr.set( 0, 000, 000, 000)
        settings.iselc   = _clr.set( 1, 000, 200, 100)
    end

    settings.hasheader = true
    settings.co_routine = co
    settings.start = start
    settings.curpos = vcur
    settings.ovfl = "manual"

    while sel > 0 do
        settings.start = start
        settings.curpos = vcur

        sel, start, vcur = print_table(file_t, count, settings)

        if sel > 1 and file_t[sel] then -- flips between hex and decimal
            local s = file_t[sel]
            if s:sub(-1) == "\b" then
                file_t[sel] = nil
                ovf = -(bufsz - 1)
                coroutine.resume(co, sel) --rebuild this item
            else
                s = s:gsub("(0x%x+:)", repl) .. "\b"
                file_t[sel] = s:gsub("(%x%x%s)", repl) .. "\b"
            end
        end
    end

    file:close()
    idx_t = nil
    file_t = nil
    return sel
end -- print_file_hex
--------------------------------------------------------------------------------
