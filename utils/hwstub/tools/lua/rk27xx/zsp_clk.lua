--
-- RK27xx DSP clock modes: which ones the DSP runs in, how fast, and whether
-- the ARM can still reach DMEM.
--
-- Usage (inside hwstub_shell, after make in rk27xx/zsp):
--   dofile("lua/rk27xx/zsp.lua"); dofile("lua/rk27xx/zsp_clk.lua")
--   ZSPCLK.run()                   -- all modes, risky ones last
--   ZSPCLK.run{ modes = { "pll96", "bypass" } }
--
-- rk27xx/zsp/tests/tclk_count.bin counts in DMEM; each mode is set while it runs, the
-- count is read twice 200 ms apart (TMR1 times it), and the clock is put
-- back to 160 MHz before the next mode. The DSP clock is the count rate
-- relative to the 160 MHz baseline. Each mode is announced before it is
-- set: if the device stops answering, that mode hung the bus (reset it).
--

ZSPCLK = {}

local DEV = DEV or hwstub.dev
local function rd(a) return DEV.read32(a) end
local function wr(a, v) DEV.write32(a, v) end

local SCU      = 0x1801c000
local PLLCON2  = SCU + 0x0c
local DIVCON1  = SCU + 0x14
local STATUS   = SCU + 0x2c
local TMR1LR   = 0x18000010
local TMR1CVR  = 0x18000014
local TMR1CON  = 0x18000018

local CNT      = 0x200              -- tests/tclk_count.s
local PLL_160  = 0x01850270
local PD       = bit32.lshift(1, 22)
local BYPASS   = 1
local SLOW     = 2                  -- DIVCON1 bit 1

-- PLLCON2 with CLKR = 5 (4 MHz reference): VCO = 4 MHz * mul, / (od + 1)
local function pll(mul, od)
    return bit32.bor(bit32.lshift(3, 23), bit32.lshift(5, 16),
                     bit32.lshift(mul - 1, 4), bit32.lshift(od, 1))
end

local function set_pll(v)
    wr(PLLCON2, v)
    hwstub.mdelay(2)                -- lock ~0.3 ms
end

local function set_slow(on)
    local d = rd(DIVCON1)
    wr(DIVCON1, on and bit32.bor(d, SLOW) or bit32.band(d, bit32.bnot(SLOW)))
    hwstub.mdelay(2)
end

local function baseline()
    set_slow(false)
    set_pll(PLL_160)
end

-- name, description, function that sets the mode (from the baseline)
local MODES = {
    { "pll96",   "PLL 96 MHz (VCO 192 / 2)",        function() set_pll(pll(48, 1)) end },
    { "pll48",   "PLL 48 MHz (VCO 192 / 4)",        function() set_pll(pll(48, 3)) end },
    { "pll24",   "PLL 24 MHz (VCO 192 / 8)",        function() set_pll(pll(48, 7)) end },
    { "pll15",   "PLL 15 MHz (VCO 120 / 8)",        function() set_pll(pll(30, 7)) end },
    { "pll40",   "PLL 40 MHz (VCO 120 / 3)",        function() set_pll(pll(30, 2)) end },
    { "bypass",  "PLL bypass, PLL on",              function() set_pll(bit32.bor(PLL_160, BYPASS)) end },
    { "slow",    "slow mode, PLL on at 160",        function() set_slow(true) end },
    { "bypassoff", "PLL bypass, PLL powered down",  function() set_pll(bit32.bor(PLL_160, BYPASS, PD)) end },
    { "slowoff", "slow mode, PLL powered down",     function() set_slow(true); set_pll(bit32.bor(PLL_160, PD)) end },
}

local function count()
    return ZSP.dread(CNT) + ZSP.dread(CNT + 1) * 0x10000
end

-- count rate (per second) over ms milliseconds, by TMR1
local function rate(ms)
    local c0, t0 = count(), rd(TMR1CVR)
    hwstub.mdelay(ms)
    local c1, t1 = count(), rd(TMR1CVR)
    local dt = ZSPCLK.tmr_down and (t0 - t1) % 4294967296 or (t1 - t0) % 4294967296
    local dc = (c1 - c0) % 4294967296
    return dc / (dt / ZSPCLK.tmr_hz), dc
end

local function timer()
    wr(TMR1CON, 0)
    wr(TMR1LR, 0xffffffff)
    wr(TMR1CON, bit32.bor(bit32.lshift(1, 8), bit32.lshift(1, 3)))
    -- TMR1 runs from PCLK; the host's mdelay is only a rough reference
    local a = rd(TMR1CVR)
    hwstub.mdelay(500)
    local b = rd(TMR1CVR)
    ZSPCLK.tmr_down = ((a - b) % 4294967296) < 0x80000000
    ZSPCLK.tmr_hz = (ZSPCLK.tmr_down and (a - b) % 4294967296 or (b - a) % 4294967296) / 0.5
    print(string.format("TMR1 ~%.1f MHz (from the host's 500 ms)", ZSPCLK.tmr_hz / 1e6))
end

function ZSPCLK.run(opts)
    opts = opts or {}
    local want = {}
    for _, m in ipairs(opts.modes or {}) do want[m] = true end
    local ok, err = xpcall(function()
        ZSP.setup()
        timer()
        ZSP.hold()
        ZSP.fill_dmem(0, 0x1000)
        ZSP.load(ZSP.cfg.dir .. "/tests/tclk_count.bin")
        ZSP.release()
        hwstub.mdelay(10)
        local base = rate(200)
        if base == 0 then error("the DSP does not count at 160 MHz") end
        print(string.format("%-10s %-32s %12s %9s  %-8s %-8s",
            "mode", "", "counts/s", "DSP MHz", "PLLCON2", "STATUS"))
        print(string.format("%-10s %-32s %12.0f %9.1f  %08x %08x",
            "base", "PLL 160 MHz", base, 160, rd(PLLCON2), rd(STATUS)))
        for _, m in ipairs(MODES) do
            if not opts.modes or want[m[1]] then
                io.write(string.format("%-10s %-32s ", m[1], m[2]))
                io.flush()              -- shows which mode it was if the bus hangs
                m[3]()
                local r = rate(200)
                print(string.format("%12.0f %9.1f  %08x %08x%s", r, 160 * r / base,
                    rd(PLLCON2), rd(STATUS), r == 0 and "  DSP STOPPED" or ""))
                baseline()
                local back = rate(100)
                if back == 0 then print("           DSP did not resume at 160 MHz") end
            end
        end
    end, debug.traceback)
    if not ok then print("ERROR: " .. tostring(err)) end
    pcall(baseline)
    ZSP.restore()
end
