--menu core settings loaded from rockbox user settings
--Bilgus 4/2021

local function get_core_settings()
    local rbs_is_loaded = (package.loaded.rbsettings ~= nil)
    local s_is_loaded = (package.loaded.settings ~= nil)

    require("rbsettings")
    require("settings")
    rb.metadata = nil -- remove track metadata settings

    local rb_settings = rb.settings.dump('global_settings', "system")
    local color_table = {}
    local talk_table = {}
    local list_settings_table = {}
    local list_settings = "cursor_style|show_icons|statusbar|scrollbar|scrollbar_width|list_separator_height|backdrop_file|"
    for key, value in pairs(rb_settings) do
            key = key or ""
            if (key:find("color")) then
                    color_table[key]=value
            elseif (key:find("talk")) then
                    talk_table[key]=value
            elseif (list_settings:find(key)) then
                    list_settings_table[key]=value
            end
    end

    if not s_is_loaded then
        rb.settings = nil
        package.loaded.settings = nil
    end

    if not rbs_is_loaded then
        rb.system = nil
        rb.metadata = nil
        package.loaded.rbsettings = nil
    end

    rb.core_color_table = color_table
    rb.core_talk_table = talk_table
    rb.core_list_settings_table = list_settings_table
end
get_core_settings()
get_core_settings = nil
package.loaded.menucoresettings = nil
collectgarbage("collect")
