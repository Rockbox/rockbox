---
--- X1000 CPU clock
---
JZ.x1000clk = {}

local h = HELP:get_topic("JZ"):create_topic("x1000clk")
h:add("CPU clock switching on the X1000, between the APLL (SCLK_A) and the MPLL.")
h:add("JZ.x1000clk.info() prints the PLLs and the CPU, L2, AHB and PCLK frequencies,")
h:add("JZ.x1000clk.freqs() returns them (in Hz).")
h:add("JZ.x1000clk.cpu_mpll(cdiv, l2div) runs the CPU and L2 from the MPLL, default /3 and /6.")
h:add("JZ.x1000clk.cpu_apll(cdiv, l2div) runs them from the APLL, default /1 and /2, turning it on if needed.")
h:add("JZ.x1000clk.apll_on(m) and apll_off() start (24 MHz * m, default 42) and stop the APLL.")
h:add("JZ.x1000clk.bus(h0div, h2div, pdiv) sets the AHB0, AHB2 and PCLK dividers from the MPLL,")
h:add("default 3, 3, 6 (200, 200, 100 MHz). bus(6) gives 100, 100, 50 MHz.")
h:add("JZ.x1000clk.test(n) switches between the two n times.")
h:add("The CPU functions do not change the AHB and PCLK clocks, they stay on the MPLL.")
h:add("JZ.x1000clk.measure() times a CPU loop against the OST (EXCLK / 4), prints and returns")
h:add("the measured and register CPU frequencies. pclk() and cpu() return register frequencies.")
h:add("JZ.x1000clk.verify() measures each CPU setting: the loop must take 6 cycles per pass")
h:add("at all of them if the frequencies are what the registers say.")

local EXCLK = 24000000

local function wait(cond, what)
    for i = 1, 1000 do
        if cond() then return end
    end
    error("timeout waiting for " .. what)
end

-- frequency of a PLL from its control register, 0 if it is off
local function pll_freq(reg)
    if reg.ON.read() == 0 then return 0 end
    local m = reg.PLLM.read() + 1
    local n = reg.PLLN.read() + 1
    local od = bit32.lshift(1, reg.PLLOD.read())
    return EXCLK * m / n / od
end

local function apll() return pll_freq(HW.CPM.APCR) end
local function mpll() return pll_freq(HW.CPM.MPCR) end

-- frequency of a CCR mux input: 1 = SCLK_A, 2 = MPLL
local function mux_freq(sel)
    if sel == 2 then return mpll() end
    if sel ~= 1 then return 0 end
    local src = HW.CPM.CCR.SEL_SRC.read()
    if src == 1 then return EXCLK end
    if src == 2 then return apll() end
    return 0
end

local function mhz(f) return string.format("%.1f MHz", f / 1e6) end

-- the clock frequencies the registers give, in Hz
function JZ.x1000clk.freqs()
    local ccr = HW.CPM.CCR
    local cpu_in = mux_freq(ccr.SEL_CPLL.read())
    local h0_in = mux_freq(ccr.SEL_H0PLL.read())
    local h2_in = mux_freq(ccr.SEL_H2PLL.read())
    return {
        apll = apll(), mpll = mpll(),
        cpu = cpu_in / (ccr.CDIV.read() + 1),
        l2 = cpu_in / (ccr.L2DIV.read() + 1),
        ahb0 = h0_in / (ccr.H0DIV.read() + 1),
        ahb2 = h2_in / (ccr.H2DIV.read() + 1),
        pclk = h2_in / (ccr.PDIV.read() + 1),
    }
end

function JZ.x1000clk.info()
    local f = JZ.x1000clk.freqs()
    print("APLL: " .. mhz(f.apll))
    print("MPLL: " .. mhz(f.mpll))
    print(string.format("CCR: 0x%08x CSR: 0x%08x", HW.CPM.CCR.read(), HW.CPM.CSR.read()))
    print("CPU:  " .. mhz(f.cpu))
    print("L2:   " .. mhz(f.l2))
    print("AHB0: " .. mhz(f.ahb0))
    print("AHB2: " .. mhz(f.ahb2))
    print("PCLK: " .. mhz(f.pclk))
end

-- Change the CPU and L2 dividers. The CE bits make the change take effect,
-- they are cleared afterwards as in Rockbox's clk_set_ccr_div().
local function set_cpu_div(cdiv, l2div)
    if l2div < cdiv or l2div > 4 * cdiv then
        error("L2 must run at 1 to 4 times less than the CPU")
    end
    local v = HW.CPM.CCR.read()
    v = bit32.replace(v, cdiv - 1, 0, 4)
    v = bit32.replace(v, l2div - 1, 4, 4)
    v = bit32.bor(v, bit32.lshift(1, 22)) -- CE_CPU
    HW.CPM.CCR.write(v)
    wait(function() return HW.CPM.CSR.CDIV_BUSY.read() == 0 end, "CPU divider")
    HW.CPM.CCR.CE_CPU.clr()
end

-- Change the AHB0, AHB2 and PCLK dividers, from the MPLL (the boot setup:
-- 3, 3 and 6 for 200, 200 and 100 MHz). The DDR has its own divider and
-- does not change.
function JZ.x1000clk.bus(h0div, h2div, pdiv)
    h0div = h0div or 3
    h2div = h2div or h0div
    pdiv = pdiv or 2 * h2div
    local ccr = HW.CPM.CCR
    local h0 = mux_freq(ccr.SEL_H0PLL.read()) / h0div
    local h2 = mux_freq(ccr.SEL_H2PLL.read()) / h2div
    -- USB: the turnaround setting is for AHB at 30 MHz or more, keep margin
    if h0 < 60e6 or h2 < 60e6 then
        error("AHB below 60 MHz, USB would stop working")
    end
    local v = ccr.read()
    v = bit32.replace(v, h0div - 1, 8, 4)
    v = bit32.replace(v, h2div - 1, 12, 4)
    v = bit32.replace(v, pdiv - 1, 16, 4)
    v = bit32.bor(v, bit32.lshift(1, 21), bit32.lshift(1, 20)) -- CE_AHB0, CE_AHB2
    ccr.write(v)
    wait(function() return HW.CPM.CSR.H0DIV_BUSY.read() == 0 and
                           HW.CPM.CSR.H2DIV_BUSY.read() == 0 end, "AHB dividers")
    ccr.CE_AHB0.clr()
    ccr.CE_AHB2.clr()
end

-- Change the CPU and L2 mux. It is glitch free as long as both inputs run.
local function set_cpu_mux(sel)
    if mux_freq(sel) == 0 then
        error("the new CPU clock source is not running")
    end
    HW.CPM.CCR.SEL_CPLL.write(sel)
    wait(function() return HW.CPM.CSR.CPU_MUX.read() == 1 end, "CPU mux")
end

-- Dividers first going down and mux first going up, so the CPU never runs
-- faster than either setting during the switch.
function JZ.x1000clk.cpu_mpll(cdiv, l2div)
    cdiv = cdiv or 3
    l2div = l2div or 2 * cdiv
    set_cpu_div(cdiv, l2div)
    set_cpu_mux(2)
end

function JZ.x1000clk.cpu_apll(cdiv, l2div)
    cdiv = cdiv or 1
    l2div = l2div or 2 * cdiv
    if apll() == 0 then JZ.x1000clk.apll_on() end
    HW.CPM.CCR.SEL_SRC.write("APLL")
    wait(function() return HW.CPM.CSR.SRC_MUX.read() == 1 end, "SCLK_A mux")
    set_cpu_mux(1)
    set_cpu_div(cdiv, l2div)
end

function JZ.x1000clk.apll_on(m)
    m = m or 42
    local apcr = HW.CPM.APCR
    if apcr.ON.read() == 1 then
        if apcr.PLLM.read() + 1 == m then return end
        JZ.x1000clk.apll_off()
    end
    apcr.write(bit32.bor(bit32.lshift(1, 31),       -- BS
                         bit32.lshift(m - 1, 24),   -- PLLM
                         bit32.lshift(1, 8)))        -- ENABLE
    wait(function() return apcr.ON.read() == 1 end, "APLL on")
end

function JZ.x1000clk.apll_off()
    local ccr = HW.CPM.CCR
    if ccr.SEL_SRC.read() == 2 and (ccr.SEL_CPLL.read() == 1 or
       ccr.SEL_H0PLL.read() == 1 or ccr.SEL_H2PLL.read() == 1) then
        error("the APLL is still in use, run JZ.x1000clk.cpu_mpll() first")
    end
    HW.CPM.APCR.ENABLE.clr()
    wait(function() return HW.CPM.APCR.ON.read() == 0 end, "APLL off")
end

function JZ.x1000clk.test(n)
    n = n or 100
    for i = 1, n do
        JZ.x1000clk.cpu_mpll()
        JZ.x1000clk.cpu_apll()
    end
    print(string.format("%d switches done", 2 * n))
    JZ.x1000clk.info()
end

--
-- Measurement: jz/x1000_spin.S, embedded below, runs n passes of a cached
-- addiu/bnez loop (6 cycles per pass) and records the OST2 counter around
-- it, which counts EXCLK / 4 independently of the PLLs.
--
local SPIN =
    "0000193c000039370000288f00b2093c20002a8d000000000000000000000000" ..
    "ffff0825feff00150000000020002c8d04002aaf08002caf0800e00300000000"

local DEV = DEV or hwstub.dev
local OST_HZ = EXCLK / 4
local spin_params, spin_code

local function spin_load()
    if spin_code then return end
    -- the end of hwstub's buffer: the stub receives USB transfers at its
    -- start, which would overwrite the parameters
    local buf = hwstub.dev.layout.buffer
    local base = bit32.band(buf.start + buf.size - 0x100, bit32.bnot(0x3f))
    spin_params = base
    spin_code = base + 0x40
    for i = 0, #SPIN / 8 - 1 do
        local hex = SPIN:sub(i * 8 + 1, i * 8 + 8)
        local w = tonumber(hex:sub(7, 8) .. hex:sub(5, 6) .. hex:sub(3, 4) .. hex:sub(1, 2), 16)
        if i == 0 then w = bit32.bor(w, bit32.rshift(spin_params, 16)) end
        if i == 1 then w = bit32.bor(w, bit32.band(spin_params, 0xffff)) end
        DEV.write32(spin_code + i * 4, w)
        if DEV.read32(spin_code + i * 4) ~= w then
            spin_code = nil
            error("spin kernel verify failed")
        end
    end
end

-- run n loop passes, return passes per second
local function spin(n)
    DEV.write32(spin_params, n)
    DEV.call(spin_code)
    local ost = (DEV.read32(spin_params + 8) - DEV.read32(spin_params + 4)) % 0x100000000
    return n * OST_HZ / ost
end

-- the CPU frequency the registers claim
local function cpu_freq()
    local ccr = HW.CPM.CCR
    return mux_freq(ccr.SEL_CPLL.read()) / (ccr.CDIV.read() + 1)
end

-- passes per second, with a loop of about 100 ms (hwstub's USB timeout
-- is 1 s)
local function rate()
    spin_load()
    local r = spin(100000)
    return spin(math.max(100000, math.floor(r * 0.1)))
end

-- returns the measured and the register CPU frequencies, in Hz
function JZ.x1000clk.measure(quiet)
    local r = rate()
    local f = cpu_freq()
    if not quiet then print(string.format("loop: %.3f M passes/s  CPU (registers): %s  -> %.3f cycles/pass, " ..
                        "measured CPU: %s", r / 1e6, mhz(f), f / r, mhz(6 * r))) end
    return 6 * r, f
end

function JZ.x1000clk.pclk()
    local ccr = HW.CPM.CCR
    return mux_freq(ccr.SEL_H2PLL.read()) / (ccr.PDIV.read() + 1)
end

function JZ.x1000clk.cpu()
    return cpu_freq()
end

function JZ.x1000clk.verify()
    local settings = {
        { "APLL / 1", function() JZ.x1000clk.cpu_apll(1) end },
        { "APLL / 2", function() JZ.x1000clk.cpu_apll(2) end },
        { "APLL / 3", function() JZ.x1000clk.cpu_apll(3) end },
        { "MPLL / 1", function() JZ.x1000clk.cpu_mpll(1) end },
        { "MPLL / 2", function() JZ.x1000clk.cpu_mpll(2) end },
        { "MPLL / 3", function() JZ.x1000clk.cpu_mpll(3) end },
        { "MPLL / 4", function() JZ.x1000clk.cpu_mpll(4) end },
    }
    print(string.format("%-9s %-11s %-13s %-12s %s", "setting", "registers",
                        "loop passes/s", "cycles/pass", "measured"))
    for _, s in ipairs(settings) do
        s[2]()
        local r = rate()
        local f = cpu_freq()
        print(string.format("%-9s %-11s %-13.0f %-12.3f %s", s[1], mhz(f), r, f / r, mhz(6 * r)))
    end
    JZ.x1000clk.cpu_apll()
end
