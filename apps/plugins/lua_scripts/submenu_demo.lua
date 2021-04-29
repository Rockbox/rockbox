--Bilgus 4/2021 Menu with subitems and context demo
require("printsubmenu")

local scrpath = rb.current_path()

local function get_ctx_menu(parent, sel, menu_t, func_t)
    local mt = {"Context menu " .. (menu_t[parent] or "ROOT") ..
            " : " .. menu_t[sel], "Quit", "Action 1", "Action 2"}
    local ft = {false, function() menu_ctx.quit = true return true end}
    return mt, ft
end

local function ITEM_MENU()

    local function flung(i, menu_t, func_t)
        local parent = submenu_get_parent() or 0
        rb.splash(100, "flung " .. (menu_t[parent] or "?"))
    end

    local function foo(i, menu_t, func_t)
        local parent = submenu_get_parent() or 0
        rb.splash(100, "FOO " .. menu_t[parent])
    end

    local function far(i, menu_t, func_t)
        local parent = submenu_get_parent() or 0
        rb.splash(100, "far" .. menu_t[parent])
    end

    return {"Flung", "FOO", "Far"},
            {flung, foo, far}
end

local function USERITEMS()
    local lv = 2
    local mt = {"Item_1", "Item_2", "Item_3"}
    local ft = {}

    local function insert_item(i, name, func) --closure
        submenu_insert(mt, i, name)
        submenu_insert(ft, i, func)
    end

    for i = 1, #mt, 1 do
        ft[i] = submenu_create(lv, ITEM_MENU())
    end

    local function add_new(i, menu_t, func_t)
        local parent, lv = submenu_get_parent(lv - 1)
        local last = #mt
        local name = "Item_" .. tostring(last)
        local func = submenu_create(lv + 1, ITEM_MENU())

        local lv_out, item_out, removed  = submenu_collapse(parent, lv + 1)-- collapse others
        submenu_collapse(parent, lv) -- collapse the parent

        insert_item(last, name, func)

        func_t[parent](parent, menu_t, func_t) -- reopen parent
        menu_ctx.start = i - removed
        return true
    end

    local next = #mt + 1
    insert_item(next, "Add New", add_new)
    return mt, ft
end

local function MAIN_MENU()

    local function go_back(i, m, f)
        local parent = submenu_get_parent() or 0
        if parent > 0 then
            f[parent](parent, m, f)
        else
            menu_ctx.quit = true
        end
        menu_ctx.start = parent - 1
        return true
    end

    local mt =  {
        [1] = "lua Menu Demo",
        [2] = "Items",
        [3] = "Back",
        }

    local ft =  {
        [0]  = go_back, --if user cancels do this function
        [1]  = false, -- shouldn't happen title occupies this slot
        [2]  = submenu_create(1, USERITEMS()),
        [3]  = go_back,
        }
    return mt, ft, get_ctx_menu
end

function ShowMain()
    set_menu(MAIN_MENU())
end

--ShowMainMenu()
ShowMain()
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
--require("print_lua_func")
os.exit(1, "Goodbye")
