package.path = string.sub(string.gsub(debug.getinfo(1).source, "load.lua", "?.lua"),2) .. ";" .. package.path
require "stmp"
require "pp"
require "rk27xx"
require "atj"
require "jz"
require "hwlib"
require "dumper"

LOAD = {}

function LOAD.init()
    if hwstub.dev.target.id == hwstub.dev.target.STMP then
        STMP.init()
    elseif hwstub.dev.target.id == hwstub.dev.target.PP then
        PP.init()
    elseif hwstub.dev.target.id == hwstub.dev.target.RK27 then
        RK27XX.init()
    elseif hwstub.dev.target.id == hwstub.dev.target.ATJ then
        ATJ.init()
    elseif hwstub.dev.target.id == hwstub.dev.target.JZ then
        JZ.init()
    end
end