--
-- RK27xx ZSP test runner for hwstub_shell.
--
-- The test suite is in rk27xx/zsp/ next to this file: test sources and
-- generators in tests/, built by its Makefile with tools/zsp/zspasm.pl
-- (make; make bench for the cycle benchmarks), result checkers
-- (tests/expect.py, make results) and the benchmark and bus sweep reports
-- in tools/. ZSP.cfg.dir is that directory unless set otherwise; results
-- are written to results/ below it. The cycle benchmarks (ZSP.bench) are
-- what tools/zsp/zspbench.py compares zspsim.py's timing model with.
--
-- Usage (inside hwstub_shell, after make in rk27xx/zsp):
--   dofile("lua/rk27xx/zsp.lua")
--   ZSP.setup()                      -- clocks on, DSP held in reset
--   ZSP.run("t0b_probe")             -- load tests/t0b_probe.bin, run, report
--   ZSP.scan()                       -- look for 0xC0Dx probe markers
--   ZSP.restore()                    -- put SCU registers back
--

ZSP = {}

local DEV = DEV or hwstub.dev

-- the test suite: rk27xx/zsp/ next to this file, however it was loaded
local function suite_dir()
    local src = debug.getinfo(1, "S").source
    local dir = src:sub(1, 1) == "@" and src:sub(2):match("^(.*)/[^/]*$")
    return (dir or ".") .. "/zsp"
end

ZSP.cfg = {
    dir        = suite_dir(),   -- test suite (tests/ and results/ below it)
    IMEM_HOST  = 0x30040000,    -- ARM address of DSP IMEM
    DMEM_HOST  = 0x30000000,    -- ARM address of DSP DMEM
    WINDOW     = 0x40000,       -- bytes reserved per memory
    IMEM_UNIT  = 2,             -- host bytes per DSP instruction address (halfword image)
    DMEM_UNIT  = 2,             -- host bytes per DSP data address (confirm with t0c)
    DMEM_DSP   = 0,             -- DSP-side address of DMEM (from t0b; must match -D DMEM_DSP)
    RES_OFF    = 0x100,         -- result block offset, DSP data units (tests/common.inc)
    RES_LEN    = 0x100,         -- result block length, DSP data units
    swap_hw    = false,         -- swap halfwords inside each 32-bit IMEM write
    timeout_ms = 200,
    dsp_mhz    = nil,           -- DSP clock set by ZSP.setup() (nil: leave the PLL as it is)
    results    = "results",     -- result directory below dir (ZSP.sweep() changes it)
    setup_each_run = true,      -- redo ZSP.setup() (peripheral reset) before every ZSP.run:
                                -- after a hang the store lag comes back otherwise
    CLEAR_BYTES = 0x2000,       -- DMEM bytes filled with 0xA5A5 before each run
    RAW_BYTES  = 0x2000,        -- DMEM bytes saved raw (host view) with each result
}

local SCU       = 0x1801c000
local CLKCFG    = SCU + 0x18
local RSTCFG    = SCU + 0x1c
local PLLCON1   = SCU + 0x08
local PLLCON2   = SCU + 0x0c
local DIVCON1   = SCU + 0x14
local STATUS    = SCU + 0x2c
local CHIPCFG   = SCU + 0x28
local CHIPCFG_BIT18   = bit32.lshift(1, 18)   -- must be clear or the DSP does not run (SDK DSPLoader.c)
local CLKCFG_DSP      = bit32.lshift(1, 1)
local RSTCFG_DSP_CORE = bit32.lshift(1, 4)
local MAGIC = 0xD0DE

local saved = {}
local function rd(a) return DEV.read32(a) end
local function wr(a, v) DEV.write32(a, v) end

-- results directory (ZSP.cfg.dir .. "/" .. ZSP.cfg.results), created if needed
function ZSP.rdir()
    local d = ZSP.cfg.dir .. "/" .. (ZSP.cfg.results or "results")
    os.execute("mkdir -p " .. d)
    return d
end

local function wait_status(mask, what)
    for _ = 1, 200 do
        if bit32.band(rd(STATUS), mask) == mask then return true end
        hwstub.mdelay(1)
    end
    print("  timeout waiting for " .. what)
    return false
end

---------------------------------------------------------------------------
-- DSP control
---------------------------------------------------------------------------

function ZSP.hold()
    wr(RSTCFG, bit32.bor(rd(RSTCFG), RSTCFG_DSP_CORE))
end

function ZSP.release()
    wr(RSTCFG, bit32.band(rd(RSTCFG), bit32.bnot(RSTCFG_DSP_CORE)))
end

---------------------------------------------------------------------------
-- clocks
---------------------------------------------------------------------------

local DIVCON1_DSP_SLOW = 2

-- output of a PLL from its PLLCONx: 24 MHz / (CLKR+1) * (CLKF+1) / (OD+1);
-- 24 MHz in bypass
local function pll_hz(p)
    if bit32.band(p, 1) ~= 0 then return 24e6 end
    return 24e6 * (bit32.extract(p, 4, 12) + 1) / (bit32.extract(p, 16, 6) + 1)
           / (bit32.extract(p, 1, 3) + 1)
end

-- DSP, ARM and AHB (HCLK, which the SDRAM runs at) clocks in Hz. ARM side
-- as measured on a YP-CP3 (rk27xx/clk.lua CLK.table()): the source is
-- PLL1, or 24 MHz in slow mode (DIVCON1 bit 0); HCLK = source / 2; ARM =
-- source, or source / 2 with DIVCON1 bit 2. Rockbox runs ARM/HCLK 50/50
-- MHz, 200/100 boosted; hwstub 200/100.
function ZSP.clocks()
    local d = rd(DIVCON1)
    local src = bit32.band(d, 1) ~= 0 and 24e6 or pll_hz(rd(PLLCON1))
    local arm = bit32.band(d, 4) ~= 0 and src / 2 or src
    local dsp = bit32.band(d, DIVCON1_DSP_SLOW) ~= 0 and 24e6 or pll_hz(rd(PLLCON2))
    return dsp, arm, src / 2
end

-- PLLCON2 for the lowest clock >= khz with a 4 MHz reference (CLKR = 5)
-- and the VCO, 4 MHz x (CLKF + 1), within 120..200 MHz: the range the
-- RK27xx PLLs are known to run at. Returns it and the clock in kHz.
local function pll_setting(khz)
    local best, pllcon = nil, nil
    for od = 0, 7 do
        local mul = math.max(30, math.floor((khz * (od + 1) + 3999) / 4000))
        if mul <= 50 then
            local f = math.floor(4000 * mul / (od + 1))
            if not best or f < best then
                best = f
                pllcon = 0x01850000 + (mul - 1) * 0x10 + od * 2
            end
        end
    end
    return pllcon, best
end

-- the DSP runs from the oscillator (DIVCON1 slow mode) while its PLL
-- changes, so this works with the core running or in reset
local function set_pllcon(v)
    wr(DIVCON1, bit32.bor(rd(DIVCON1), DIVCON1_DSP_SLOW))
    wr(PLLCON2, v)
    hwstub.mdelay(2)                -- lock ~0.3 ms
    wr(DIVCON1, bit32.band(rd(DIVCON1), bit32.bnot(DIVCON1_DSP_SLOW)))
end

-- Sets the DSP clock to the lowest the PLL makes >= mhz (15..200); returns it
function ZSP.set_clock(mhz)
    local pllcon, khz = pll_setting(math.floor(mhz * 1000 + 0.5))
    set_pllcon(pllcon)
    return khz / 1000
end

---------------------------------------------------------------------------

-- DSP power-up as the SDK does it (PMU_SetFreq in hw_pmu.o, StartDecoder in
-- DualCore.o): gate the DSP clock, assert core+peripheral reset, start the
-- PLL, ungate the clock, release the peripheral reset; the core stays in
-- reset until ZSP.release().
--   opts.chipcfg18: true = set CHIPCFG[18] (video loader), default clear (JPEG loader)
--   opts.no_peri:   skip the peripheral reset pulse (old behaviour)
local RSTCFG_DSP_PERI = bit32.lshift(1, 5)
function ZSP.setup(opts)
    opts = opts or {}
    if not saved.clkcfg then
        saved.clkcfg = rd(CLKCFG)
        saved.rstcfg = rd(RSTCFG)
        saved.pllcon2 = rd(PLLCON2)
        saved.divcon1 = rd(DIVCON1)
        saved.chipcfg = rd(CHIPCFG)
    end
    if not opts.no_peri then
        wr(CLKCFG, bit32.bor(rd(CLKCFG), CLKCFG_DSP))                       -- DSP clock off
        wr(RSTCFG, bit32.bor(rd(RSTCFG), RSTCFG_DSP_CORE + RSTCFG_DSP_PERI)) -- core + peri reset
        hwstub.mdelay(1)
    end
    ZSP.hold()
    if bit32.band(rd(PLLCON2), bit32.lshift(1, 22)) ~= 0 then
        wr(PLLCON2, bit32.band(rd(PLLCON2), bit32.bnot(bit32.lshift(1, 22))))
    end
    wait_status(0x2, "DSP PLL lock")
    wr(DIVCON1, bit32.band(rd(DIVCON1), bit32.bnot(2)))
    if ZSP.cfg.dsp_mhz then ZSP.set_clock(ZSP.cfg.dsp_mhz) end
    wr(CLKCFG, bit32.band(rd(CLKCFG), bit32.bnot(CLKCFG_DSP)))              -- DSP clock on
    wait_status(0x10, "DSPSYSCLKVALID")
    hwstub.mdelay(1)
    wr(RSTCFG, bit32.band(rd(RSTCFG), bit32.bnot(RSTCFG_DSP_PERI)))         -- peri out of reset
    if opts.chipcfg18 then
        wr(CHIPCFG, bit32.bor(rd(CHIPCFG), CHIPCFG_BIT18))
    else
        wr(CHIPCFG, bit32.band(rd(CHIPCFG), bit32.bnot(CHIPCFG_BIT18)))
    end
    if not opts.quiet then
        local dsp, arm, hclk = ZSP.clocks()
        print(string.format("ZSP setup: CLKCFG=%08x RSTCFG=%08x CHIPCFG=%08x STATUS=%08x",
            rd(CLKCFG), rd(RSTCFG), rd(CHIPCFG), rd(STATUS)))
        print(string.format("clocks: DSP %.2f MHz, ARM %.0f MHz, HCLK %.0f MHz",
            dsp / 1e6, arm / 1e6, hclk / 1e6))
    end
end

function ZSP.restore()
    if not saved.clkcfg then return end
    ZSP.hold()
    for _, r in ipairs({ { CHIPCFG, saved.chipcfg }, { DIVCON1, saved.divcon1 }, { PLLCON2, saved.pllcon2 },
                         { RSTCFG, saved.rstcfg }, { CLKCFG, saved.clkcfg } }) do
        pcall(wr, r[1], r[2])
    end
    saved = {}
end

---------------------------------------------------------------------------
-- memory helpers (DSP addresses -> host addresses)
---------------------------------------------------------------------------

local function dhost(d)   -- DSP data address -> host byte address
    local c = ZSP.cfg
    return c.DMEM_HOST + (d - c.DMEM_DSP) * c.DMEM_UNIT
end

function ZSP.dread(d)
    if ZSP.cfg.DMEM_UNIT == 2 then return DEV.read16(dhost(d)) end
    return DEV.read8(dhost(d))
end

function ZSP.dwrite(d, v)
    if ZSP.cfg.DMEM_UNIT == 2 then DEV.write16(dhost(d), v) else DEV.write8(dhost(d), v) end
end

local function readfile(path)
    local f = assert(io.open(path, "rb"))
    local s = f:read("*a")
    f:close()
    return s
end

function ZSP.load(path)
    local c = ZSP.cfg
    local s = readfile(path)
    if #s % 4 ~= 0 then s = s .. string.rep("\0", 4 - #s % 4) end
    for i = 1, #s, 4 do
        local b1, b2, b3, b4 = s:byte(i, i + 3)
        local lo, hi = b1 + b2 * 0x100, b3 + b4 * 0x100
        if c.swap_hw then lo, hi = hi, lo end
        wr(c.IMEM_HOST + (i - 1), lo + hi * 0x10000)
    end
    -- verify
    for i = 1, #s, 4 do
        local b1, b2, b3, b4 = s:byte(i, i + 3)
        local lo, hi = b1 + b2 * 0x100, b3 + b4 * 0x100
        if c.swap_hw then lo, hi = hi, lo end
        local got = rd(c.IMEM_HOST + (i - 1))
        if got ~= lo + hi * 0x10000 then
            error(string.format("IMEM verify failed @ %#x: %08x != %08x",
                c.IMEM_HOST + i - 1, got, lo + hi * 0x10000))
        end
    end
    return #s
end

function ZSP.fill_dmem(val, bytes)
    local c = ZSP.cfg
    bytes = bytes or c.WINDOW
    local v = val + val * 0x10000
    for a = 0, bytes - 4, 4 do wr(c.DMEM_HOST + a, v) end
end

function ZSP.clear_results()
    local c = ZSP.cfg
    for i = 0, c.RES_LEN - 1 do ZSP.dwrite(c.DMEM_DSP + c.RES_OFF + i, 0xA5A5) end
end

function ZSP.results()
    local c = ZSP.cfg
    local t = {}
    for i = 0, c.RES_LEN - 1 do t[i] = ZSP.dread(c.DMEM_DSP + c.RES_OFF + i) end
    return t
end

---------------------------------------------------------------------------
-- running tests
---------------------------------------------------------------------------

local function save_results(name, t, ok)
    local c = ZSP.cfg
    local f = assert(io.open(ZSP.rdir() .. "/" .. name .. ".txt", "w"))
    local dsp, arm, hclk = ZSP.clocks()
    f:write(string.format("# %s %s DMEM_DSP=%#x DMEM_UNIT=%d DSP_HZ=%.0f ARM_HZ=%.0f HCLK=%.0f\n", name,
        ok and "done" or "TIMEOUT", c.DMEM_DSP, c.DMEM_UNIT, dsp, arm, hclk))
    for i = 0, c.RES_LEN - 1, 8 do
        local line = {}
        for j = i, i + 7 do line[#line + 1] = string.format("%04x", t[j]) end
        f:write(string.format("%03x: %s\n", i, table.concat(line, " ")))
    end
    -- raw host view of the start of DMEM, independent of DMEM_UNIT/DMEM_DSP
    f:write(string.format("# raw DMEM host bytes from %#x\n", c.DMEM_HOST))
    local raw = DEV.read(c.DMEM_HOST, c.RAW_BYTES)
    for i = 1, #raw, 32 do
        local line = {}
        for j = i, math.min(i + 31, #raw), 2 do
            line[#line + 1] = string.format("%04x", raw:byte(j) + raw:byte(j + 1) * 0x100)
        end
        f:write(string.format("raw %05x: %s\n", i - 1, table.concat(line, " ")))
    end
    f:close()
end

-- opts.timeout_ms, opts.clear_all (fill whole DMEM with 0xA5A5 first),
-- opts.on_release: function called right after the DSP leaves reset,
-- opts.save_as: result file name (default: the test name),
-- opts.before_release: function called after loading, before the DSP starts
function ZSP.run(name, opts)
    opts = opts or {}
    local c = ZSP.cfg
    if c.setup_each_run and saved.clkcfg then ZSP.setup({ quiet = true }) end
    ZSP.hold()
    -- result block + scratch area; clear_all fills the whole DMEM window
    ZSP.fill_dmem(0xA5A5, opts.clear_all and c.WINDOW or c.CLEAR_BYTES)
    local n = ZSP.load(c.dir .. "/tests/" .. name .. ".bin")
    if opts.before_release then opts.before_release() end
    ZSP.release()
    if opts.on_release then opts.on_release() end
    local ok = false
    local tmo = opts.timeout_ms or c.timeout_ms
    for _ = 1, tmo do
        if ZSP.dread(c.DMEM_DSP + c.RES_OFF + 63) == MAGIC then ok = true break end
        hwstub.mdelay(1)
    end
    ZSP.hold()
    local t = ZSP.results()
    save_results(opts.save_as or name, t, ok)
    print(string.format("%s: %d bytes, %s, test id %04x -> %s/%s.txt",
        name, n, ok and "done" or "TIMEOUT (no magic)", t[62], c.results or "results", opts.save_as or name))
    ZSP.last = t
    return ok
end

-- DMEM halfword i (DSP address i) = 0xB000 + i for i < 0x100.
function ZSP.pattern()
    local c = ZSP.cfg
    for i = 0, 0xff, 2 do
        wr(c.DMEM_HOST + i * 2, (0xB000 + i) + (0xB000 + i + 1) * 0x10000)
    end
end

-- Run without waiting for MAGIC (t0a, t0b): release for ms milliseconds.
-- opts.pattern: write ZSP.pattern() after clearing DMEM.
function ZSP.run_for(name, ms, opts)
    local c = ZSP.cfg
    opts = opts or {}
    ZSP.hold()
    ZSP.fill_dmem(0xA5A5)
    if opts.pattern then ZSP.pattern() end
    local n = ZSP.load(c.dir .. "/tests/" .. name .. ".bin")
    ZSP.release()
    hwstub.mdelay(ms or 50)
    ZSP.hold()
    print(string.format("%s: %d bytes, ran %d ms", name, n, ms or 50))
end

-- Print n DSP data halfwords starting at DSP address d.
function ZSP.dump(d, n)
    local line = {}
    for i = 0, (n or 16) - 1 do line[#line + 1] = string.format("%04x", ZSP.dread(d + i)) end
    print(string.format("%#06x: %s", d, table.concat(line, " ")))
end

-- Find halfwords lo..hi (default the 0xC0D0-0xC0DF probe markers) in DMEM
-- (and IMEM when with_imem; the program image itself contains the constants).
function ZSP.scan(lo, hi, with_imem)
    local c = ZSP.cfg
    lo, hi = lo or 0xC0D0, hi or 0xC0DF
    local found = 0
    local mems = { { "DMEM", c.DMEM_HOST } }
    if with_imem then mems[2] = { "IMEM", c.IMEM_HOST } end
    for _, m in ipairs(mems) do
        local CH = 0x1000
        for base = 0, c.WINDOW - CH, CH do
            local s = DEV.read(m[2] + base, CH)
            for i = 1, #s - 1, 2 do
                local v = s:byte(i) + s:byte(i + 1) * 0x100
                if v >= lo and v <= hi then
                    local off = base + i - 1
                    print(string.format("  %s %#x (+%#x bytes, halfword %#x): %04x",
                        m[1], m[2] + off, off, math.floor(off / 2), v))
                    found = found + 1
                end
            end
        end
    end
    print(string.format("scan %04x-%04x: %d hits", lo, hi, found))
end

-- Run a list of tests in order.
function ZSP.suite(names, opts)
    for _, n in ipairs(names) do ZSP.run(n, opts) end
end

---------------------------------------------------------------------------
-- timing: ARM TMR1 measures the time from DSP release to MAGIC
---------------------------------------------------------------------------

local TMR1LR, TMR1CVR, TMR1CON = 0x18000010, 0x18000014, 0x18000018
local tmr = nil

local function tmr_ticks(a, b)          -- elapsed ticks between two CVR reads
    local d = tmr.down and (a - b) or (b - a)
    return d % 4294967296
end

function ZSP.timer_init()
    wr(TMR1CON, 0)
    wr(TMR1LR, 0xffffffff)
    wr(TMR1CON, bit32.bor(bit32.lshift(1, 8), bit32.lshift(1, 3)))
    local a = rd(TMR1CVR)
    hwstub.mdelay(1000)
    local b = rd(TMR1CVR)
    tmr = { down = ((a - b) % 4294967296) < 0x80000000 }
    tmr.hz = tmr_ticks(a, b) / 1.0
    local dsp, arm, hclk = ZSP.clocks()
    print(string.format("TMR1 %.3f MHz (%s), DSP clock %.2f MHz, ARM %.0f MHz, HCLK %.0f MHz",
        tmr.hz / 1e6, tmr.down and "down" or "up", dsp / 1e6, arm / 1e6, hclk / 1e6))
end

-- Run one image with outer count n; returns elapsed TMR1 ticks or nil on timeout.
local function timed_run(path, n, timeout_ms)
    local c = ZSP.cfg
    ZSP.hold()
    ZSP.load(path)
    ZSP.dwrite(c.DMEM_DSP + c.RES_OFF + 0x20, n - 1)
    ZSP.dwrite(c.DMEM_DSP + c.RES_OFF + 63, 0)
    local t0 = rd(TMR1CVR)
    ZSP.release()
    local t1
    local deadline = timeout_ms or 20000
    for _ = 1, deadline do
        if ZSP.dread(c.DMEM_DSP + c.RES_OFF + 63) == MAGIC then t1 = rd(TMR1CVR) break end
        hwstub.mdelay(1)
    end
    ZSP.hold()
    if not t1 then return nil end
    return tmr_ticks(t0, t1)
end

-- Benchmarks built by tools/gen_bench.py (make bench). names: list of case
-- names (default: all, read from tests/bench/cases.json is not possible in
-- plain Lua, so pass ZSP.BENCH or your own list). Each case runs at U1 and
-- U2 with an outer count scaled so one run takes about opts.target_ms.
-- Writes results/bench.txt: case u outer ticks tmr_hz dsp_hz hclk
function ZSP.bench(names, opts)
    opts = opts or {}
    local c = ZSP.cfg
    if not tmr then ZSP.timer_init() end
    local f = assert(io.open(ZSP.rdir() .. "/bench.txt", opts.append and "a" or "w"))
    local target = (opts.target_ms or 300) / 1000 * tmr.hz
    for _, name in ipairs(names or ZSP.BENCH) do
        for _, u in ipairs({ opts.u1 or 8, opts.u2 or 24 }) do
            local path = string.format("%s/tests/bench/%s_u%d.bin", c.dir, name, u)
            -- grow the outer count until a run is long enough that host
            -- polling latency (~1 ms) is negligible
            local n = 1
            local t = timed_run(path, n)
            for _ = 1, 6 do
                if not t or t >= target / 2 or n >= 65535 then break end
                n = math.max(n + 1, math.min(65535, math.floor(n * target / math.max(t, 1))))
                t = timed_run(path, n)
            end
            if t then
                local dsp_hz, _, hclk = ZSP.clocks()
                f:write(string.format("%s %d %d %d %.0f %.0f %.0f\n", name, u, n, t, tmr.hz, dsp_hz, hclk))
                print(string.format("%-12s u=%-2d outer=%-5d %8.3f ms  %.2f cycles/group-pass",
                    name, u, n, t / tmr.hz * 1000, t / tmr.hz * dsp_hz / n / 1000 / u))
            else
                f:write(string.format("%s %d %d TIMEOUT\n", name, u, n))
                print(string.format("%-12s u=%-2d TIMEOUT", name, u))
            end
            f:flush()
        end
    end
    f:close()
end

---------------------------------------------------------------------------
-- interrupts and the bus window (tests/gen_t21.py)
---------------------------------------------------------------------------

-- Hex dump of host memory to results/<fname>.txt; unreadable chunks are
-- reported instead of aborting.
function ZSP.peek(addr, bytes, fname)
    local c = ZSP.cfg
    local f = assert(io.open(ZSP.rdir() .. "/" .. fname .. ".txt", "w"))
    for base = addr, addr + bytes - 1, 0x40 do
        local okr, s = pcall(DEV.read, base, 0x40)
        if okr and s then
            local line = {}
            for i = 1, #s - 3, 4 do
                line[#line + 1] = string.format("%08x", s:byte(i) + s:byte(i + 1) * 0x100
                    + s:byte(i + 2) * 0x10000 + s:byte(i + 3) * 0x1000000)
            end
            f:write(string.format("%08x: %s\n", base, table.concat(line, " ")))
        else
            f:write(string.format("%08x: read error %s\n", base, tostring(s)))
        end
    end
    f:close()
    print("wrote results/" .. fname .. ".txt")
end

-- ARM view of the "DSP EPR" region and the mailbox (AHB0 port), DSP in reset.
function ZSP.peek_epr()
    ZSP.hold()
    ZSP.peek(0x30080000, 0x400, "epr_30080000")
    ZSP.peek(0x30088000, 0x100, "epr_30088000")
    ZSP.peek(0x18088000, 0x80, "mailbox_ahb0")
end

-- t23: pulse CHIPCFG bit 16 ("ARM to DSP interrupt") while the DSP spins.
-- Also records CHIPCFG and the ARM INTC raw state before/after in
-- results/t23_arm.txt.
local INTC0 = 0x18080000
function ZSP.nmi_test(opts)
    opts = opts or {}
    local c = ZSP.cfg
    local log = {}
    local function note(s) log[#log + 1] = s print("  " .. s) end
    local bit16 = bit32.lshift(1, 16)
    local okrun = ZSP.run("t23_nmi", { timeout_ms = 500, on_release = function()
        hwstub.mdelay(opts.delay_ms or 5)
        note(string.format("CHIPCFG before %08x", rd(CHIPCFG)))
        wr(CHIPCFG, bit32.bor(rd(CHIPCFG), bit16))
        note(string.format("CHIPCFG set    %08x", rd(CHIPCFG)))
        hwstub.mdelay(opts.hold_ms or 1)
        wr(CHIPCFG, bit32.band(rd(CHIPCFG), bit32.bnot(bit16)))
        note(string.format("CHIPCFG clear  %08x", rd(CHIPCFG)))
    end })
    for off = 0x100, 0x11c, 4 do
        local okr, v = pcall(rd, INTC0 + off)
        note(string.format("INTC0+%03x %s", off, okr and string.format("%08x", v) or "read error"))
    end
    local f = assert(io.open(ZSP.rdir() .. "/t23_arm.txt", "w"))
    f:write(table.concat(log, "\n") .. "\n")
    f:close()
    return okrun
end

-- Order: safe ones first. t22/t23 load 64 KiB images (slow over USB).
ZSP.IRQ = { "t22_vec_e", "t22_vec_f", "t22_vec_g", "t22_vec_h", "t22_vec_j",
            "t22_vec_s1", "t22_vec_s13", "t22_vec_s14", "t22_vec_s15" }

-- t22_vec_m: the ARM raises the mailbox interrupt (H2C0) while the DSP spins
-- with %imask = 8010. Logs H2C_STA around it to results/t22_mbox_arm.txt.
local MBOX0 = 0x18088000
function ZSP.mbox_test(opts)
    opts = opts or {}
    local c = ZSP.cfg
    local log = {}
    local function note(s) log[#log + 1] = s print("  " .. s) end
    note(string.format("MAILBOX_ID %08x H2C_STA %08x C2H_STA %08x", rd(MBOX0), rd(MBOX0 + 0x10), rd(MBOX0 + 0x40)))
    local okrun = ZSP.run("t22_vec_m", { timeout_ms = 500, on_release = function()
        hwstub.mdelay(opts.delay_ms or 2)
        wr(MBOX0 + 0x20, 0x11223344)                 -- H2C0 data
        note(string.format("after H2C0  write: H2C_STA %08x", rd(MBOX0 + 0x10)))
        wr(MBOX0 + 0x24, 0x55667788)                 -- H2C0b command: raises the interrupt
        note(string.format("after H2C0b write: H2C_STA %08x", rd(MBOX0 + 0x10)))
    end })
    note(string.format("after run: H2C_STA %08x H2C0 %08x H2C0b %08x", rd(MBOX0 + 0x10), rd(MBOX0 + 0x20), rd(MBOX0 + 0x24)))
    local f = assert(io.open(ZSP.rdir() .. "/t22_mbox_arm.txt", "w"))
    f:write(table.concat(log, "\n") .. "\n")
    f:close()
    return okrun
end
-- EPR block (ARM 0x30080000): which halfwords are writable? The 16-byte
-- pattern repeats, so 8 halfwords are probed, and the mirror at +0x10 is
-- read after each write. Logs to results/epr_probe.txt; returns
-- { [offset] = writable mask }.
local EPR = 0x30080000
function ZSP.epr_probe()
    local c = ZSP.cfg
    ZSP.hold()
    local log, wmask = {}, {}
    local function note(s) log[#log + 1] = s print("  " .. s) end
    for off = 0, 14, 2 do
        local a = EPR + off
        local o = DEV.read16(a)
        DEV.write16(a, 0xFFFF)
        local f, m = DEV.read16(a), DEV.read16(a + 0x10)
        DEV.write16(a, 0x0000)
        local z = DEV.read16(a)
        DEV.write16(a, o)
        local back = DEV.read16(a)
        note(string.format("+%02x: orig %04x  ffff->%04x (mirror +10: %04x)  0000->%04x  restored %04x",
            off, o, f, m, z, back))
        if f ~= z then wmask[off] = bit32.bxor(f, z) end
    end
    -- 32-bit view of the first word, in case the block wants word writes
    local o = DEV.read32(EPR)
    DEV.write32(EPR, 0xFFFFFFFF)
    local f = DEV.read32(EPR)
    DEV.write32(EPR, o)
    note(string.format("32-bit +00: orig %08x  ffffffff->%08x  restored %08x", o, f, DEV.read32(EPR)))
    local fh = assert(io.open(ZSP.rdir() .. "/epr_probe.txt", "w"))
    fh:write(table.concat(log, "\n") .. "\n")
    fh:close()
    return wmask
end

-- For every writable EPR halfword, set each candidate value, run
-- t26_ext_none (DSP reads above DMEM) and save it as
-- results/t26_epr_<off>_<value>.txt; the original value is restored after.
ZSP.EPR_VALUES = { 0x0001, 0x8000, 0xFFFF, 0x1808, 0x0C04, 0x1880 }
function ZSP.epr_try(values)
    local w = ZSP.epr_probe()
    local any = false
    for off = 0, 14, 2 do
        if w[off] then
            any = true
            local a = EPR + off
            local o = DEV.read16(a)
            for _, v in ipairs(values or ZSP.EPR_VALUES) do
                local name = string.format("t26_epr_%02x_%04x", off, v)
                -- peripheral reset first (it might reset the EPR block too),
                -- then the EPR value, then the run without another setup
                ZSP.setup({ quiet = true })
                DEV.write16(a, v)
                local keep = ZSP.cfg.setup_each_run
                ZSP.cfg.setup_each_run = false
                ZSP.run("t26_ext_none", { save_as = name, timeout_ms = 300 })
                ZSP.cfg.setup_each_run = keep
                print(string.format("  EPR+%02x = %04x, reads %04x after the run", off, v, DEV.read16(a)))
            end
            DEV.write16(a, o)
        end
    end
    if not any then print("no writable EPR halfword; nothing to try") end
end

-- t27_ext / t28_mbox (default): the DSP reads the mailbox through the EPR
-- window and writes C2H0/C2H1.
-- The ARM preloads H2C0 = 11223344, H2C0b = 55667788 and checks afterwards.
-- Log: results/t2N_arm.txt
function ZSP.ext_test(name)
    name = name or "t29_mbox"
    local c = ZSP.cfg
    local log = {}
    local function note(s) log[#log + 1] = s print("  " .. s) end
    local function mb(off) return rd(MBOX0 + off) end
    note(string.format("before: ID %08x H2C_STA %08x C2H_STA %08x", mb(0), mb(0x10), mb(0x40)))
    wr(MBOX0 + 0x20, 0x11223344)
    wr(MBOX0 + 0x24, 0x55667788)
    note(string.format("after H2C0/H2C0b writes: H2C_STA %08x", mb(0x10)))
    local okrun = ZSP.run(name, { timeout_ms = 300 })
    note(string.format("after run: H2C_STA %08x C2H_STA %08x", mb(0x10), mb(0x40)))
    note(string.format("C2H0 %08x C2H1 %08x", mb(0x50), mb(0x58)))
    note(string.format("C2H0b %08x C2H1b %08x (read clears)", mb(0x54), mb(0x5c)))
    note(string.format("after reading C2Hxb: C2H_STA %08x", mb(0x40)))
    local f = assert(io.open(ZSP.rdir() .. "/" .. name:sub(1, 3) .. "_arm.txt", "w"))
    f:write(table.concat(log, "\n") .. "\n")
    f:close()
    return okrun
end

-- t33_sdstore: SDRAM store timing in hwstub's scratch buffer. Picks a
-- 4 KiB-aligned bus address A in the buffer, passes the window parameters to
-- the DSP at DMEM 0x7F0, fills A..A+0x3FF with 0xEEEE first and dumps it
-- afterwards to results/t33_sdram.txt for tests/expect.py.
-- hwstub stages USB transfers in this buffer, so data written there does
-- not survive the result collection; the block is placed 16 KiB in (if the
-- buffer is big enough) to stay clear of the transfers made during the run.
function ZSP.sdstore_test(name)
    name = name or "t33_sdstore"
    local c = ZSP.cfg
    local buf = hwstub.dev.layout.buffer
    local skip = (buf.size >= 0x5000) and 0x4000 or 0
    local A = bit32.band(buf.start + skip + 0xFFF, bit32.bnot(0xFFF))
    if A < 0x60000000 or A >= 0x70000000 or A + 0x400 > buf.start + buf.size then
        print(string.format("hwstub buffer %08x+%x is not usable SDRAM; not running", buf.start, buf.size))
        return false
    end
    if bit32.band(A, 0x3FFFF) + 0x400 > 0x40000 then
        print(string.format("A = %08x: block would cross a 256 KiB window slice; not running", A))
        return false
    end
    local n = bit32.extract(A, 18, 3)
    local epr = bit32.rshift(A, 21)
    local dsp = 0xD00000 + bit32.rshift(bit32.band(A, 0x1FFFFF), 1)
    print(string.format("hwstub buffer %08x+%x; SDRAM test block at %08x: EPR[%d] = %03x, DSP address %06x",
        buf.start, buf.size, A, n, epr, dsp))
    for off = 0, 0x3FC, 4 do wr(A + off, 0xEEEEEEEE) end
    local okrun = ZSP.run(name, { timeout_ms = 1000, before_release = function()
        ZSP.dwrite(c.DMEM_DSP + 0x7F0, n)
        ZSP.dwrite(c.DMEM_DSP + 0x7F1, epr)
        ZSP.dwrite(c.DMEM_DSP + 0x7F2, bit32.band(dsp, 0xFFFF))
        ZSP.dwrite(c.DMEM_DSP + 0x7F3, bit32.rshift(dsp, 16))
    end })
    if name ~= "t33_sdstore" then return okrun end
    local f = assert(io.open(ZSP.rdir() .. "/t33_sdram.txt", "w"))
    f:write(string.format("A %08x EPR %d %03x DSP %06x\n", A, n, epr, dsp))
    for off = 0, 0x3FF, 0x20 do
        local line = {}
        for k = off, off + 0x1E, 2 do line[#line + 1] = string.format("%04x", DEV.read16(A + k)) end
        f:write(string.format("%03x: %s\n", off, table.concat(line, " ")))
    end
    f:close()
    print("wrote results/t33_sdram.txt")
    return okrun
end

-- t35_dma: DWDMA SDRAM -> DMEM. Source = 4 KiB in hwstub's buffer (16 KiB
-- in), filled with halfword k = k; its bus address goes to DMEM 0x7F2/0x7F3.
function ZSP.dma_test(name)
    name = name or "t35_dma"
    local c = ZSP.cfg
    local buf = hwstub.dev.layout.buffer
    local skip = (buf.size >= 0x6000) and 0x4000 or 0
    local A = bit32.band(buf.start + skip + 0xFFF, bit32.bnot(0xFFF))
    if A < 0x60000000 or A >= 0x70000000 or A + 0x1000 > buf.start + buf.size then
        print(string.format("hwstub buffer %08x+%x is not usable SDRAM; not running", buf.start, buf.size))
        return false
    end
    print(string.format("hwstub buffer %08x+%x; DMA source at %08x", buf.start, buf.size, A))
    -- DWDMA clock: SCU_CLKCFG bit 4 = 1 gates it (ZSP.restore() puts CLKCFG back)
    if not saved.clkcfg then ZSP.setup({ quiet = true }) end
    print(string.format("CLKCFG %08x (bit 4 = DW DMA clock off)", rd(CLKCFG)))
    wr(CLKCFG, bit32.band(rd(CLKCFG), bit32.bnot(0x10)))
    print(string.format("CLKCFG %08x after enabling the DW DMA clock", rd(CLKCFG)))
    for j = 0, 0x3FF do wr(A + 4 * j, (2 * j) + (2 * j + 1) * 0x10000) end
    local okrun = ZSP.run(name, { timeout_ms = 2000, before_release = function()
        ZSP.dwrite(c.DMEM_DSP + 0x7F2, bit32.band(A, 0xFFFF))
        ZSP.dwrite(c.DMEM_DSP + 0x7F3, bit32.rshift(A, 16))
    end })
    local bad = 0
    for j = 0, 0x3FF do
        if rd(A + 4 * j) ~= (2 * j) + (2 * j + 1) * 0x10000 then bad = bad + 1 end
    end
    print(string.format("source block after the run: %d words changed (hwstub traffic)", bad))
    return okrun
end

-- t36_dma2: DMEM -> SDRAM and DMA with concurrent DSP bus traffic.
-- Blocks A (source, halfword k = k) and B = A + 0x1000 (destination, 0xEEEE)
-- in hwstub's buffer; both must be in one 256 KiB window slice other than 3
-- (slice 3 maps the DWDMA registers).
function ZSP.dma2_test()
    local c = ZSP.cfg
    local buf = hwstub.dev.layout.buffer
    local skip = (buf.size >= 0x7000) and 0x4000 or 0
    local A = bit32.band(buf.start + skip + 0xFFF, bit32.bnot(0xFFF))
    local B = A + 0x1000
    if A < 0x60000000 or A >= 0x70000000 or B + 0x1000 > buf.start + buf.size then
        print(string.format("hwstub buffer %08x+%x is not usable SDRAM; not running", buf.start, buf.size))
        return false
    end
    local n = bit32.extract(A, 18, 3)
    if n ~= bit32.extract(B + 0xFFF, 18, 3) or n == 3 then
        print(string.format("A = %08x: blocks cross a slice or use slice 3; not running", A))
        return false
    end
    local epr = bit32.rshift(A, 21)
    local function win(x) return 0xD00000 + bit32.rshift(bit32.band(x, 0x1FFFFF), 1) end
    print(string.format("hwstub buffer %08x+%x; A %08x B %08x, EPR[%d] = %03x, DSP %06x / %06x",
        buf.start, buf.size, A, B, n, epr, win(A), win(B)))
    if not saved.clkcfg then ZSP.setup({ quiet = true }) end
    print(string.format("CLKCFG %08x (bit 4 = DW DMA clock off)", rd(CLKCFG)))
    wr(CLKCFG, bit32.band(rd(CLKCFG), bit32.bnot(0x10)))
    for j = 0, 0x3FF do
        wr(A + 4 * j, (2 * j) + (2 * j + 1) * 0x10000)
        wr(B + 4 * j, 0xEEEEEEEE)
    end
    local P = c.DMEM_DSP + 0x7F0
    local okrun = ZSP.run("t36_dma2", { timeout_ms = 3000, before_release = function()
        local v = { n, epr, A % 0x10000, bit32.rshift(A, 16), B % 0x10000, bit32.rshift(B, 16),
                    win(A) % 0x10000, bit32.rshift(win(A), 16), win(B) % 0x10000, bit32.rshift(win(B), 16) }
        for k, x in ipairs(v) do ZSP.dwrite(P + k - 1, x) end
    end })
    -- B as the ARM sees it: words 0..15 hold the DMEM -> SDRAM data of the
    -- last d2s case, overwritten from 0 by the 64 std of the '+st' cases
    local f = assert(io.open(ZSP.rdir() .. "/t36_sdram.txt", "w"))
    f:write(string.format("A %08x B %08x\n", A, B))
    for off = 0, 0xFFF, 0x20 do
        local line = {}
        for k = off, off + 0x1E, 2 do line[#line + 1] = string.format("%04x", DEV.read16(B + k)) end
        f:write(string.format("%03x: %s\n", off, table.concat(line, " ")))
    end
    f:close()
    return okrun
end

-- t38_armirq: DSP -> ARM signals. Handshake through DMEM 0x7F8; phase
-- reached at RES+0x41. Logs the ARM interrupt controller (ISR/IPR/IMR),
-- mailbox flags and CHIPCFG at each step to results/t38_arm.txt.
function ZSP.armirq_test()
    local c = ZSP.cfg
    local log = {}
    local function note(s) log[#log + 1] = s print("  " .. s) end
    local function intc(tag)
        note(string.format("%-28s ISR %02x IPR %08x IMR %08x | C2H_STA %x | CHIPCFG %08x", tag,
            rd(INTC0 + 0x104), rd(INTC0 + 0x108), rd(INTC0 + 0x10C), rd(MBOX0 + 0x40), rd(CHIPCFG)))
    end
    local function wait_phase(p)
        for _ = 1, 500 do
            if ZSP.dread(c.DMEM_DSP + c.RES_OFF + 0x41) == p then return true end
            hwstub.mdelay(1)
        end
        note("timeout waiting for DSP phase " .. p)
        return false
    end
    local cc = rd(CHIPCFG)
    -- let sources 7 (mailbox, AHB0) and 28 (DSP) into INTC_IPR without
    -- unmasking them in INTC_IMR, so the ARM core never sees an interrupt
    local IECR, ICCR = INTC0 + 0x114, INTC0 + 0x118
    local SRC = bit32.bor(bit32.lshift(1, 7), bit32.lshift(1, 28))
    local iecr = rd(IECR)
    note(string.format("IECR %08x -> %08x (IMR untouched)", iecr, bit32.bor(iecr, SRC)))
    wr(IECR, bit32.bor(iecr, SRC))
    wr(ICCR, SRC)
    local okrun = ZSP.run("t38_armirq", { timeout_ms = 1000,
        before_release = function()
            ZSP.dwrite(c.DMEM_DSP + 0x7F4, bit32.band(cc, 0xFFFF))
            ZSP.dwrite(c.DMEM_DSP + 0x7F5, bit32.rshift(cc, 16))
            ZSP.dwrite(c.DMEM_DSP + 0x7F8, 0)
        end,
        on_release = function()
            intc("before")
            if not wait_phase(1) then return end
            intc("after DSP wrote C2H0b")
            note(string.format("C2H0 %08x C2H0b %08x (read clears)", rd(MBOX0 + 0x50), rd(MBOX0 + 0x54)))
            intc("after ARM read C2H0b")
            wr(ICCR, SRC)
            intc("after ICCR clear")
            ZSP.dwrite(c.DMEM_DSP + 0x7F8, 1)
            if not wait_phase(2) then return end
            intc("after DSP set CHIPCFG[17]")
            ZSP.dwrite(c.DMEM_DSP + 0x7F8, 2)
            if not wait_phase(3) then return end
            intc("after DSP cleared it")
            wr(ICCR, SRC)
            intc("after ICCR clear")
        end })
    -- make sure bit 17 is clear whatever happened; restore IECR
    wr(CHIPCFG, bit32.band(rd(CHIPCFG), bit32.bnot(bit32.lshift(1, 17))))
    wr(ICCR, SRC)
    wr(IECR, iecr)
    intc("end")
    local f = assert(io.open(ZSP.rdir() .. "/t38_arm.txt", "w"))
    f:write(table.concat(log, "\n") .. "\n")
    f:close()
    return okrun
end

ZSP.ROUND4 = { "t48_src14", "t49_stdshort", "t50_nest", "t51_circ5" }

ZSP.ROUND5 = { "t52_cexe", "t53_vxor_a", "t53_vxor_b", "t53_vxor_c" }

-- t54: %smode bits 1 and 15 wake-up. The NMI variants pulse CHIPCFG[16]
-- 10 ms after release.
function ZSP.smode_test()
    for _, n in ipairs({ "t54_s1_t500", "t54_s1_t2000", "t54_s1_t8000" }) do ZSP.run(n) end
    local bit16 = bit32.lshift(1, 16)
    for _, n in ipairs({ "t54_b15", "t54_s1_nmi" }) do
        ZSP.run(n, { timeout_ms = 500, on_release = function()
            hwstub.mdelay(10)
            wr(CHIPCFG, bit32.bor(rd(CHIPCFG), bit16))
            hwstub.mdelay(1)
            wr(CHIPCFG, bit32.band(rd(CHIPCFG), bit32.bnot(bit16)))
        end })
    end
end

-- t58: %hwflag bit 2; pulses the NMI 5 ms after release.
function ZSP.hwflag_test()
    local bit16 = bit32.lshift(1, 16)
    return ZSP.run("t58_hwflag2d", { timeout_ms = 500, on_release = function()
        hwstub.mdelay(5)
        wr(CHIPCFG, bit32.bor(rd(CHIPCFG), bit16))
        hwstub.mdelay(1)
        wr(CHIPCFG, bit32.band(rd(CHIPCFG), bit32.bnot(bit16)))
    end })
end

ZSP.ROUND6 = { "t61_cexe2", "t62_nestprio", "t63_circ6" }

ZSP.REPRO = { "t32_a", "t32_b", "t32_c", "t32_d", "t32_e", "t32_f" }

ZSP.EXT = {}
for _, v in ipairs({ "none", "all", "b0", "b2", "b3", "b4", "b5", "b6", "b7", "b8", "b9", "b10", "b11" }) do
    ZSP.EXT[#ZSP.EXT + 1] = "t26_ext_" .. v
end

ZSP.SMODE = {}
for n = 0, 15 do ZSP.SMODE[#ZSP.SMODE + 1] = "t25_smode_" .. n end

ZSP.ALL = { "t0c_units", "t1_call", "t2_branch", "t3_regs", "t4_mem", "t5_imm",
            "t6_creg", "t7_xbits", "t7_xbits_ref", "t8_alu", "t9_cexe",
            "t12_hazards", "t13_inferred", "t14_settle", "t15_rest", "t16_flags", "t17_circ", "t17b_circ", "t20_final" }
ZSP.BENCH = {
    "nop", "add_dep", "add_indep4", "add_imm4", "add3", "mov_imm16",
    "shll_imm", "shll_reg", "mul16", "mac_dep", "mac_indep2", "mov_a",
    "add_a_imm", "ld", "ld_indep4", "ldd", "ldq", "ld_short",
    "ld_disp16", "ldu_pm", "ldx", "st", "st_indep4", "std",
    "stq", "stu_pm", "ld_ld_near", "ld_ld_far", "ld_st", "push_pop",
    "load_use", "load_nouse", "aload_use", "aload_nouse", "st_ld_same", "st_ld_diff",
    "mac_use", "mac_nouse", "mul_use", "mul_nouse", "creg_use", "creg_nouse",
    "br_next", "bz_taken", "bz_nottaken", "cmp_only", "call_ret", "cexe_true",
    "cexe_false",
}
-- mixed groups typical of DSP filter code (window taps and DCT steps of
-- an MP3 subband synthesis), for the timing model of tools/zsp/zspsim.py:
--   ZSP.bench(ZSP.BENCH_MIX, { append = true })
ZSP.BENCH_MIX = {
    "adde_dep", "adde_indep2", "shrae", "move_indep2", "move_add2",
    "move_add2b", "adde_add", "mulsu",
    "ldu_2areg", "lddu_ldu", "ld2_mac2", "win_tap", "mac_ld", "ldd_std",
    "mulsu_chain", "dct_rot", "dct_bfly",
}
-- may hang the DSP (not the host): run separately
ZSP.RISKY = { "t19_jump_0", "t19_jump_1", "t19_jump_2", "t19_jump_3", "t19_jump_4", "t19_jump_5",
              "t18_cp_0", "t18_cp_1", "t18_cp_2", "t18_cp_3", "t18_cp_4", "t18_cp_5",
              "t18_cp_6", "t18_cp_7", "t18_cp_8", "t2b_bgt32", "t10_E000", "t10_DC00", "t10_0100", "t10_7941", "t10_FE00" }


---------------------------------------------------------------------------
-- Test groups: ZSP.run_group(name) runs one area in a safe order;
-- ZSP.run_group("all") runs every group except "bench". Entries are test
-- names (run with ZSP.run) or functions. See README.md for what each
-- group establishes and tests/expect.py for checking the results.
---------------------------------------------------------------------------
local function names(prefix, list)
    local t = {}
    for _, v in ipairs(list) do t[#t + 1] = prefix .. v end
    return t
end

ZSP.GROUPS = {
    -- bring-up history: DSP-side DMEM address, units, store lag (t0a/t0b need no MAGIC)
    -- t0a/t0b/t0d-t0g end without MAGIC: they run for a while and their DMEM
    -- is printed (no result file, nothing for tests/expect.py), as each
    -- test's header says
    startup = { function() ZSP.run_for("t0a_loop", 50) end,
                function() ZSP.run_for("t0b_probe", 50, { pattern = true }) ZSP.scan() end,
                "t0c_units",
                function() ZSP.run_for("t0d_hazard") ZSP.scan() end,
                function() ZSP.run_for("t0e_storelag") ZSP.scan() end,
                function() ZSP.run_for("t0f_storedata") ZSP.scan() end,
                function() ZSP.run_for("t0g_loads", 50, { pattern = true }) ZSP.dump(0xa0, 8) end,
                function() ZSP.run("t0h_slot0") ZSP.dump(0x100, 16) end },
    -- instruction set: encodings, semantics, flags, conditional execution
    isa = ZSP.ALL,
    -- may hang the DSP (not the host); each runs after a fresh ZSP.setup()
    risky = ZSP.RISKY,
    timers = { "t11_timers", table.unpack(names("t11b_", { "0001", "0002", "0004", "0008", "0010", "0020",
               "0040", "0080", "0100", "0200", "0400", "0800", "1000", "2000", "4000", "8000" })) },
    -- interrupts: sources, vectors, priorities, nesting, low-power modes, flags
    irq = { "t21_ireq", "t22_vec_a", "t22_vec_i", table.unpack(ZSP.IRQ) },
    irq2 = { function() ZSP.nmi_test() end, function() ZSP.mbox_test() end,
             "t45_reti_smode", "t46_prio", "t47_prio2", "t48_src14", "t50_nest", "t62_nestprio",
             function() ZSP.smode_test() end,
             "t55_hwflag2", "t56_hwflag2b", "t57_hwflag2c", function() ZSP.hwflag_test() end,
             "t59_bisect", "t60_hwflag2e" },
    smode = ZSP.SMODE,
    -- conditional execution and remaining instruction details
    misc = { "t49_stdshort", "t52_cexe", "t53_vxor_a", "t53_vxor_b", "t53_vxor_c", "t61_cexe2" },
    -- addressing: circular-buffer search and bit-reversed addressing
    addr = { "t39_circ", "t40_circ2", "t41_circ3", "t42_brev", "t43_circ4", "t44_brevst", "t51_circ5", "t63_circ6" },
    -- external bus: data space, EPR window, mailbox, timing, SDRAM, DMA, DSP -> ARM
    bus = { "t24_space", function() ZSP.peek_epr() end, table.unpack(ZSP.EXT) },
    bus2 = { function() ZSP.epr_try() end, function() ZSP.ext_test("t27_ext") end,
             function() ZSP.ext_test("t28_mbox") end, function() ZSP.ext_test("t29_mbox") end,
             "t30_bustime", "t31_bususe", table.unpack(ZSP.REPRO) },
    bus3 = { function() ZSP.sdstore_test("t33_sdstore") end, function() ZSP.sdstore_test("t34_sdbuf") end,
             function() ZSP.dma_test("t35_dma") end, function() ZSP.dma2_test() end,
             function() ZSP.dma_test("t37_dmairq") end, function() ZSP.armirq_test() end },
    -- cycle timing (minutes): needs make bench first
    bench = { function() ZSP.bench() end },
}
-- Bus and DMA timing at several DSP clocks: every test of opts.tests (names
-- or functions, default ZSP.SWEEP_TESTS) at every clock of opts.mhz (default
-- ZSP.SWEEP_MHZ), results in results/sweep/<MHz>/ (tests/expect.py takes the
-- directory; tools/bus_sweep.py fits cycles = fixed + ns x clock). The ARM
-- clock (HCLK, SDRAM) stays as it is. The DSP clock goes back afterwards.
ZSP.SWEEP_MHZ = { 18, 24, 48, 96, 160 }
ZSP.SWEEP_TESTS = { "t30_bustime",
                    function() ZSP.dma_test("t35_dma") end, function() ZSP.dma2_test() end }
function ZSP.sweep(opts)
    opts = opts or {}
    local c = ZSP.cfg
    local keep_mhz, keep_res = c.dsp_mhz, c.results
    local ok, err = xpcall(function()
        for _, mhz in ipairs(opts.mhz or ZSP.SWEEP_MHZ) do
            c.dsp_mhz = mhz
            ZSP.setup({ quiet = true })
            local dsp, arm, hclk = ZSP.clocks()
            c.results = string.format("results/sweep/%g", mhz)
            print(string.format("=== DSP %.2f MHz, ARM %.0f MHz, HCLK %.0f MHz -> %s",
                dsp / 1e6, arm / 1e6, hclk / 1e6, c.results))
            for _, t in ipairs(opts.tests or ZSP.SWEEP_TESTS) do
                if type(t) == "function" then t() else ZSP.run(t, { timeout_ms = 2000 }) end
            end
        end
    end, debug.traceback)
    c.dsp_mhz, c.results = keep_mhz, keep_res
    if saved.pllcon2 then set_pllcon(bit32.band(saved.pllcon2, bit32.bnot(bit32.lshift(1, 22)))) end
    if not ok then print("ERROR: " .. tostring(err)) end
end

ZSP.GROUP_ORDER = { "startup", "isa", "timers", "risky", "irq", "irq2", "smode", "misc", "addr", "bus", "bus2", "bus3" }

function ZSP.run_group(name)
    if name == "all" then
        for _, g in ipairs(ZSP.GROUP_ORDER) do ZSP.run_group(g) end
        return
    end
    local g = assert(ZSP.GROUPS[name], "unknown group " .. tostring(name))
    print("=== group " .. name)
    for _, t in ipairs(g) do
        if type(t) == "function" then t() else ZSP.run(t) end
    end
end

print("zsp loaded: ZSP.setup(), ZSP.run(name), ZSP.run_group(name|\"all\"), ZSP.restore()")
