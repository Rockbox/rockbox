ATJ.gpio = {}

function ATJ.gpio.muxsel(dev)
    if type(dev) == "string" then
        if dev == "LCM" then dev = 0
        elseif dev == "SD" then dev = 1
        elseif dev == "NAND" then dev = 2
        else error("Invalid mux string " .. dev)
        end
    end

    local mfctl0 = HW.GPIO.MFCTL0.read()
    if dev == 0 then
        -- LCM (taken from WELCOME.BIN)
        mfctl0 = bit32.band(mfctl0, 0xfe3f3f00)
        mfctl0 = bit32.bor(mfctl0,  0x00808092)
    elseif dev == 1 then
        -- SD (taken from CARD.DRV)
        mfctl0 = bit32.band(mfctl0, 0xff3ffffc)
        mfctl0 = bit32.bor(mfctl0,  0x01300004)
    elseif dev == 2 then
        -- NAND (taken from BROM dump)
        mfctl0 = bit32.band(mfctl0, 0xfe3ff300)
        mfctl0 = bit32.bor(mfctl0,  0x00400449)
    end

    -- enable multifunction mux
    HW.GPIO.MFCTL1.write(0x80000000)

    -- write multifunction mux selection
    HW.GPIO.MFCTL0.write(mfctl0)
end

function ATJ.gpio.outen(port, pin, en)
    if type(port) == "string" then
        if port == "PORTA" then
            HW.GPIO.AOUTEN.write(bit32.replace(HW.GPIO.AOUTEN.read(), en, pin, 1))
        elseif port == "PORTB" then
            HW.GPIO.BOUTEN.write(bit32.replace(HW.GPIO.BOUTEN.read(), en, pin, 1))
        else error("Invalid port string " .. port)
        end
    end
end

function ATJ.gpio.inen(port, pin)
    if type(port) == "string" then
        if port == "PORTA" then
            HW.GPIO.AINEN.write(bit32.replace(HW.GPIO.AINEN.read(), en, pin, 1))
        elseif port == "PORTB" then
            HW.GPIO.BINEN.write(bit32.replace(HW.GPIO.BINEN.read(), en, pin, 1))
        else error("Invalid port string " .. port)
        end
    end
end

function ATJ.gpio.set(port, pin, val)
    if type(port) == "string" then
        if port == "PORTA" then
            HW.GPIO.ADAT.write(bit32.replace(HW.GPIO.ADAT.read(), val, pin, 1))
        elseif port == "PORTB" then
            HW.GPIO.BDAT.write(bit32.replace(HW.GPIO.BDAT.read(), val, pin, 1))
        else error("Invalid port string " .. port)
        end
    end
end
