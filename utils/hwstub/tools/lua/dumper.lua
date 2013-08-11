DUMPER = {}

local h = HELP:create_topic("DUMPER")
h:add("This table contains some tools to dump the registers from the device to a file.")

local hh = h:create_topic("dump_all")
hh:add("The DUMPER.dump_all(file) function dumps all the registers under HW to a file.")
hh:add("If the argument is a string, the function will interpret it as a path.")
hh:add("Otherwise it will be interpreted as an object returned by io.open")

function DUMPER.dump_all_reg(prefix, hw, f)
    for reg, tabl in pairs(hw) do
        if type(reg) == "string" and type(tabl) == "table" and tabl.read ~= nil then
            f:write(string.format("%s%s = %#08x\n", prefix, tabl.name, tabl.read()))
        end
    end
end

function DUMPER.dump_all_dev(prefix, hw, f)
    for block, tabl in pairs(hw) do
        if type(block) == "string" and type(tabl) == "table" then
            DUMPER.dump_all_reg(prefix .. block .. ".", tabl, f)
        end
    end
end

function DUMPER.dump_all(file)
    local f = file
    if type(file) == "string" then
        f = io.open(file, "w")
    end
    if f == nil then error("Cannot open file or write to nil") end
    f:write(string.format("HW = %s\n", HW.name))
    DUMPER.dump_all_dev("HW.", HW, f)
    if type(file) == "string" then
        io.close(f)
    end
end