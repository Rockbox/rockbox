--
-- RK27xx clock configurations: which clocks a SCU_PLLCON1 / SCU_DIVCON1
-- setting really gives, and whether switching keeps PCLK steady.
--
-- Usage (inside hwstub_shell):
--   require 'rk27xx/clk'
--   CLK.table()      -- ARM, PCLK and AHB access rate of each configuration
--   CLK.switch()     -- PCLK while switching slow (idle) <-> PLL 48 MHz
--   CLK.od()         -- PLL 48 <-> 192 MHz: output divider change and lock
--   CLK.set("normal") -- stay at Rockbox's normal clocks: ARM 50, HCLK 50 MHz
--   CLK.set("hwstub") -- back to hwstub's: ARM 200, HCLK 100 MHz
--
-- CLK.set() is plain register writes from here, the sequence Rockbox's
-- set_cpu_frequency() uses; only the measurements need the kernel below.
--
-- The kernel (rk27xx/clk_test.s, embedded below; rebuild with
--   arm-elf-eabi-as -mcpu=arm926ej-s clk_test.s -o clk.o &&
--   arm-elf-eabi-objcopy -O binary clk.o clk.bin && xxd -p -c 32 clk.bin)
-- runs a list of ops from IRAM with the cache on and IRQs masked, and puts
-- the original clocks back at the end. While the list runs, the SDRAM
-- refresh is set as for 12 MHz HCLK (the value set_cpu_frequency() uses
-- while switching) so that it is frequent enough at any HCLK.
--
-- Clocks are measured against host time: a call is timed with n and with
-- 2n loop passes, and the difference removes the USB overhead. The core
-- clock is a cached subs/bne loop, calibrated on hwstub's own setting
-- (ARM 200 MHz); PCLK is TMR1; "AHB reads/us" are uncached INTC reads.
--
-- Measured on a YP-CP3 (REV_B): HCLK is always half the source (PLL, or
-- 24 MHz in slow mode); SCU_DIVCON1 bit 2 = 0: ARM = source, bit 2 = 1:
-- ARM = source / 2 (= HCLK); bits 4:3: PCLK = HCLK / 1, 2, 4.
--

CLK = {}

local DEV = DEV or hwstub.dev

local DIVCON1   = 0x1801c014
local PLLCON1   = 0x1801c008
local TMR1LR    = 0x18000010
local TMR1CVR   = 0x18000014
local TMR1CON   = 0x18000018
local PARAM, CODE, OPS, RES = 0x700, 0x800, 0xc00, 0xe00

local OP = { DIV = 1, PLL = 2, LOCK = 3, SPIN = 4, AHB = 5, STAMP = 6,
             LOOP = 7, ENDLOOP = 8, TREF = 9, STATUS = 10 }
local TREF_12MHZ = 187

local BLOB =
    "f04f2de907cca0e300b00fe1c0008be300f021e106a3a0e3d0919fe5d0819fe5" ..
    "140099e500008ce5080099e504008ce5000098e508008ce5b8619fe5b8019fe5" ..
    "100086e5140086e5b0019fe5180086e5ac019fe51c0086e50200a0e3040086e5" ..
    "041096e5030011e3fcffff1a020180e3040086e5041096e5030011e3fcffff1a" ..
    "000096e5020180e3000086e5037ba0e30e6ca0e3035ba0e30040a0e3040097e4" ..
    "041097e40a0050e300f18f903a0000ea390000ea080000ea0c0000ea0d0000ea" ..
    "1a0000ea1e0000ea240000ea260000ea280000ea2b0000ea2c0000ea142099e5" ..
    "1f20c2e3012082e1142089e5eaffffea081089e5e8ffffea14309ae50126a0e3" ..
    "2c0099e5010010e30400001a012052e2faffff1a0000e0e3010186e7deffffea" ..
    "14009ae5000043e0010186e7daffffea000051e3d8ffff0a011051e2fdffff1a" ..
    "d5ffffea000051e3d3ffff0ab4209fe5000092e5011051e2fcffff1aceffffea" ..
    "14009ae5010186e7cbffffea0750a0e10040a0e3c8ffffea014084e2010054e1" ..
    "0570a031c4ffffea001088e5c2ffffea2c0099e5010186e7bfffffea140099e5" ..
    "010080e3140089e504009ce5080089e50126a0e32c0099e5010010e30100001a" ..
    "012052e2faffff1a00009ce5140089e508009ce5000088e518609fe5000096e5" ..
    "0201c0e3000086e50bf021e1f08fbde800c0011810010b180000ffeffe000018" ..
    "fe000030fe0000ee08010818"

local function rd(a) return DEV.read32(a) end
local function wr(a, v) DEV.write32(a, v) end

local function hex2bin(h)
    return (h:gsub("%x%x", function(c) return string.char(tonumber(c, 16)) end))
end

local function words(s)
    local t = {}
    for i = 1, #s, 4 do
        local a, b, c, d = s:byte(i, i + 3)
        t[#t + 1] = a + b * 0x100 + c * 0x10000 + d * 0x1000000
    end
    return t
end

local function now()
    local p = io.popen("date +%s.%N")
    local t = tonumber(p:read("*l"))
    p:close()
    return t
end

-- SCU_PLLCON1 for Fout = 24 MHz / nr * nf / od (VCO 24/nr*nf: 120..600)
local function pll(nr, nf, od)
    return 0x01800000 + (nr - 1) * 0x10000 + (nf - 1) * 0x10 + (od - 1) * 2
end

local loaded = false
local orig_div

local function setup()
    if not loaded then
        local code = hex2bin(BLOB)
        local w = words(code)
        for i, v in ipairs(w) do wr(CODE + (i - 1) * 4, v) end
        local chk = words(DEV.read(CODE, #code))
        for i, v in ipairs(w) do
            if chk[i] ~= v then error(string.format("IRAM verify failed @ %#x", CODE + (i - 1) * 4)) end
        end
        loaded = true
    end
    wr(TMR1CON, 0)
    wr(TMR1LR, 0xffffffff)
    wr(TMR1CON, 0x100)
    orig_div = bit32.band(rd(DIVCON1), 0x1f)
end

-- run an op list (pairs), return host seconds and the result slots
local function run(ops, nslots)
    local t = {}
    for _, o in ipairs(ops) do t[#t + 1] = o[1]; t[#t + 1] = o[2] or 0 end
    t[#t + 1] = 0; t[#t + 1] = 0
    if #t > 128 then error("op list too long") end
    for i, v in ipairs(t) do wr(OPS + (i - 1) * 4, v) end
    for i = 0, (nslots or 8) - 1 do wr(RES + i * 4, 0) end
    local h0 = now()
    local ok, err = pcall(DEV.call, CODE)
    local h1 = now()
    if not ok then error(string.format("call failed after %.2f s: %s - chip hung? reset it", h1 - h0, tostring(err))) end
    return h1 - h0, words(DEV.read(RES, (nslots or 8) * 4))
end

local function ticks(a, b) return (a - b) % 0x100000000 end    -- TMR1 counts down

-- the ops that get to a configuration: slow mode, PLL, lock, dividers
local function enter(cfg)
    local ops = { { OP.TREF, TREF_12MHZ }, { OP.DIV, bit32.bor(orig_div, 1) } }
    if cfg.pll then
        ops[#ops + 1] = { OP.PLL, cfg.pll }
        ops[#ops + 1] = { OP.LOCK, 7 }
    end
    ops[#ops + 1] = { OP.DIV, cfg.div }
    return ops
end

local function append(a, b) for _, x in ipairs(b) do a[#a + 1] = x end return a end

-- core loop passes per second, PCLK and AHB reads per us in a configuration
local function measure(cfg)
    local function once(n, m)
        local ops = append(enter(cfg), { { OP.STAMP, 0 }, { OP.SPIN, n }, { OP.STAMP, 1 },
                                         { OP.AHB, m }, { OP.STAMP, 2 } })
        local h, r = run(ops)
        if r[8] == 0xffffffff then error(cfg.name .. ": PLL did not lock") end
        return h, ticks(r[1], r[2]), ticks(r[2], r[3])
    end
    local h = once(200000, 0)                     -- rough speed
    local n = math.max(200000, math.floor(200000 * 0.15 / math.max(h - 0.004, 0.002)))
    -- both calls do the same AHB reads, so only n core loop passes differ
    local h1, t1 = once(n, 20000)
    local h2, t2, ta = once(2 * n, 20000)
    local dt = h2 - h1
    local pclk = (t2 - t1) / dt
    return { passes = n / dt, pclk = pclk, ahb = 20000 / (ta / pclk) / 1e6 }
end

CLK.CONFIGS = {
    { name = "hwstub: PLL 200, 0x08",        pll = pll(6, 50, 1), div = 0x08, ref = true },
    { name = "Rockbox normal: PLL 100, 0x04",   pll = pll(6, 50, 2), div = 0x04 },
    { name = "slow, 0x05",                      div = 0x05 },
    { name = "slow, 0x0d",                      div = 0x0d },
    { name = "slow, 0x09",                      div = 0x09 },
    { name = "PLL 96, 0x14",                    pll = pll(6, 48, 2), div = 0x14 },
    { name = "PLL 96, 0x10",                    pll = pll(6, 48, 2), div = 0x10 },
    { name = "PLL 48, 0x0c",                    pll = pll(6, 48, 4), div = 0x0c },
    { name = "PLL 48, 0x00",                    pll = pll(6, 48, 4), div = 0x00 },
    { name = "PLL 192, 0x10",                   pll = pll(6, 48, 1), div = 0x10 },
    { name = "PLL 192, 0x08",                   pll = pll(6, 48, 1), div = 0x08 },
}

-- DIVCON1[4:0] as measured (see the top)
local function ratios(div)
    local hp = ({ [0] = "1", "2", "4", "?" })[bit32.band(bit32.rshift(div, 3), 3)]
    return string.format("%sARM = src%s, HCLK = src/2, PCLK = HCLK/%s", bit32.band(div, 1) ~= 0 and "slow: " or "",
        bit32.band(div, 4) ~= 0 and "/2" or "", hp)
end

function CLK.table()
    setup()
    local ref
    local res = {}
    for _, c in ipairs(CLK.CONFIGS) do
        local m = measure(c)
        if c.ref then ref = m end
        res[#res + 1] = { c, m }
        local arm = m.passes * 200e6 / ref.passes
        print(string.format("%-34s ARM %6.1f MHz  PCLK %6.2f MHz  AHB reads %5.2f/us (x%.2f of hwstub's)  [%s]",
            c.name, arm / 1e6, m.pclk / 1e6, m.ahb, m.ahb / ref.ahb, ratios(c.div)))
        io.stdout:flush()
    end
    print(string.format("(core loop: %.2f cycles per pass at hwstub's 200 MHz)", 200e6 / ref.passes))
end

-- PCLK while switching between slow 0x05 (idle: 12/12/12) and PLL 48 0x0c
-- (24/24/12) with one SCU_DIVCON1 write each way; both give PCLK 12 MHz.
-- opts.dwell: core loop passes in each state; opts.control: same writes
-- without switching
function CLK.switch(opts)
    opts = opts or {}
    setup()
    local d = opts.dwell or 20
    local function once(n, idle)
        local ops = append(enter{ pll = pll(6, 48, 4), div = 0x0c },
            { { OP.STAMP, 0 }, { OP.LOOP }, { OP.DIV, idle }, { OP.SPIN, d }, { OP.DIV, 0x0c },
              { OP.SPIN, d }, { OP.ENDLOOP, n }, { OP.STAMP, 1 } })
        local h, r = run(ops)
        return h, ticks(r[1], r[2])
    end
    for _, v in ipairs{ { "steady PLL 48 (control)", 0x0c }, { "switching slow <-> PLL 48", 0x05 } } do
        local h = once(5000, v[2])
        local n = math.max(5000, math.floor(5000 * 0.3 / math.max(h - 0.004, 0.002)))
        local h1, t1 = once(n, v[2])
        local h2, t2 = once(2 * n, v[2])
        print(string.format("%-28s %7d switch pairs per call: PCLK %.3f MHz",
            v[1], n, (t2 - t1) / (h2 - h1) / 1e6))
        io.stdout:flush()
    end
end

-- PLL 48 <-> 192 MHz (same VCO, output divider 4 <-> 1), each change made
-- in slow mode: does the lock bit drop, and how long until it is back?
function CLK.od(opts)
    opts = opts or {}
    setup()
    local ops = enter{ pll = pll(6, 48, 4), div = 0x0c }
    local slot = 0
    local steps = { { pll(6, 48, 1), 0x10, "-> 192 (OD 4->1)" }, { pll(6, 48, 4), 0x0c, "-> 48  (OD 1->4)" } }
    local seq = {}
    for i = 1, opts.n or 3 do append(seq, steps) end
    -- control: the VCO itself changes (192 -> 200 MHz, NF 48 -> 50)
    append(seq, { { pll(6, 50, 4), 0x0c, "-> 50  (VCO 192->200)" }, { pll(6, 48, 4), 0x0c, "-> 48  (VCO 200->192)" } })
    do
        for _, c in ipairs(seq) do
            append(ops, { { OP.DIV, 0x05 }, { OP.PLL, c[1] }, { OP.STATUS, slot },
                          { OP.LOCK, slot + 1 }, { OP.DIV, c[2] }, { OP.SPIN, 10000 } })
            slot = slot + 2
        end
    end
    local _, r = run(ops, slot)
    for i = 0, slot - 1, 2 do
        local w = r[i + 2]
        print(string.format("%-24s lock bit right after the write %d, lock after %s", seq[i / 2 + 1][3],
            bit32.band(r[i + 1], 1), w == 0xffffffff and "TIMEOUT" or string.format("%d PCLK ticks (%.1f us at 12 MHz)", w, w / 12)))
    end
end

-- SDRAM timing for an HCLK in MHz, as set_sdram_timing() in
-- firmware/target/arm/rk27xx/system-rk27xx.c (refresh only; the mode
-- register stays as the boot set it)
local MCSDR_T_REF = 0x180b0110
local MCSDR_T_RFC = 0x180b0114
local STATUS      = 0x1801c02c
local function sdram_timing(hclk)
    wr(MCSDR_T_REF, math.floor(125 * hclk / 8))
    wr(MCSDR_T_RFC, math.floor(64 * hclk / 1000))
end

-- clock settings to stay at (CLK.set): Rockbox's own, from
-- set_cpu_frequency() in system-rk27xx.c (boosted is hwstub's)
CLK.SETTINGS = {
    hwstub = { name = "hwstub / Rockbox boosted: ARM 200, HCLK 100, PCLK 50",
               pll = pll(6, 50, 1), div = 0x08, hclk = 100 },
    normal = { name = "Rockbox normal: ARM 50, HCLK 50, PCLK 50",
               pll = pll(6, 50, 2), div = 0x04, hclk = 50 },
}

-- Switches the ARM clocks to a setting of CLK.SETTINGS (name, or a table
-- with pll, div and hclk) and leaves them there, with the register writes
-- of set_cpu_frequency(). Each write is a USB request to hwstub running
-- on the ARM whose clock changes; while in slow mode that is at ARM 24,
-- HCLK 12 MHz. If the device stops answering, reset it.
function CLK.set(which)
    local cfg = type(which) == "table" and which or CLK.SETTINGS[which]
    if not cfg then error("unknown setting " .. tostring(which)) end
    -- refresh often enough for any HCLK while switching
    sdram_timing(12)
    -- the new dividers and slow mode in one write: the ARM runs from the
    -- 24 MHz oscillator. DIVCON1 bits 4:2 and 0 are the ARM's; bit 1 (the
    -- DSP's slow mode) and the others are kept.
    local keep = bit32.band(rd(DIVCON1), bit32.bnot(0x1d))
    wr(DIVCON1, bit32.bor(keep, cfg.div, 1))
    -- the PLL, and its lock (~0.3 ms)
    wr(PLLCON1, cfg.pll)
    local locked = false
    for _ = 1, 100 do
        if bit32.band(rd(STATUS), 1) ~= 0 then locked = true break end
        hwstub.mdelay(1)
    end
    if not locked then
        error("ARM PLL did not lock; left in slow mode (ARM 24, HCLK 12 MHz)")
    end
    -- out of slow mode, then the refresh for the new HCLK
    wr(DIVCON1, bit32.band(rd(DIVCON1), bit32.bnot(1)))
    sdram_timing(cfg.hclk)
    print(string.format("%s: PLLCON1 %08x DIVCON1 %08x MCSDR_T_REF %d MCSDR_T_RFC %d",
        cfg.name or "set", rd(PLLCON1), rd(DIVCON1), rd(MCSDR_T_REF), rd(MCSDR_T_RFC)))
end
