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
 * Copyright (C) 2021 William Wilgus
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
if not rb.lcd_framebuffer then rb.splash(rb.HZ, "No Support!") return nil end
--GLOBALS
menu_ctx = {}
submenu_insert = table.insert
submenu_remove = table.remove

local last_ctx = false
local p_settings
local function empty_fn() end

--[[root menu tables
expanded menus get inserted / removed
and context menus replace them but unless you
want a new root menu they are never overwritten
func_t functions get 3 variables passed by the menu_system
func_t[i] =function sample(i, menu_t, func_t]
this function gets run on user selection
for every function in func_t:
'i' is the selected item
'menu_t' is the current strings table
'func_t' is the current function table

menu_t[i] will returnm the text of the item user selected and
func_t[i] will return the function we are currently in
]]

menu_t = {}
func_t = {}

require("printmenus")

local BUTTON = require("menubuttons")
local last_sel = 0

local function display_context_menu() end -- forward declaration

local function dpad(x, xi, xir, y, yi, yir, timeout, overflow, selected)
    local scroll_is_fixed = overflow ~= "manual"
    if timeout == nil then timeout = -1 end
    local cancel, select = 0, 0
    local x_chg, y_chg = 0, 0
    local button
    while true do
        button = rb.get_plugin_action(timeout)

        if button == BUTTON.CANCEL then
            cancel = 1
            break;
        elseif button == BUTTON.EXIT then
            cancel = 1
            break;
        elseif button == BUTTON.SEL then
            last_sel = 1
            timeout = timeout + 1
        elseif button == BUTTON.SELR then
            last_sel = 2
            if display_context_menu(selected or -1) == true then
                select = 1
                break;
            end
            timeout = timeout + 1
        elseif button == BUTTON.SELREL then
            if last_sel == 1 then
                select = 1
            end
            last_sel = 0
            timeout = timeout + 1
        elseif button == BUTTON.LEFT then
            x_chg = x_chg - xi
            if scroll_is_fixed then
                cancel = 1
                break;
            end
        elseif button == BUTTON.LEFTR then
            x_chg = x_chg - xir
        elseif button == BUTTON.RIGHT then
            x_chg = x_chg + xi
            if scroll_is_fixed then
                select = 1
                timeout = timeout + 1
            end
        elseif button == BUTTON.RIGHTR then
            x_chg = x_chg + xir
        elseif button == BUTTON.UP then
            y_chg = y_chg + yi
        elseif button == BUTTON.UPR then
            y_chg = y_chg + yir
        elseif button == BUTTON.DOWN then
            y_chg = y_chg - yi
        elseif button == BUTTON.DOWNR then
            y_chg = y_chg - yir
        elseif timeout >= 0 then--and rb.button_queue_count() < 1 then
            break;
        end

        if x_chg ~= 0 or y_chg ~= 0 then
            timeout = timeout + 1
        end
    end

    x = x + x_chg
    y = y + y_chg

    return cancel, select, x_chg, x, y_chg, y, 0xffff
end -- dpad

local function ctx_loop()
    local loopfn = ctx_loop
    ctx_loop = empty_fn() --prevent another execution
    local mt, ft = get_menu()
    local i
    repeat
        if menu_ctx.update then mt, ft = get_menu(); menu_ctx.update = false end
        _, i = print_menu(mt, ft, menu_ctx.start, p_settings)
    until menu_ctx.quit

    ctx_loop = loopfn --restore for another run
end

--[[ push_ctx() save surrent menu and load another ]]
local function push_ctx(new_getmenu)
    last_ctx = last_ctx or {}
    submenu_insert(last_ctx, menu_ctx)
    menu_ctx.getmenu = get_menu
    menu_ctx.settings = p_settings
    --menu_ctx is a new variable after this point
    submenu_set_defaults()
    menu_ctx.update = true
    if type(new_getmenu) == 'function' then
        get_menu = new_getmenu
    end
end

--[[ pop_ctx() restore last menu ]]
local function pop_ctx()
    menu_ctx = submenu_remove(last_ctx)
    if menu_ctx then
        get_menu = menu_ctx.getmenu
        p_settings = menu_ctx.settings
        if menu_ctx.restorefn then
            menu_ctx.restorefn(menu_t, func_t)
            menu_ctx.restorefn = nil
        end
        menu_ctx.getmenu = nil
        menu_ctx.settings = nil
        menu_ctx.update = true
        return true
    end
end

--[[ display_context_menu_internal() supplies a new get_menu function that returns
     the context menu 'user_context_fn' supplied by set_menu()
     this menu is immediately displayed and when finished will
     automatically restore the last menu
]]
local function display_context_menu_internal(sel)
        if sel <= 0 or not menu_ctx.user_context_fn then return false end
        local parent = submenu_get_parent() or 0
        local user_context_fn = menu_ctx.user_context_fn

        local function display_context_menu(i, menu_t, func_t)
            local function new_getmenu()
                local mt, ft = user_context_fn(parent, i, menu_t, func_t)
                ft[0] = pop_ctx --set back fn
                return mt, ft
            end
            push_ctx(new_getmenu)
            return true
        end

        --save the current function in closure restore_fn for later
        local funct = func_t[sel]
        local function restore_fn(mt, ft)
            ft[sel] = funct
            menu_ctx.start = sel - 1
        end

        menu_ctx.restorefn = restore_fn
        -- insert into the current fn table so it gets execd by the menu
        func_t[sel] = display_context_menu

        return true
end

--[[ submenu_get_parent() gets the parent of the top most level
     if lv is supplied it instead gets the parent of that level ]]
function submenu_get_parent(lv)
    lv = lv or #menu_ctx.collapse_fn or 1
    collectgarbage("step")
    local t = menu_ctx.collapse_fn[lv] or {}
    return t[2] or -1, lv
end

--[[ submenu_collapse() collapses submenu till level or ROOT is reached ]]
function submenu_collapse(parent, lv)
    local lv_out, menu_sz = 0, 0
    local item_out = -1
    local items_removed = 0
    if lv <= #menu_ctx.collapse_fn then
        repeat
            local collapse_fn = submenu_remove(menu_ctx.collapse_fn)
            if collapse_fn then
                lv_out, item_out, menu_sz = collapse_fn[1](parent, menu_t, func_t)
                items_removed = items_removed + menu_sz
            end

        until not collapse_fn or lv >= lv_out
    end
    return lv_out, item_out, items_removed
end

--[[ submenu_create() supply level of submenu > 0, ROOT is lv 0
    supply menu strings table and function table
    closure returned run this function to expand the menu
]]
function submenu_create(lv, mt, ft)
    if lv < 1 then error("Level < 1") end
    if type(mt) ~= 'table' or type(ft) ~= 'table' then
        error("mt and ft must be tables")
    end

    -- everything in lua is 1 based menu level is no exception
    local lv_tab = string.rep ("\t", lv)
    local function submenu_closure(i, m, f)
        menu_ctx.lv = lv
        local lv_out, menusz_out, start_item
        local item_in, item_out = i, i
        if lv <= #menu_ctx.collapse_fn then --something else expanded??
            repeat
                local collapse_fn = submenu_remove(menu_ctx.collapse_fn)
                if collapse_fn then
                    lv_out, item_out, menusz_out = collapse_fn[1](i, m, f)
                    -- if the item i is below this menu, it needs to shift too
                    if item_in > item_out then i = i - (menusz_out) end
                end
            until not collapse_fn or lv >= lv_out
            menu_ctx.start = i
            if item_out == item_in then return end
        end

        local menu_sz = #mt
        menu_ctx.start = i
        start_item = i
        menu_ctx.update = true
        for item, _ in ipairs(mt) do
            i = i + 1
            submenu_insert(m, i, lv_tab .. mt[item])
            submenu_insert(f, i, ft[item])
        end

        local function collapse_closure(i, m, f)
            --creates a closure around lv, start_item and menu_sz
            for j = 1, menu_sz, 1 do
                submenu_remove(m, start_item + 1)
                submenu_remove(f, start_item + 1)
            end
            return lv, start_item, menu_sz
        end

        submenu_insert(menu_ctx.collapse_fn, lv, {collapse_closure, start_item})
        return true
    end

    return submenu_closure
end

--
function submenu_set_defaults(settings, ctx)
    p_settings = settings or {wrap = true, hasheader = true, justify = "left", dpad_fn = dpad}
    menu_ctx = ctx or {collapse_fn = {}, lv = 0, update = false, start = 1}
end

--[[ get_menu() returns the ROOT string and fn tables]]
function get_menu()
    return menu_t, func_t
end

--[[ set_menu() set your menu and the menu has now been entered ]]
function set_menu(mt, ft, user_context_fn, settings)

    submenu_set_defaults(settings)
    if type(user_context_fn) == 'function' then
        display_context_menu = display_context_menu_internal
        menu_ctx.user_context_fn = user_context_fn
    else
        display_context_menu = empty_fn
        menu_ctx.user_context_fn = false
    end
    p_settings = settings or p_settings
    menu_t, func_t = mt, ft
    ctx_loop()
end
