LV24230 = {}
LV24230.ADDR = 0xc0

function LV24230.write(reg, value)
    local dat = { reg, value }
    ATJ.i2c.transmit(LV24230.ADDR, dat)
end

function LV24230.read(reg)
    local dat = nil
    local lv24230_addr = 0xc0

    ATJ.i2c.reset()
    ATJ.i2c.start(LV24230.ADDR)

    -- register address
    ATJ.I2C.DAT.write(reg)
    ATJ.I2C.CTL.write(0x82)

    ATJ.i2c.wait_busy()

    if bit32.band(ATJ.I2C.CTL.read(), 1) == 0 then
        ATJ.i2c.reset()
        return nil
    end

    ATJ.i2c.restart(bit32.bor(LV24230.ADDR, 1))

    -- get data
    ATJ.I2C.CTL.write(0x83)

    if ATJ.i2c.wait_busy() ~= true then
        ATJ.i2c.stop()
        return nil
    end

    dat = ATJ.I2C.DAT.read()
    ATJ.i2c.stop()
    return dat
end

function LV24230.wait_busy()
    local try = 10
    while ((bit32.rshift(LV24230.read(0x11), 5) ~= 0) and (try > 0)) do
        hwstub.mdelay(100)
        try = try - 1
    end

    if try > 0 then
        return true
    else
        return false
    end
end

function LV24230.init()
    -- We need initialized i2c subsystem
    if ATJ.I2C == nil then
        error("i2c not initialized")
    end

    -- 1) write initialization values
    LV24230.write(0x0b, 0xff)
    LV24230.write(0x0d, 0x61)
    LV24230.write(0x0e, 0x50)
    LV24230.write(0x19, 0x80)
    LV24230.write(0x1a, 0x82)
    LV24230.write(0x1b, 0x00)
    LV24230.write(0x1d, 0x02)
    LV24230.write(0x0f, 0x13)

    -- 2) wait at least 200 ms
    hwstub.mdelay(300)

    -- 3) oscillator calibration
    LV24230.write(0x1e, 0x06)
    Lv24230.write(0x1f, 0x13)

    -- monitor status bits, callibration takes 500 - 600 ms
    LV24230.wait_busy()

    -- SD PLL lock, IF PLL lock
    LV24230.write(0x0f, 0x0b)

    -- 4) setup clock parameters for FM operation
    --    values recommended in app note
    LV24230.write(0x19, 0x43)
    LV24230.write(0x1a, 0x14)
    LV24230.write(0x1b, 0xcd)

    -- 50 kHz scan grid, set seek stop level
    LV24230.write(0x1d, 0x48)

    -- HACK setup PA to pass analog signal from FM
    local tmp = HW.CMU.DEVCLKEN.read()
    tmp = bit32.bor(tmp, 0x20000)
    HW.CMU.DEVCLKEN.write(tmp)

    tmp = HW.DAC.ANALOG.read()
    tmp = bit32.bor(tmp, 0x4fe)
    HW.DAC.ANALOG.write(tmp)
end

-- MUTE
function LV24230.mute(mute)
    local val = LV24230.read(0x0d)

    if mute == true then
        val = bit32.band(val, 0xfb)
    else
        val = bit32.bor(val, 4)
    end

    LV24230.write(0x0d, val)
end

-- force MONO mode
function LV24230.force_mono(mono)
    local val = LV24230.read(0x0d)

    if mono == true then
        val = bit32.bor(val, 8)
    else
        val = bit32.band(val, 0xf7)
    end

    Lv24230.write(0x0d, val)
end

-- frequency in Hz
function LV24230.set_frequency(freq)
    -- formula according to app note
    local value = (freq*100/1000000 - 6400)/5

    LV24230.mute(true)

    LV24230.write(0x1e, bit32.band(value, 0xff))
    LV24230.write(0x1f, bit32.rshift(value, 8))

    LV24230.wait_busy()

    -- set FLL on as described in table 5
    LV24230.write(0x1d, 0x68)

    LV24230.mute(false)
end

function LV24230.get_frequency()
    local val_hi = bit32.band(LV24230.read(0x11), 3)
    local val_lo = LV24230.read(0x10)
    local freq = bit32.lshift(val_hi, 8)
    freq = bit32.bor(freq, val_lo)
    freq = freq * 5 + 6400
    freq = freq * 1000000/100
    return freq
end

function LV24230.get_rssi()
    local val = LV24230.read(0x02)

    return bit32.band(val, 0x0f) * 5
end

function LV24230.seek(frequency, dir)
    LV24230.mute(true)
    LV24230.force_mono(true)

    -- 50kHz grid, FLL off
    LV24230.write(0x1d, 0x48)

    -- set start frequency
    local start_val = (frequency*100/1000000 - 6400)/5
    LV24230.write(0x1e, bit32.band(start_val, 0xff))
    LV24230.write(0x1f, bit32.rshift(start_val, 8))

    LV24230.wait_busy()

    local end_val = nil
    local end_lo = nil
    local end_hi = nil

    if dir == true then
        -- seek UP
        -- limit value 108.1 MHz
        end_val = (108100000*100/1000000 - 6400)/5
        end_lo = bit32.band(end_val, 0xff)
        end_hi = bit32.rshift(end_val, 8)

        -- set bit[7:6] to 0b11 for up seeking
        end_hi = bit32.bor(end_hi, 0xc0)
    else
        -- seek DOWN
        -- limit value 75.9 MHz
        end_val = (75900000*100/1000000 - 6400)/5
        end_lo = bit32.band(end_val, 0xff)
        end_hi = bit32.rshift(end_val, 8)

        -- set bit[7:6] to 0b10 for down seeking
        end_hi = bit32.bor(end_hi, 0x80)
    end

    -- write seek limit value
    LV24230.write(0x1e, end_lo)
    LV24230.write(0x1f, end_hi)

    LV24230.wait_busy()

    -- FLL on
    LV24230.write(0x1d, 0x68)

    -- enable stereo mode and unmute
    LV24230.force_mono(false)
    LV24230.mute(false)

    -- return tuned frequency
    return LV24230.get_frequency()
end

function LV24230.standby(standby)
    local val = LV24230.read(0x0f)

    if standby == true then
        val = bit32.band(val, 0xfe)
        LV24230.mute(true)
        LV24230.write(0x0f, val)
    else
        val = bit32.bor(val, 1)
        LV24230.write(0x0b, 0xff)
        LV24230.write(0x0f, val)

        -- app note says min 200 ms
        -- but shorter delay causes unplesent noise
        hwstub.mdelay(1000)

        -- restore last frequency to actually turn on
        LV24230.set_frequency(LV24230.get_frequency())
    end
end

