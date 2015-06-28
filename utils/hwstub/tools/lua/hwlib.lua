HWLIB = {}

local h = HELP:create_topic("HWLIB")
h:add("This table contains helper functions for use inside hwstub_shell")

local hh = h:create_topic("load_blob")
hh:add("load_blob(filename, address) -- this function loads raw binary blob from the file filename")
hh:add("                                at specified address in memory. No cache coherency is")
hh:add("                                guaranteed")

hh = h:create_topic("printf")
hh:add("printf(s,...) -- this function is simple wrapper around string.format to emulate")
hh:add("                 C printf() function")

function HWLIB.load_blob(filename, address)
    local f = assert(io.open(filename, "rb"))
    local bytes = f:read("*all")
    for b in string.gmatch(bytes, ".") do
        DEV.write8(address, string.byte(b))
        address = address + 1
    end
    io.close(f)
end

function HWLIB.printf(...)
   local function wrapper(...) io.write(string.format(...)) end
   local status, result = pcall(wrapper, ...)
   if not status then error(result, 2) end
end
