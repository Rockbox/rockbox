---
--- E150 FM tuner (Sanyo LV24230LP) on the ATJ213x I2C controller
---
--- Follows the OF driver FM.DRV (see atj2137_fm.md):
---   I2C controller instance 0 at 0xb0180000, tuner at 0xC0/0xC1, one register per
---   transfer (no auto-increment). CTL: 0x86 START, 0x8e repeated START, 0x82 byte
---   with ACK, 0x83 byte with NACK, 0x8b STOP; STAT bit 7 transfer done, bit 6 stop
---   done (both w1c); CTL bit 0 = ACK received.
---   The I2C pins are muxed in (GPIO_MFCTL1 I2C1SS = 0, MFEN) only for a transfer
---   and the previous mux restored afterwards: the OF does so, the pins are shared.
---
--- Audio is analog: tuner -> FMINL/R -> ADC_CTL FMLE/FMRE -> DAC_ANALOG FIMM ->
--- mixer -> headphone PA.
---
ATJ.fm = {}

local CMU_DEVCLKEN = 0xb0010080
local GPIO_MFCTL1 = 0xb01c001c
local GPIO_AOUTEN, GPIO_AINEN, GPIO_ADAT = 0xb01c0000, 0xb01c0004, 0xb01c0008
local ADC_CTL = 0xb0110000
local DAC_ANALOG = 0xb0100010

local I2C_INST = 0
local I2C = 0xb0180000 + I2C_INST * 0x20
local I2C_CTL, I2C_CLKDIV, I2C_STAT, I2C_ADDR, I2C_DAT =
    I2C, I2C + 4, I2C + 8, I2C + 0xc, I2C + 0x10

local TUNER = 0xc0

local function rd(a) return DEV.read32(a) end
local function wr(a, v) DEV.write32(a, v) end
local function bor(a, m) wr(a, bit32.bor(rd(a), m)) end
local function bclr(a, m) wr(a, bit32.band(rd(a), bit32.bnot(m))) end

ATJ.fm.verbose = false

local function log(fmt, ...)
    if ATJ.fm.verbose then print(string.format(fmt, ...)) end
end

-- ---- I2C ----

local saved_mux

local function mux_on()
    saved_mux = rd(GPIO_MFCTL1)
    wr(GPIO_MFCTL1, bit32.bor(bit32.band(saved_mux, bit32.bnot(0x30)), 0x80000000))
end

local function mux_off()
    if saved_mux then wr(GPIO_MFCTL1, saved_mux) end
    saved_mux = nil
end

-- wait for STAT bit, ack it; returns false on timeout
local function wait_stat(mask)
    for _ = 1, 200 do
        if bit32.band(rd(I2C_STAT), mask) ~= 0 then
            wr(I2C_STAT, mask)
            return true
        end
    end
    return false
end

-- transfer with ACK check (START / repeated START / write byte)
local function xfer(ctl, what)
    wr(I2C_CTL, ctl)
    if not wait_stat(0x80) then
        log("i2c: %s: no transfer done (STAT %02x)", what, rd(I2C_STAT))
        return false
    end
    if bit32.band(rd(I2C_CTL), 1) ~= 1 then
        log("i2c: %s: no ACK (CTL %02x)", what, rd(I2C_CTL))
        return false
    end
    return true
end

local function stop()
    wr(I2C_CTL, 0x8b)
    wait_stat(0x40)
end

function ATJ.fm.i2c_init()
    bor(CMU_DEVCLKEN, bit32.lshift(1, 23))  -- I2C clock
    wr(I2C_CTL, 0)
    wr(I2C_CTL, 0x80)
    wr(I2C_CLKDIV, 2)                        -- ~350kHz at PCLK 11.25MHz
end

-- returns value or nil
function ATJ.fm.read(reg)
    mux_on()
    local v
    wr(I2C_ADDR, TUNER)
    if xfer(0x86, "start") then
        wr(I2C_DAT, reg)
        if xfer(0x82, "reg") then
            wr(I2C_ADDR, bit32.bor(TUNER, 1))
            if xfer(0x8e, "restart") then
                wr(I2C_CTL, 0x83)               -- read one byte, NACK
                wait_stat(0x80)
                v = bit32.band(rd(I2C_DAT), 0xff)
            end
        end
    end
    stop()
    mux_off()
    return v
end

-- returns true on success
function ATJ.fm.write(reg, val)
    mux_on()
    local ok = false
    wr(I2C_ADDR, TUNER)
    if xfer(0x86, "start") then
        wr(I2C_DAT, reg)
        if xfer(0x82, "reg") then
            wr(I2C_DAT, val)
            ok = xfer(0x82, "data")
        end
    end
    stop()
    mux_off()
    log("w %02x = %02x %s", reg, val, ok and "" or "FAILED")
    return ok
end

-- ---- tuner ----

local REGS = {0x00, 0x02, 0x0b, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x19, 0x1a, 0x1b, 0x1d, 0x1e, 0x1f}

-- first test: does the chip answer, and what do the registers hold
function ATJ.fm.probe()
    ATJ.fm.i2c_init()
    local was = ATJ.fm.verbose
    ATJ.fm.verbose = true
    for _, r in ipairs(REGS) do
        local v = ATJ.fm.read(r)
        print(string.format("reg %02x = %s", r, v and string.format("%02x", v) or "no answer"))
    end
    ATJ.fm.verbose = was
end

-- poll reg 11 until the measurement is done; true on success
local function measure_done(tries)
    local v
    for _ = 1, tries do
        hwstub.mdelay(20)
        v = ATJ.fm.read(0x11)
        if v == nil then return false, "no answer" end
        if bit32.band(v, 0xe0) ~= 0xe0 then break end
    end
    -- OF Set_SD_Freq: done (bits 7:5 clear) and bit 3 set
    local ok = v and bit32.band(v, 0xe8) == 0x08
    return ok, string.format("reg11 %02x", v or 0)
end

local function set_target(t)
    ATJ.fm.write(0x1e, bit32.band(t, 0xff))
    ATJ.fm.write(0x1f, bit32.rshift(t, 8))
end

-- OF FM_INIT_Drv
local ctrl1, ctrl3 = 0x61, 0x13

function ATJ.fm.init()
    ATJ.fm.i2c_init()
    local seq = {{0x0b, 0xff}, {0x0d, 0x61}, {0x0e, 0x50}, {0x19, 0x80},
                 {0x1a, 0x82}, {0x1b, 0x00}, {0x1d, 0x02}}
    for _, rv in ipairs(seq) do
        if not ATJ.fm.write(rv[1], rv[2]) then
            print(string.format("write %02x failed, tuner not answering", rv[1]))
            return false
        end
        hwstub.mdelay(10)
    end
    ctrl1 = 0x61
    ctrl3 = 0x13
    ATJ.fm.write(0x0f, ctrl3)            -- power on
    hwstub.mdelay(750)
    set_target(0x1306)                   -- IF / SD oscillator calibration
    local ok, st = measure_done(800)
    -- the OF ignores this result (seen as reg11 13, bit 3 clear, on a working unit)
    print("calibration: " .. (ok and "ok" or "not confirmed") .. " (" .. st .. ")")
    ctrl3 = bit32.bor(bit32.band(ctrl3, 0xef), 0x08)
    ATJ.fm.write(0x0f, ctrl3)
    hwstub.mdelay(750)
    ATJ.fm.write(0x19, 0x43)
    ATJ.fm.write(0x1a, 0x14)
    ATJ.fm.write(0x1b, 0xcd)
    ATJ.fm.write(0x1d, 0x88)             -- 100kHz grid, FLL on
    return ok
end

-- field strength 0..15 and stereo flag
function ATJ.fm.signal()
    local v = ATJ.fm.read(0x02)
    if not v then return nil end
    return bit32.band(v, 0xf), bit32.band(v, 0x10) ~= 0
end

-- freq in MHz (e.g. 101.5)
function ATJ.fm.tune(mhz)
    local khz = math.floor(mhz * 1000 + 0.5)
    set_target(math.floor((khz - 64000) / 50))
    local ok, st = measure_done(400)
    local rssi, stereo = ATJ.fm.signal()
    print(string.format("%.1f MHz: %s (%s) rssi %s %s", khz / 1000,
          ok and "locked" or "FAILED", st, tostring(rssi), stereo and "stereo" or "mono"))
    return ok, rssi
end

-- step through the band and print the field strength
function ATJ.fm.scan(from, to, step)
    from, to, step = from or 87.5, to or 108.0, step or 0.1
    local f = from
    while f <= to + 0.001 do
        local ok, rssi = ATJ.fm.tune(f)
        f = f + step
    end
end

-- tuner audio on/off, analog path to the headphones; vol 0..31
function ATJ.fm.audio(on, vol)
    if on then
        bor(CMU_DEVCLKEN, bit32.bor(bit32.lshift(1, 17), bit32.lshift(1, 18))) -- DAC, ADC
        -- FMLE, FMRE, FMGC = 101 (0dB)
        local adc = rd(ADC_CTL)
        adc = bit32.band(adc, bit32.bnot(bit32.lshift(7, 13)))
        wr(ADC_CTL, bit32.bor(adc, bit32.lshift(5, 13), 0x1800))
        -- EN, AMPE, FIMM, HAVC
        local dac = rd(DAC_ANALOG)
        dac = bit32.band(dac, bit32.bnot(bit32.lshift(0x1f, 3)))
        dac = bit32.bor(dac, 0x1, 0x2, 0x400, bit32.lshift(bit32.band(vol or 20, 0x1f), 3))
        wr(DAC_ANALOG, dac)
        ctrl1 = bit32.bor(ctrl1, 5)          -- tuner audio un-mute
        ATJ.fm.write(0x0d, ctrl1)
    else
        ctrl1 = bit32.band(ctrl1, bit32.bnot(4))
        ATJ.fm.write(0x0d, ctrl1)
        bclr(DAC_ANALOG, 0x400)
        bclr(ADC_CTL, 0x1800)
    end
    print(string.format("ADC_CTL %08x DAC_ANALOG %08x", rd(ADC_CTL), rd(DAC_ANALOG)))
end

-- OF FM_STANDBY_Drv
function ATJ.fm.off()
    ATJ.fm.audio(false)
    ctrl3 = bit32.band(ctrl3, bit32.bnot(1))
    ATJ.fm.write(0x0f, ctrl3)
end

-- headphones (the antenna) plugged: GPIO A26, low when inserted on the E150
-- (the OF FM.DRV tests for high, measured the other way round on the device)
function ATJ.fm.headphones()
    bclr(GPIO_AOUTEN, bit32.lshift(1, 26))
    bor(GPIO_AINEN, bit32.lshift(1, 26))
    return bit32.band(rd(GPIO_ADAT), bit32.lshift(1, 26)) == 0
end
