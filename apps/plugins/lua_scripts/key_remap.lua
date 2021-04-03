--core keymap generator
--Bilgus 4/2021

require "actions"
require "buttons"
require("printtable")
local scrpath = rb.current_path()


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
local s = 0
    mainmenu = {"View", "Edit", "Load", "Save", "Exit"} -- define the items of the menu
    while s == 0 or s == 4 do -- don't exit of program until user selects Exit

        s = rb.do_menu("Key remap", mainmenu, s, false) -- actually tell Rockbox to draw the menu

        -- In the line above: "Test" is the title of the menu, mainmenu is an array with the items
        -- of the menu, nil is a null value that needs to be there, and the last parameter is
        -- whether the theme should be drawn on the menu or not.
        -- the variable s will hold the index of the selected item on the menu.
        -- the index is zero based. This means that the first item is 0, the second one is 1, etc.
        if     s == 0 then rb.splash(rb.HZ, "Not Implemented"); s = 4
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
end

function ShowSelector(title, items)
        ret = -1;
        local p_settings = {wrap = true, hasheader = true, ifgc = rb.LCD_DEFAULT_FG, ibgc = rb.LCD_DEFAULT_BG, iselc = rb.LCD_DEFAULT_FG}
        local s = 0
        local i
        local menu = {}

        for key, value in pairs(items) do
           table.insert(menu, key) 
        end
        table.sort(menu, cmp_alphanum)
        table.insert(menu, 1, title or "")

        i = print_table(menu, #menu, p_settings)

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


ShowMainMenu()

rb.lcd_clear_display()
rb.lcd_update()

os.exit(1, "Goodbye")

