---
--- Chip Identification
---

PP = { info = {} }

local h = HELP:create_topic("PP")
h:add("This table contains the abstraction of the different device blocks for the Portal Player / GoForce")
h:add("It allows one to use higher-level primitives rather than poking at register directly.")
h:add("Furthermore, it tries as much as possible to hide the differences between the different PP families.")

local function identify(name, family, desc)
    PP.chipid = hwstub.dev.pp.chipid
    PP.info.chip = name
    PP.info.revision = hwstub.dev.pp.rev
    PP.desc = desc
    PP.family = family
    print("Chip identified as " .. name ..", ROM " .. PP.info.revision)
    if not hwstub.soc:select(desc) then
        print("Looking for soc " .. desc .. ": not found. Please load a soc by hand.")
    end
end

local hh = h:create_topic("is_pp611x")
hh:add("PP.is_pp611x() returns true if the chip ID reports a PP611x")

function PP.is_pp611x()
    return hwstub.dev.pp.chipid >= 0x6110
end

hh = h:create_topic("is_pp502x")
hh:add("PP.is_pp502x() returns true if the chip ID reports a PP502x")

function PP.is_pp502x()
    return hwstub.dev.pp.chipid >= 0x5020 and hwstub.dev.pp.chipid < 0x6100
end

hh = h:create_topic("is_pp500x")
hh:add("PP.is_pp500x() returns true if the chip ID reports a PP500x")

function PP.is_pp500x()
    return hwstub.dev.pp.chipid >= 0x5000 and hwstub.dev.pp.chipid < 0x5010
end

hh = h:create_topic("debug")
hh:add("PP.debug(...) prints some debug output if PP.debug_on is true and does nothing otherwise.")

PP.debug_on = false

function PP.debug(...)
    if PP.debug_on then print(...) end
end

hh = h:create_topic("debug")
hh:add("PP.debug(...) prints some debug output if PP.debug_on is true and does nothing otherwise.")

PP.debug_on = false

-- init
function PP.init()
    if PP.is_pp611x() then
        identify("PP611x (aka GoForce6110)", "pp6110", "pp6110")
    elseif PP.is_pp502x() then
        identify("PP502x", "pp502x", "pp502x")
    elseif PP.is_pp500x() then
        identify("PP500x", "pp500x", "pp500x")
    else
        print(string.format("Unable to identify this chip as a PP: chipid=0x%x", hwstub.dev.pp.chipid));
    end
end

require "pp/gpio"
