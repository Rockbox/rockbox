--core keymap generator
--Bilgus 4/2021

require "buttons"
require("printmenus")

local scrpath = rb.current_path()
local p_settings = {wrap = true, hasheader = true, drawsep = true, justify = "left"}

function exit_now()
    os.exit()
end -- exit_now

function say_msg(message, timeout)
    rb.splash(1, message)
    rb.sleep(timeout * rb.HZ)
end

function say_value(value,message,timeout)
  local message = string.format(message .. "%d", value)
  say_msg(message, timeout)
end

    function cmp_alphanum (op1, op2)
    local type1= type(op1)
    local type2 = type(op2)

    if type1 ~= type2 then
        return type1 < type2
    else
        if type1 == "string" then
            op1 = op1:upper()
            op2 = op2:upper()
        end
        return op1 < op2
    end
     end

function ShowMainMenu() -- we invoke this function every time we want to display the main menu of the script

    local mt =  {
        [1] = "Key Remap",
        [2] = "User Contexts",
        [3] = "",
        [4] = "",
        [5] = "",
        [6] = "",
        [7] = "",
        [8] = "",
        [9] = "",
        [10] = "",
        [11] = "",
        [12] = "",
        [13] = "",
        [14] = "Exit"
        }

    local ft =  {
        [0]  = exit_now, --if user cancels do this function
        [1]  = function(TITLE) return true end, -- shouldn't happen title occupies this slot
        [2]  = function() ShowUserContextMenu(); end,
        [3]  = function() end,
        [4]  = function() end,
        [5]  = function() end,
        [6]  = function() end,
        [7]  = function() end,
        [8]  = function() end,
        [9]  = function() end,
        [10] = function() end,
        [11] = function() end,
        [12] = function() end,
        [13] = function() end,
        [14] = function(EXIT_) return true end
        }

    print_menu(mt, ft, 0, p_settings)
--[[



local s = 0
    mainmenu = {"View", "Edit", "Load", "Save", "Exit"} -- define the items of the menu
    while s == 0 or s == 4 do -- don't exit of program until user selects Exit

    s = rb.do_menu("Key remap", mainmenu, s, false) -- actually tell Rockbox to draw the menu

    -- In the line above: "Test" is the title of the menu, mainmenu is an array with the items
    -- of the menu, nil is a null value that needs to be there, and the last parameter is
    -- whether the theme should be drawn on the menu or not.
    -- the variable s will hold the index of the selected item on the menu.
    -- the index is zero based. This means that the first item is 0, the second one is 1, etc.
    if     s == 0 then rb.splash(rb.HZ, "Not Implemented"); s = -1
    elseif s == 1 then ShowSelector("Choose Context", rb.contexts); s = 4 --edit
    elseif s == 2 then rb.splash(rb.HZ, "Not Implemented"); s = 4
    elseif s == 3 then ShowSelector("Choose Action", rb.actions); s = 4 --save
    elseif s == 4 then os.exit() -- User selected to exit
    elseif s == -2 then os.exit() -- -2 index is returned from do_menu() when user presses the key to exit the menu (on iPods, it's the left key).
                                  -- In this case, user probably wants to exit (or go back to last menu).
    else rb.splash(2 * rb.HZ, "Error! Selected index: " .. s) -- something strange happened. The program shows this message when
                                                              -- the selected item is not on the index from 0 to 3 (in this case), and displays
                                                              -- the selected index. Having this type of error handling is not
                                                              -- required, but it might be nice to have Especially while you're still
                                                              -- developing the plugin.
    end
    end
]]
end

function ShowUserContextMenu() -- we invoke this function every time we want to display the main menu of the script

    local ctx_menu = { expanded = false, start = -1, back = false}

    --forward declarations-- 
    local function get_menu() end
    local function insert_menu(mt, ft) end
    local function get_user_context_menu_items() end
    local function get_user_contexts() end
    local function get_global_menu_items() end
    ------------------------
    local function Context_Expand(i, menu_t, func_t)
        if ctx_menu.expanded then
            ctx_menu.start = i
            i = ctx_menu.expanded
            local ctxitems = get_user_context_menu_items()
            for j = 1, #ctxitems, 1 do
                table.remove(menu_t, i) 
                table.remove(func_t, i)
            end
            i = ctx_menu.start
            if ctx_menu.start == ctx_menu.expanded - 1 then
                ctx_menu.expanded = false
                return false
            elseif ctx_menu.start > ctx_menu.expanded - 1 then
                i = i - #ctxitems
            end
        end

        if not ctx_menu.expanded or (ctx_menu.expanded - 1 ~= ctx_menu.start) then
            local mt, ft = get_user_context_menu_items()

            ctx_menu.start = i
            ctx_menu.expanded = i + 1

            for item, _ in ipairs(mt) do
                i = i + 1
                table.insert(menu_t, i, mt[item]) 
                table.insert(func_t, i, ft[item])
            end
            return true
        end
    end



    get_menu = function ()
        local mt = {[0] = "?", [1] = "User Contexts"}
        local ft =  {[0] = function() ctx_menu.back = true end,
                     [1] = function(TITLE) return true end}

        insert_menu = function (menu_t, func_t, func_fb)
            func_t = func_t or {}
            for item, _ in ipairs(menu_t) do
                table.insert(mt, menu_t[item]) 
                table.insert(ft, func_t[item] or func_fb)
            end
        end

        local c_t = get_user_contexts()
        insert_menu(c_t, nil, function(i, m, f) return Context_Expand(i, m, f) end)
        insert_menu(get_global_menu_items())
        return mt, ft
    end

    local function ctx_loop()
        local mt, ft = get_menu()

        repeat
            _, i = print_menu(mt, ft, ctx_menu.start, p_settings)
        until (not ctx_menu.expanded and ctx_menu.start > 0) or ctx_menu.back == true

        rb.splash(100, "Escaped")
    end

    get_user_contexts = function()
        return {"Context_1", "Context_2", "Context_3"}
    end

    get_global_menu_items = function()
        local function BackFn()
            ctx_menu.back = true
            return ctx_menu.back
        end

        local function AddNew()
            rb.splash(100, " Add New ")
        end

        return {"Add New", "Back"}, {AddNew, BackFn}

    end


    get_user_context_menu_items = function()
        local function captured_ctx(i, menu_t, func_t)
            rb.splash(100, "captured contexts " .. menu_t[ctx_menu.expanded - 1])
        end
        local function mapped_act(i, menu_t, func_t)
            rb.splash(100, "mapped actions " .. menu_t[ctx_menu.expanded - 1])
        end
        local function remove_ctx(i, menu_t, func_t)
            rb.splash(100, "remove context " .. menu_t[ctx_menu.expanded - 1])
        end

        return {"\tCaptured contexts", "\tMapped actions", "\tRemove"},
                {captured_ctx, mapped_act, remove_ctx}
    end

    ctx_loop()

end



function ShowSelector(title, items)
    ret = -1;
    local s = 0
    local i
    local menu = {}

    for key, value in pairs(items) do
       table.insert(menu, key) 
    end
    table.sort(menu, cmp_alphanum)
    table.insert(menu, 1, title or "")

    --print_menu(menu_t, func_t, selected, settings, copy_screen)
    _, i = print_menu(menu, nil, 0, p_settings)
    --i = print_table(menu, #menu, p_settings)

    -- If item was selected follow directory or return filename
    if i > 0 then
    for key, value in pairs(items) do
        if (key == menu[i]) then
            ret = value;
            break
        end
    end
       rb.splash(100, menu[i] .. " Selected " .. ret )
    end
end

rb.sound_settings = nil

ShowMainMenu()

rb.lcd_clear_display()
rb.lcd_update()
local lu = collectgarbage("collect")
local used, allocd, free = rb.mem_stats()
local lu = collectgarbage("count")
local fmt = function(t, v) return string.format("%s: %d Kb\n", t, v /1024) end

-- this is how lua recommends to concat strings rather than ..
local s_t = {}
s_t[1] = "rockbox:\n"
s_t[2] = fmt("Used  ", used)
s_t[3] = fmt("Allocd ", allocd)
s_t[4] = fmt("Free  ", free)
s_t[5] = "\nlua:\n"
s_t[6] = fmt("Used", lu * 1024)
s_t[7] = "\n\nNote that the rockbox used count is a high watermark"
rb.splash_scroller(5 * rb.HZ, table.concat(s_t))
PRINT_LUA_FUNC_DONT_LOAD_MODULES=true
require("print_lua_func")
os.exit(1, "Goodbye")

