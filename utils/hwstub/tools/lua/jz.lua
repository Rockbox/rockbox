---
--- Chip Identification
---
JZ = { info = {} }

local h = HELP:create_topic("JZ")
h:add("This table contains the abstraction of the different device blocks for the JZ.")
h:add("It allows one to use higher-level primitives rather than poking at register directly.")

hh = h:create_topic("debug")
hh:add("STMP.debug(...) prints some debug output if JZ.debug_on is true and does nothing otherwise.")

JZ.debug_on = false

function STMP.debug(...)
    if STMP.debug_on then print(...) end
end

-- init
function JZ.init()
    local desc = string.format("jz%04x%c", hwstub.dev.jz.chipid, hwstub.dev.jz.rev)
    desc = desc:lower()
    if not hwstub.soc:select(desc) then
        print("Looking for soc " .. desc .. ": not found. Please load a soc by hand.")
    end
end

require "jz/gpio"
require "jz/lcd"
require "jz/nand"