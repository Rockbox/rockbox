-- tagnav.lua BILGUS 2018
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
 * Copyright (C) 2018 William Wilgus
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

--local scrpath = rb.current_path()"

--package.path = scrpath .. "/?.lua;" .. package.path --add lua_scripts directory to path
require("printmenus")
require("printtable")
require("dbgettags")

local _print = require("print")

rb.actions = nil
package.loaded["actions"] = nil

local sERROROPENING   = "Error opening"
local sERRORMENUENTRY = "Error finding menu entry"

local sBLANKLINE = "##sBLANKLINE##"
local sDEFAULTMENU = "customfilter"

local sFILEOUT       = "/.rockbox/tagnavi_custom.config"
local sFILEHEADER    = "#! rockbox/tagbrowser/2.0"
local sMENUSTART     = "%menu_start \"custom\" \"Database\""
local sMENUTITLE     = "title = \"fmt_title\""
local sRELOADSEARCH  = "^\"Reload\.\.\.\"%s*%->.+"
local sRELOADMENU    = "\"Reload...\" -> %reload"

local TAG_ARTIST, TAG_ALBARTIST, TAG_ALBUM, TAG_GENRE, TAG_COMPOSER = 1, 2, 3, 4, 5
local ts_TAGTYPE = {"Artist", "AlbumArtist", "Album", "Genre", "Composer"}
local ts_DBPATH  = {"database_0.tcd", "database_7.tcd", "database_1.tcd", "database_2.tcd", "database_5.tcd"}

local COND_OR, COND_AND, COND_NOR, COND_NAND = 1, 2, 3, 4
local ts_CONDITIONALS = {"OR", "AND", "!, OR", "!, AND"}
local ts_CONDSYMBOLS    = {"|", "&", "|", "&"}

local ts_YESNO        = {"", "Yes", "No"}
local s_OVERWRITE     = "Overwrite"
local s_EXISTS        = "Exists"


local function question(tInquiry, start)
    settings = {}
    settings.justify = "center"
    settings.wrap = true
    settings.msel = false
    settings.hasheader = true
    settings.co_routine = nil
    settings.curpos = start or 1
    local sel = print_table(tInquiry, #tInquiry, settings)
    return sel
end

local function find_linepos(t_lines, search, startline)
    startline = startline or 1

    for i = startline, #t_lines do
        if string.match (t_lines[i], search) then 
            return i
        end
    end

    return -1
end

local function replacelines(t_lines, search, replace, startline)
    startline = startline or 1
    repcount = 0
    for i = startline, #t_lines do
        if string.match (t_lines[i], search) then
            t_lines[i] = replace
            repcount = repcount + 1
        end
    end
    return repcount
end

local function replaceemptylines(t_lines, replace, startline)
    startline = startline or 1
    replace = replace or nil
    repcount = 0
    for i = startline, #t_lines do
        if t_lines[i] == "" then
            t_lines[i] = replace
            repcount = repcount + 1
        end
    end
    return repcount
end

local function checkexistingmenu(t_lines, menuname)
    local pos = find_linepos(t_lines, "^\"" .. menuname .. "\"%s*%->.+")
    local sel = 0
    if pos > 0 then
        ts_YESNO[1] = menuname .. " " .. s_EXISTS .. ", " ..  s_OVERWRITE .. "?"
        sel = question(ts_YESNO, 3)
        if sel == 3 then
            pos = nil
        elseif sel < 2 then
            pos = 0
        end
    else
        pos = nil
    end
    return pos
end

local function savedata(filename, ts_tags, cond, menuname)
        menuname = menuname or sDEFAULTMENU

        local lines = {}
        local curline = 0
        local function lines_next(str, pos)
            pos = pos or #lines + 1
            lines[pos] = str or ""
            curline = pos
        end

        local function lines_append(str, pos)
            pos = pos or curline or #lines
            lines[pos] = lines[pos] .. str or ""
        end

        if rb.file_exists(filename) ~= true then
            lines_next(sFILEHEADER)
            lines_next("#")
            lines_next("# CUSTOM MENU")
            lines_next(sMENUSTART)
        else
            local file = io.open(filename, "r") -- read
            if not file then
                rb.splash(rb.HZ, "Error opening" .. " " .. filename)
                return
            end

            -- copy all existing lines to table we will overwrite existing file
            for line in file:lines() do
                lines_next(line)
            end
            file:close()
        end

        local menupos = find_linepos(lines, sMENUSTART)
        if menupos < 1 then
            rb.splash(rb.HZ, sERRORMENUENTRY)
            return
        end

        replaceemptylines(lines, sBLANKLINE, menupos)

        -- remove reload menu, we will add it at the end
        replacelines(lines, sRELOADSEARCH, sBLANKLINE)

        local existmenupos = checkexistingmenu(lines, menuname)
        if existmenupos and existmenupos < 1 then return end -- user canceled

        local lastcond = ""
        local n_cond = COND_OR
        local tags, tagtype
        local tagpos = 0

        local function buildtag(e_tagtype)
            if ts_tags[e_tagtype] then
                tagpos = tagpos + 1
                n_cond = (cond[e_tagtype] or COND_OR)
                if tagpos > 1 then
                    lines_append(" " .. ts_CONDSYMBOLS[n_cond])
                end
                tags = ts_tags[e_tagtype]
                tagtype = string.lower(ts_TAGTYPE[e_tagtype])

                if n_cond <= COND_AND then
                    lines_append(" " .. tagtype)
                    lines_append(" @ \"".. table.concat(tags, "|")  .. "\"")
                else
                    for k = 1, #tags do
                        lines_append(" " .. tagtype)
                        lines_append(" !~ \"".. tags[k] .. "\"")
                        if k < #tags then lines_append(" &") end
                    end
                end
            end        
        end

        if ts_tags[TAG_ARTIST] or ts_tags[TAG_ALBARTIST] or ts_tags[TAG_ALBUM] or
           ts_tags[TAG_GENRE] or ts_tags[TAG_COMPOSER] then

            lines_next("\"" .. menuname .. "\" -> " .. sMENUTITLE .. " ?", existmenupos)

            buildtag(TAG_ARTIST)
            buildtag(TAG_ALBARTIST)
            buildtag(TAG_ALBUM)
            buildtag(TAG_GENRE)
            buildtag(TAG_COMPOSER)
            -- add reload menu
            lines_next(sRELOADMENU .. "\n")

        else
            rb.splash(rb.HZ, "Nothing to save")
            return
        end

        local file = io.open(filename, "w+") -- overwrite
        if not file then
            rb.splash(rb.HZ, "Error writing " .. filename)
            return
        end

        for i = 1, #lines do 
            if lines[i] and lines[i] ~= sBLANKLINE then
                file:write(lines[i], "\n")
            end
        end

        file:close()
end

-- uses print_table to display database tags
local function print_tags(ftable, settings, t_selected)
    if not s_cond then s_sep = "|" end
    ftable = ftable or {}

    if t_selected then
        for k = 1, #t_selected do
            ftable[t_selected[k]] = ftable[t_selected[k]] .. "\0"
        end
    end
    rb.lcd_clear_display()
    _print.clear()

    if not settings then
        settings = {}
        settings.justify = "left"
        settings.wrap = true
        settings.msel = true
    end

    settings.hasheader = true
    settings.co_routine = nil

    local sel = print_table(ftable, #ftable, settings)

    --_lcd:splashf(rb.HZ * 2, "%d items {%s}", #sel, table.concat(sel, ", "))
    local selected = {}
    local str = ""
    for k = 1,#sel do
        str = ftable[sel[k]] or ""
        selected[#selected + 1] = string.sub(str, 1, -2) -- REMOVE \0
    end

    ftable = nil

    if #sel == 0 then
        return nil, nil
    end

    return sel, selected
end -- print_tags

-- uses print_table to display a menu
function main_menu()
    local menuname = sDEFAULTMENU
    local t_tags = {}
    local ts_tags = {}
    local cond = {}
    local sel = {}
    local mt =  {
                [1] = "TagNav Customizer",
                [2] = "", --ts_CONDITIONALS[cond[TAG_ARTIST] or COND_OR],
                [3] = ts_TAGTYPE[TAG_ARTIST],
                [4] = ts_CONDITIONALS[cond[TAG_ALBARTIST] or COND_OR],
                [5] = ts_TAGTYPE[TAG_ALBARTIST],
                [6] = ts_CONDITIONALS[cond[TAG_ALBUM] or COND_OR],
                [7] = ts_TAGTYPE[TAG_ALBUM],
                [8] = ts_CONDITIONALS[cond[TAG_GENRE] or COND_OR],
                [9] = ts_TAGTYPE[TAG_GENRE],
                [10] = ts_CONDITIONALS[cond[TAG_COMPOSER] or COND_OR],
                [11] = ts_TAGTYPE[TAG_COMPOSER],
                [12] = "",
                [13] = "Save to Tagnav",
                [14] = "Exit"
                }

    local function sel_cond(item, item_mt)
        cond[item] = cond[item] or 1
        cond[item] = cond[item] + 1
        if cond[item] > #ts_CONDITIONALS then cond[item] = 1 end
        mt[item_mt] = ts_CONDITIONALS[cond[item]]
    end

    local function sel_tag(item, item_mt, t_tags)
        t_tags = get_tags(rb.ROCKBOX_DIR .. "/" .. ts_DBPATH[item], ts_TAGTYPE[item], t_tags)
        sel[item], ts_tags[item] = print_tags(t_tags, nil, sel[item])
        if ts_tags[item] then
            mt[item_mt] = ts_TAGTYPE[item] .. " [" .. #sel[item] .. "]"
        else
            mt[item_mt] = ts_TAGTYPE[item]
        end
    end

    local ft =  {
                [0] = exit_now, --if user cancels do this function
                [1] = function(TITLE) return true end, -- shouldn't happen title occupies this slot
                [2]  = function(ARTCOND)
                            sel_cond(TAG_ARTIST, ARTCOND)
                        end,
                [3]  = function(ART)
                            sel_tag(TAG_ARTIST, ART, t_tags)
                        end,
                [4]  = function(ALBARTCOND)
                            sel_cond(TAG_ALBARTIST, ALBARTCOND)
                        end,
                [5]  = function(ALBART)
                            sel_tag(TAG_ALBARTIST, ALBART, t_tags)                           
                        end,
                [6]  = function(ALBCOND)
                            sel_cond(TAG_ALBUM, ALBCOND)
                        end,
                [7]  = function(ALB)
                            sel_tag(TAG_ALBUM, ALB, t_tags)
                        end,
                [8]  = function(GENRECOND)
                            sel_cond(TAG_GENRE, GENRECOND)
                        end,
                [9]  = function(GENRE)
                            sel_tag(TAG_GENRE, GENRE, t_tags)                           
                        end,
                [10]  = function(COMPCOND)
                            sel_cond(TAG_COMPOSER, COMPCOND)
                        end,
                [11]  = function(COMP)
                            sel_tag(TAG_COMPOSER, COMP, t_tags)
                        end,
                [12]  = function(B) mt[B] = "TNC Bilgus 2018" end,
                [13]  = function(SAVET)
                            menuname = menuname or sDEFAULTMENU
                            menuname = rb.kbd_input(menuname)
                            menuname = string.match(menuname or "", "%w+")
                            if menuname == "" then menuname = nil end
                            if menuname then
                                savedata(sFILEOUT, ts_tags, cond, menuname)
                            end
                        end,
                [14] = function(EXIT_) return true end
                }

    print_menu(mt, ft, 2) --start at item 2

end

function exit_now()
    rb.lcd_update()
    os.exit()
end -- exit_now

main_menu()
exit_now()
