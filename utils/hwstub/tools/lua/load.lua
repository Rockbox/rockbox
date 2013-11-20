package.path = string.sub(string.gsub(debug.getinfo(1).source, "load.lua", "?.lua"),2) .. ";" .. package.path

if hwstub.dev.target.id == hwstub.dev.target.STMP then
    require "stmp"
end
require "dumper"