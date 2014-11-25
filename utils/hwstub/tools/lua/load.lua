package.path = string.sub(string.gsub(debug.getinfo(1).source, "load.lua", "?.lua"),2) .. ";" .. package.path

if hwstub.dev.target.id == hwstub.dev.target.STMP then
    require "stmp"
elseif hwstub.dev.target.id == hwstub.dev.target.PP then
    require "pp"
elseif hwstub.dev.target.id == hwstub.dev.target.RK27 then
    require "rk27xx"
elseif hwstub.dev.target.id == hwstub.dev.target.ATJ then
    require "atj"
end

require "hwlib"
require "dumper"
