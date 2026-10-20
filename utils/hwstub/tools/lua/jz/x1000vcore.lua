---
--- X1000 CPU core voltage (AXP192 DCDC2) on the Agptek Rocker
---
JZ.x1000vcore = {}

local h = HELP:get_topic("JZ"):create_topic("x1000vcore")
h:add("CPU core voltage scaling: the AXP192 PMU at 0x34 on I2C2 (PD0/PD1), DCDC2 = CPU core.")
h:add("JZ.x1000vcore.info() prints the DCDC rails and the DCDC2 ramp control.")
h:add("JZ.x1000vcore.get() / set(mV) read and set DCDC2 (700-2275 mV, 25 mV steps).")
h:add("JZ.x1000vcore.check() runs the CPU timing loop and a DRAM pattern test.")
h:add("JZ.x1000vcore.scan(floor_mV, start_mV) steps DCDC2 down from start_mV (default: its")
h:add("boot value) to floor_mV")
h:add("(default 900) at the current CPU clock, checking each step, and goes back to the")
h:add("boot value at the end. Set the CPU clock first, e.g. JZ.x1000clk.cpu_mpll().")
h:add("JZ.x1000vcore.restore() goes back to the boot value.")
h:add("If the CPU dies at some voltage, the stub dies with it and DCDC2 stays there:")
h:add("unplug USB and power the player off to get the PMU back to its defaults.")

local DEV = DEV or hwstub.dev

local I2C_BUS = 2
local AXP_ADDR = 0x34
local I2C_FREQ = 100000

-- AXP192 registers
local AXP_DCDC2_CTRL = 0x10     -- bit 0: DCDC2 on
local AXP_OUT_CTRL = 0x12       -- bit 0: DCDC1 on, bit 1: DCDC3 on
local AXP_DCDC2_VOLT = 0x23     -- bits 5:0, 700 mV + 25 mV * n
local AXP_DCDC2_VRC = 0x25      -- voltage ramp control
local AXP_DCDC1_VOLT = 0x26     -- bits 6:0, 700 mV + 25 mV * n
local AXP_DCDC3_VOLT = 0x27     -- bits 6:0, 700 mV + 25 mV * n

-- DCDC2 boot values outside of this are not a CPU core voltage: refuse
local BOOT_MIN_MV, BOOT_MAX_MV = 1000, 1400
-- never go below this, whatever the caller asks
local HARD_FLOOR_MV = 800

local boot_mv

--
-- I2C, polled, as Rockbox's i2c-ingenic.c sets it up
--
-- the PCLK the timings are for, the controller is set up again if it changes
local i2c_pclk

local function i2c_init()
    local pclk = JZ.x1000clk.pclk()
    if i2c_pclk == pclk then return end
    local i2c = HW.I2C[I2C_BUS]
    HW.CPM.CLKGR.I2C2.clr()
    -- PD0/PD1: function 1 (I2C2)
    HW.GPIO.INT[3].CLR.write(3)
    HW.GPIO.MSK[3].CLR.write(3)
    HW.GPIO.PAT1[3].CLR.write(3)
    HW.GPIO.PAT0[3].SET.write(3)

    if i2c.ENBST.ACTIVE.read() == 1 then
        i2c.ENABLE.ACTIVE.clr()
        while i2c.ENBST.ACTIVE.read() == 1 do end
    end

    local su = math.floor(pclk / (I2C_FREQ * 8))
    if su > 1 then su = su - 1 end
    i2c.CON.write(bit32.bor(bit32.lshift(1, 6),   -- SLVDIS
                            bit32.lshift(1, 5),   -- RESTART
                            bit32.lshift(1, 1),   -- SPEED = 100K
                            1))                   -- MD
    i2c.SDASU.write(math.min(su, 255))
    i2c.SDAHD.write(math.floor(pclk / (I2C_FREQ * 12)))
    i2c.SLCNT.write(math.max(math.floor(pclk / (I2C_FREQ * 2)), 8))
    i2c.SHCNT.write(math.max(math.floor(pclk / (I2C_FREQ * 2)), 6))
    i2c.TAR.write(AXP_ADDR)
    i2c.INTMSK.write(0)

    i2c.ENABLE.ACTIVE.set()
    while i2c.ENBST.ACTIVE.read() == 0 do end
    i2c_pclk = pclk
end

local function i2c_check_abort(what)
    local i2c = HW.I2C[I2C_BUS]
    if i2c.RINTST.TXABT.read() == 1 then
        local src = i2c.ABTSRC.read()
        i2c.CTXABT.read()
        error(string.format("I2C %s aborted, ABTSRC = 0x%08x", what, src))
    end
end

local function i2c_wait_idle(what)
    local i2c = HW.I2C[I2C_BUS]
    for i = 1, 1000 do
        i2c_check_abort(what)
        if i2c.STATUS.TFE.read() == 1 and i2c.STATUS.MSTACT.read() == 0 then return end
    end
    error("I2C " .. what .. " timeout")
end

local function axp_read(reg)
    i2c_init()
    local i2c = HW.I2C[I2C_BUS]
    i2c.CINT.read()
    i2c.DC.write(reg)
    i2c.DC.write(bit32.bor(bit32.lshift(1, 8), bit32.lshift(1, 9))) -- CMD (read), STOP
    for i = 1, 1000 do
        i2c_check_abort("read")
        if i2c.RXFLR.read() > 0 then
            return bit32.band(i2c.DC.read(), 0xff)
        end
    end
    error("I2C read timeout")
end

local function axp_write(reg, val)
    i2c_init()
    local i2c = HW.I2C[I2C_BUS]
    i2c.CINT.read()
    i2c.DC.write(reg)
    i2c.DC.write(bit32.bor(val, bit32.lshift(1, 9))) -- STOP
    i2c_wait_idle("write")
end

--
-- DCDC2
--
local function dcdc2_mv()
    return 700 + 25 * bit32.band(axp_read(AXP_DCDC2_VOLT), 0x3f)
end

function JZ.x1000vcore.get()
    return dcdc2_mv()
end

local function remember_boot()
    if boot_mv then return end
    local mv = dcdc2_mv()
    if mv < BOOT_MIN_MV or mv > BOOT_MAX_MV then
        error(string.format("DCDC2 boot value %d mV is outside %d-%d mV, " ..
                            "it does not look like the CPU core voltage", mv,
                            BOOT_MIN_MV, BOOT_MAX_MV))
    end
    boot_mv = mv
end

function JZ.x1000vcore.set(mv)
    remember_boot()
    if mv < HARD_FLOOR_MV or mv > boot_mv then
        error(string.format("%d mV is outside %d mV to the boot value %d mV",
                            mv, HARD_FLOOR_MV, boot_mv))
    end
    local n = math.floor((mv - 700) / 25)
    local r = axp_read(AXP_DCDC2_VOLT)
    axp_write(AXP_DCDC2_VOLT, bit32.bor(bit32.band(r, bit32.bnot(0x3f)), n))
    -- the ramp is 25 mV per 15.625 or 31.25 us, a millisecond is plenty
    hwstub.mdelay(1)
    local got = dcdc2_mv()
    if got ~= 700 + 25 * n then
        error(string.format("DCDC2 reads back %d mV instead of %d mV", got, 700 + 25 * n))
    end
end

function JZ.x1000vcore.restore()
    if boot_mv then JZ.x1000vcore.set(boot_mv) end
end

function JZ.x1000vcore.info()
    remember_boot()
    local on2 = bit32.band(axp_read(AXP_DCDC2_CTRL), 1)
    local on13 = axp_read(AXP_OUT_CTRL)
    print(string.format("DCDC1 (I/O):      %4d mV %s", 700 + 25 * bit32.band(axp_read(AXP_DCDC1_VOLT), 0x7f),
                        bit32.band(on13, 1) ~= 0 and "on" or "off"))
    print(string.format("DCDC2 (CPU core): %4d mV %s, boot value %d mV", dcdc2_mv(),
                        on2 ~= 0 and "on" or "off", boot_mv))
    print(string.format("DCDC3 (core):     %4d mV %s", 700 + 25 * bit32.band(axp_read(AXP_DCDC3_VOLT), 0x7f),
                        bit32.band(on13, 2) ~= 0 and "on" or "off"))
    print(string.format("DCDC2 ramp control (0x25): 0x%02x", axp_read(AXP_DCDC2_VRC)))
end

--
-- Checks
--

-- DRAM pattern test through the stub: the CPU copies each word over USB.
-- Below the timing loop at the end of hwstub's buffer.
local function dram_test(words)
    local buf = hwstub.dev.layout.buffer
    local base = bit32.band(buf.start + buf.size - 0x1100, bit32.bnot(0x3f))
    -- a different pattern each run, and every word different
    local seed = os.time() % 0x10000
    local function pat(i) return bit32.bxor(seed * 0x10001, i * 0x01000193) end
    for i = 0, words - 1 do DEV.write32(base + 4 * i, pat(i)) end
    for i = 0, words - 1 do
        local v = DEV.read32(base + 4 * i)
        if v ~= pat(i) then
            return false, string.format("DRAM @ 0x%08x: 0x%08x instead of 0x%08x", base + 4 * i, v, pat(i))
        end
    end
    return true
end

-- the CPU frequency must be the register one (the loop takes 6 cycles per
-- pass) and the DRAM pattern must read back
function JZ.x1000vcore.check()
    local measured, expected = JZ.x1000clk.measure(true)
    if math.abs(measured - expected) > expected * 1e-3 then
        return false, string.format("CPU runs at %.1f MHz instead of %.1f MHz",
                                    measured / 1e6, expected / 1e6)
    end
    return dram_test(256)
end

function JZ.x1000vcore.scan(floor_mv, start_mv)
    floor_mv = floor_mv or 900
    if floor_mv < HARD_FLOOR_MV then
        error(string.format("floor %d mV is below the hard floor of %d mV", floor_mv, HARD_FLOOR_MV))
    end
    remember_boot()
    start_mv = math.min(start_mv or boot_mv, boot_mv)
    print(string.format("CPU %.1f MHz, PCLK %.1f MHz, DCDC2 boot value %d mV, scanning %d to %d mV",
                        JZ.x1000clk.cpu() / 1e6, JZ.x1000clk.pclk() / 1e6, boot_mv,
                        start_mv, floor_mv))
    local ok, err = JZ.x1000vcore.check()
    if not ok then error("check fails at the boot voltage: " .. err) end

    local lowest = boot_mv
    for mv = start_mv - 25, floor_mv, -25 do
        JZ.x1000vcore.set(mv)
        for i = 1, 3 do
            ok, err = JZ.x1000vcore.check()
            if not ok then break end
        end
        if not ok then
            print(string.format("%d mV: FAIL: %s", mv, err))
            break
        end
        print(string.format("%d mV: ok", mv))
        lowest = mv
    end
    JZ.x1000vcore.restore()
    print(string.format("lowest passing: %d mV, back to %d mV", lowest, boot_mv))
    return lowest
end
