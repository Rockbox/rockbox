---
--- Chip Identification
---
STMP = { info = {} }

local h = HELP:create_topic("STMP")
h:add("This table contains the abstraction of the different device blocks for the STMP.")
h:add("It allows one to use higher-level primitives rather than poking at register directly.")
h:add("Furthermore, it tries as much as possible to hide the differences between the different STMP families.")

local function identify(name, family, desc)
    STMP.chipid = hwstub.dev.stmp.chipid
    STMP.info.chip = name
    STMP.info.revision = "TA" .. tostring(hwstub.dev.stmp.rev+1)
    STMP.desc = desc
    STMP.family = family
    print("Chip identified as " .. name ..", ROM " .. STMP.info.revision)
    if not hwstub.soc:select(desc) then
        print("Looking for soc " .. desc .. ": not found. Please load a soc by hand.")
    end
end

local hh = h:create_topic("is_imx233")
hh:add("STMP.is_imx233() returns true if the chip ID reports a i.MX233")

function STMP.is_imx233()
    return hwstub.dev.stmp.chipid == 0x3780
end

hh = h:create_topic("is_stmp3700")
hh:add("STMP.is_stmp3700() returns true if the chip ID reports a STMP3700")

function STMP.is_stmp3700()
    return hwstub.dev.stmp.chipid == 0x3700
end

hh = h:create_topic("is_stmp3770")
hh:add("STMP.is_stmp3770() returns true if the chip ID reports a STMP3770")

function STMP.is_stmp3770()
    return hwstub.dev.stmp.chipid == 0x37b0
end

hh = h:create_topic("is_stmp3600")
hh:add("STMP.is_stmp3600() returns true if the chip ID reports a STMP36xx")

function STMP.is_stmp3600()
    return hwstub.dev.stmp.chipid >= 0x3600 and hwstub.dev.stmp.chipid < 0x3700
end

hh = h:create_topic("debug")
hh:add("STMP.debug(...) prints some debug output if STMP.debug_on is true and does nothing otherwise.")

STMP.debug_on = false

function STMP.debug(...)
    if STMP.debug_on then print(...) end
end

-- init
function STMP.init()
    if STMP.is_imx233() then
        identify("STMP3780 (aka i.MX233)", "imx233", "imx233")
    elseif STMP.is_stmp3700() then
        identify("STMP3700", "stmp3700", "stmp3700")
    elseif STMP.is_stmp3770() then
        identify("STMP3770", "stmp3770", "stmp3700")
    elseif STMP.is_stmp3600() then
        identify("STMP3600", "stmp3600", "stmp3600")
    else
        print(string.format("Unable to identify this chip as a STMP: chipid=0x%x", hwstub.dev.stmp.chipid));
    end
end

require "stmp/digctl"
require "stmp/pinctrl"
require "stmp/lcdif"
require "stmp/pwm"
require "stmp/clkctrl"
require "stmp/i2c"
require "stmp/rom"
