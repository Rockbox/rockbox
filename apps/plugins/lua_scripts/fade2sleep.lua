--Bilgus 12-2016
--revisited 8-2019
require "actions"
require "buttons"
require "sound"
require "audio"
TIMEOUT = 0

local SOUND_VOLUME = rb.sound_settings.SOUND_VOLUME
rb.sound_settings = nil
package.loaded["sound_defines"] = nil

function say_msg(message, timeout)
    rb.splash(1, message)
    rb.sleep(timeout * rb.HZ)
end

function say_value(value,message,timeout)
  local message = string.format(message .. "%d", value)
  say_msg(message, timeout)
end

function ShowMainMenu() -- we invoke this function every time we want to display the main menu of the script
local s = 0
local mult = 1
local unit = " Minutes"


    while s == 0 or s == 5 do -- don't exit of program until user selects Exit
        if mult < 1 then
            mult = 1
            s = 0
        end
        mainmenu = {"More", mult * 1 .. unit, mult * 5 .. unit, mult * 10 .. unit, mult * 15 .. unit, "Less", "Exit"} -- define the items of the menu
        s = rb.do_menu("Reduce volume + sleep over", mainmenu, s, false) -- actually tell Rockbox to draw the menu

        -- In the line above: "Test" is the title of the menu, mainmenu is an array with the items
        -- of the menu, nil is a null value that needs to be there, and the last parameter is
        -- whether the theme should be drawn on the menu or not.
        -- the variable s will hold the index of the selected item on the menu.
        -- the index is zero based. This means that the first item is 0, the second one is 1, etc.
        if     s == 0 then mult = mult + 1
        elseif s == 1 then TIMEOUT = mult
        elseif s == 2 then TIMEOUT = mult * 5
        elseif s == 3 then TIMEOUT = mult * 10
        elseif s == 4 then TIMEOUT = mult * 15
        elseif s == 5 then mult = mult - 1 -- User selected to exit
        elseif s == 6 then os.exit() -- User selected to exit
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

ShowMainMenu()
rb.set_sleeptimer_duration(TIMEOUT)
rb.lcd_clear_display()
rb.lcd_update()

local volume = rb.sound_current(SOUND_VOLUME)
local vol_min = rb.sound_min(SOUND_VOLUME)
local volsteps = -(vol_min - volume)
local seconds = (TIMEOUT * 60) / volsteps
local sec_left = (TIMEOUT * 60)
local hb = 0
local action = rb.get_action(rb.contexts.CONTEXT_STD, 0)
    if rb.audio_status() == 1 then
        while ((volume > vol_min) and (action ~= rb.actions.ACTION_STD_CANCEL)) do
            rb.lcd_clear_display()
            say_value(volume,sec_left .. " Sec, Volume: ", 1)
            local i = seconds * 2
            while ((i > 0) and (action ~= rb.actions.ACTION_STD_CANCEL)) do
                i = i - 1
                rb.lcd_drawline(hb, 1, hb, 1)
                rb.lcd_update()
                if hb >= rb.LCD_WIDTH then
                    hb = 0
                    rb.lcd_clear_display()
                    say_value(volume,sec_left .. " Sec, Volume: ", 1)
                end
                hb = hb + 1
                rb.sleep(rb.HZ / 2)
                action = rb.get_action(rb.contexts.CONTEXT_STD, 0)
                rb.yield()
            end
            volume = volume - 1
            rb.sound_set(SOUND_VOLUME, volume);
            sec_left = sec_left - seconds

        end
        rb.audio_stop()
        rb.lcd_clear_display()
        rb.lcd_update()

        os.exit(1, "Playback Stopped")

    else
        rb.lcd_clear_display()
        rb.lcd_update()

        os.exit(2, "Nothing is playing")
    end
