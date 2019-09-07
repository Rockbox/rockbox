require("rbsettings")
require("settings") --settings.lua
rb.system = nil -- remove system settings
-------------------------------------------------------------------------------
local track_data = rb.metadata.mp3_entry
local cur_trk = "audio_current_track"
-------------------------------------------------------------------------------
local trackname  = rb.settings.read(cur_trk, track_data.title) or
                   rb.settings.read(cur_trk, track_data.path)
if not trackname or trackname == "" then
    os.exit(1, "No track loaded")
else
    rb.splash(100, trackname)
end
-------------------------------------------------------------------------------
local function dump_albumart(fileout)
    local t_albumart = rb.settings.read(cur_trk, track_data.albumart, "metadata")
    local t_aaext = {".bmp",".png", ".jpg"}
    local path = rb.settings.read(cur_trk, track_data.path)
    if t_albumart.pos > 0 and t_albumart.size > 0 and t_albumart.type > 0 then

        if t_aaext[t_albumart.type] then
            local filename = "/" .. fileout .. t_aaext[t_albumart.type]
            local aa = io.open(filename, "w+") -- overwrite
            if not aa then
                rb.splash(rb.HZ, "Error writing " .. filename)
                return
            end

            local track = io.open(path, "r")
            if not track then
                rb.splash(rb.HZ, "Error opening " .. path)
                return
            end
            track:seek("set", t_albumart.pos )
            for i = 0, t_albumart.size, 32 do
                aa:write(track:read(32))
            end
            rb.splash(rb.HZ, "Saved: " .. filename)
            track:close()
            aa:close()
        else

        end
    end
end

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

local filename = "/metadata.txt"
local file = io.open(filename, "w+") -- overwrite
local t_settings

if not file then
    rb.splash(rb.HZ, "Error writing " .. filename)
    return
end

---[[
t_settings = rb.settings.dump(cur_trk, "metadata", "mp3_entry")
file:write(trackname .. ":\n")
file:write(print_setting_table(t_settings, "\n"))
file:write("\n\n")
file:close()

rb.splash(100, "metadata dumped: " .. filename)

if rb.settings.read(cur_trk, track_data.has_embedded_albumart) then
    dump_albumart("/albumart")
end
