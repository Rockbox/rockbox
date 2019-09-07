require("rbsettings")
require("settings")
rb.metadata = nil -- remove track metadata settings
-------------------------------------------------------------------------------

local function print_setting_table(t_tbl, s_sep)
    s_sep = s_sep or ""
    local str = ""
    local function pfunct(t, sep, s, n) -- recursive print function
        local vtype
        for k, v in pairs(t) do
            vtype = type(v)
            if vtype == "table" then
                local f = string.format("%s[%s]", n, k)
                s = pfunct(v, sep, s, f)
            elseif vtype ==  "boolean" then
                v = v and "true" or "false"
                s = string.format("%s%s[%s] = %s%s", s, n, k, v, sep)
            elseif v then
                s = string.format("%s%s[%s] = %s%s", s, n, k, v, sep)
            end
        end
        return s
    end
    return pfunct(t_tbl, s_sep, str, "")
end

local filename = "/settings.txt"
local file = io.open(filename, "w+") -- overwrite
local t_settings

if not file then
    rb.splash(rb.HZ, "Error writing " .. filename)
    return
end

t_settings = rb.settings.dump('global_settings', "system")
file:write("global_settings:\n")
file:write(print_setting_table(t_settings, "\n"))
file:write("\n\n")

t_settings = rb.settings.dump('global_status', "system")
file:write("global_status:\n")
file:write(print_setting_table(t_settings, "\n"))
file:write("\n\n")

file:close()

rb.splash(100, "rb settings dumped: " .. filename)
