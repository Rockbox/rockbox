ATJ.gpio = {}

function ATJ.gpio.muxsel(dev)
    if type(dev) == "string" then
        local mfctl0 = HW.GPIO.MFCTL0.read()
        local mfctl1 = HW.GPIO.MFCTL1.read()

        if dev == "LCM" then
            -- LCM (taken from WELCOME.BIN)
            mfctl0 = bit32.band(mfctl0, 0xfe3f3f00)
            mfctl0 = bit32.bor(mfctl0,  0x00808092)
        elseif dev == "SD" then
            -- SD (taken from CARD.DRV)
            mfctl0 = bit32.band(mfctl0, 0xff3ffffc)
            mfctl0 = bit32.bor(mfctl0,  0x01300004)
        elseif dev == "NAND" then
            -- NAND (taken from BROM dump)
            mfctl0 = bit32.band(mfctl0, 0xfe3ff300)
            mfctl0 = bit32.bor(mfctl0,  0x00400449)
        elseif dev == "I2C1" then
            -- I2C1 interface
            mfctl1 = bit32.band(mfctl1, 0xffffffcf)
        elseif dev == "I2C2" then
            -- I2C2 interface
            mfctl1 = bit32.band(mfctl1, 0xffffffbf)
        else
            error("Invalid mux string " .. dev)
        end

        -- enable multifunction mux
        mfctl1 = bit32.bor(mfctl1, 0x80000000)
        HW.GPIO.MFCTL1.write(mfctl1)

        -- write multifunction mux selection
        HW.GPIO.MFCTL0.write(mfctl0)
    end
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
