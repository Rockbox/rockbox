ATJ.i2c = {}
ATJ.I2C = nil

-- Ungate I2C clock, set IO mux
-- port can be either "I2C1" or "I2C2"
function ATJ.i2c.init(port)
    HW.CMU.DEVCLKEN.write(bit32.bor(HW.CMU.DEVCLKEN.read(), 0x800000))

    if type(port) == "string" then
        if port == "I2C1" then
            ATJ.gpio.muxsel(port)
        elseif port == "I2C2" then
            ATJ.gpio.muxsel(port)
        else
            error("Invalid port string " .. port)
        end

        ATJ.I2C = ATJ.i2c.hwport(port)
        -- SCL = PCLK / (CLKDIV * 16)
        -- in hwstub PCLK is 4.5 MHz by default
        -- so this translates to SCL = 281.250 kHz
        ATJ.I2C.CLKDIV.write(0x1)
    end
end

-- Internal use
function ATJ.i2c.hwport(port)
     local I2C = nil
     if type(port) == "string" then
        if port == "I2C1" then
            I2C = HW.I2C1
        elseif port == "I2C2" then
            I2C = HW.I2C2
        else
            error("Invalid port string " .. port)
        end
    end

    return I2C
end

function ATJ.i2c.wait_busy()
    local try = 10
    repeat
        if bit32.band(ATJ.I2C.STAT.read(), 0x80) ~= 0 then
            -- transfer complete condition
            break
        end
        try = try - 1
    until (try > 0)

    -- Clear transmit complete status flag
    ATJ.I2C.STAT.write(0x80)

    if try > 0 then
        return true
    else
        return false
    end
end

function ATJ.i2c.start(address)
    ATJ.I2C.ADDR.write(address)

    -- Generate start condition
    ATJ.I2C.CTL.write(0x00000086)

    ATJ.i2c.wait_busy()

    if bit32.band(ATJ.I2C.CTL.read(), 1) ~= 0 then
        return true
    else
        return false
    end
end

function ATJ.i2c.restart(address)
    ATJ.I2C.ADDR.write(address)

    -- Generate restart condition
    ATJ.I2C.CTL.write(0x0000008e)

    ATJ.i2c.wait_busy()

    if bit32.band(ATJ.I2C.CTL.read(), 1) ~= 0 then
        return true
    else
        return false
    end
end

function ATJ.i2c.stop()
    -- Generate stop condition
    ATJ.I2C.CTL.write(0x0000008b)

    -- clear STOP status flag
    ATJ.I2C.STAT.write(0x00000040)
end

function ATJ.i2c.reset()
    ATJ.I2C.CTL.write(0)
    hwstub.mdelay(1)
    ATJ.I2C.CTL.write(0x80)
end

function ATJ.i2c.transmit(slave_addr, buffer, send_stop)
    ATJ.i2c.reset()

    ATJ.i2c.start(slave_addr)

    for i, v in ipairs(buffer) do
        ATJ.I2C.DAT.write(v)
        ATJ.I2C.CTL.write(0x82)

        ATJ.i2c.wait_busy()

        if bit32.band(ATJ.I2C.CTL.read(), 1) == 0 then
            ATJ.i2c.reset()
            return false
        end
    end

    if send_stop then
        ATJ.i2c.stop()
    end

    return true
end

function ATJ.i2c.receive(slave_addr, length)
    local dat = {}

    ATJ.i2c.reset()
    ATJ.i2c.start(bit32.bor(slave_addr,1))

    for i=length,1,-1 do
        if i > 1 then
            ATJ.I2C.CTL.write(0x82)
        else
            ATJ.I2C.CTL.write(0x83)
        end

        if ATJ.i2c.wait_busy() ~= true then
            ATJ.i2c.stop()
            return nil
        end

        table.insert(dat, ATJ.I2C.DAT.read())
    end

    ATJ.i2c.stop()
    return dat
end
