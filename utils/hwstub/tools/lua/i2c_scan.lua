I2CSCAN = {}

function I2CSCAN.scan()
    STMP.i2c.init()
    STMP.i2c.set_speed(true)
    for i = 2, 254, 2 do
        if STMP.i2c.transmit(i, {}, true) then
            print(string.format("%#x OK", i))
        end
    end 
end

-- if file is nil, return array
-- if size is nil, dump the whole EEPROM
function I2CSCAN.dump_rom(file, size)
    STMP.i2c.init()
    STMP.i2c.set_speed(true)
    if not STMP.i2c.transmit(0xa0, {0, 0}, false) then
        error("Cannot send address")
    end
    local res = {}
    if size == nil then
        size = 0xffff
    end
    for i = 0, size do
        local l = STMP.i2c.receive(0xa0, 1)
        if l == nil then
            error("error during transfer")
        end
        for i = 1, #l do
            table.insert(res, l[i])
        end
    end
    if file == nil then
        return res
    end
    local f = file
    if type(file) == "string" then
        f = io.open(file, "w")
    end
    if f == nil then error("Cannot open file or write to nil") end
    for i = 1, #res do
        f:write(string.char(res[i]))
    end
    if type(file) == "string" then
        io.close(f)
    end
end