package.path = string.sub(string.gsub(debug.getinfo(1).source, "load.lua", "?.lua"),2) .. ";" .. package.path

require "stmp"
