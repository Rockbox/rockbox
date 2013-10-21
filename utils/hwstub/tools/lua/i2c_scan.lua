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