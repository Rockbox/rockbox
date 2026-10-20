---
--- X1000 memory throughput
---
JZ.x1000mem = {}

local h = HELP:get_topic("JZ"):create_topic("x1000mem")
h:add("Memory throughput on the X1000: read, write and copy, by the CPU, from")
h:add("the L1 and L2 caches, cached and uncached DRAM, and TCSM.")
h:add("JZ.x1000mem.run() measures each region at the current clocks.")
h:add("JZ.x1000mem.table() measures them at several CPU, L2 and AHB settings, and")
h:add("goes back to hwstub's clocks (CPU 1008, L2 504, AHB 200 MHz) at the end.")
h:add("JZ.x1000mem.measure(addr, size, op) measures one buffer: op is \"read\",")
h:add("\"write\" or \"copy\" (from the first half of the buffer to the second).")

--
-- jz/x1000_mem.S, embedded below, runs passes of 32-byte reads, writes or
-- copies over a buffer and records the OST2 counter around them.
--
local KERNEL =
    "0000193c000039370000288f0400248f0800258f0c00268f1000278f00b2183c" ..
    "2000028f254880002550a000215886001b000011ffff01252900201000000000" ..
    "00002c8d04002d8d08002e8d0c002f8d00004cad04004dad08004ead0c004fad" ..
    "10002c8d14002d8d18002e8d1c002f8d10004cad14004dad18004ead1c004fad" ..
    "20002925eeff2b1520004a251f00001000000000000000000000000000000000" ..
    "00002c8d04002d8d08002e8d0c002f8d10002c8d14002d8d18002e8d1c002f8d" ..
    "20002925f6ff2b15000000000f00001000000000000000000000000000000000" ..
    "000029ad040029ad080029ad0c0029ad100029ad140029ad180029ad1c0029ad" ..
    "20002925f6ff2b1500000000ffffe724c4ffe014000000002000038f140022af" ..
    "180023af0800e00300000000"

local DEV = DEV or hwstub.dev
local OST_HZ = 24000000 / 4
local OPS = { read = 0, write = 1, copy = 2 }

local params, code

local function load()
    if code then return end
    -- near the end of hwstub's buffer, below the timing loop of x1000clk.lua
    local buf = hwstub.dev.layout.buffer
    params = bit32.band(buf.start + buf.size - 0x400, bit32.bnot(0x3f))
    code = params + 0x40
    for i = 0, #KERNEL / 8 - 1 do
        local hex = KERNEL:sub(i * 8 + 1, i * 8 + 8)
        local w = tonumber(hex:sub(7, 8) .. hex:sub(5, 6) .. hex:sub(3, 4) .. hex:sub(1, 2), 16)
        if i == 0 then w = bit32.bor(w, bit32.rshift(params, 16)) end
        if i == 1 then w = bit32.bor(w, bit32.band(params, 0xffff)) end
        DEV.write32(code + i * 4, w)
        if DEV.read32(code + i * 4) ~= w then
            code = nil
            error("memory kernel verify failed")
        end
    end
end

-- OST ticks for a number of passes
local function ticks(op, addr, dst, size, passes)
    DEV.write32(params, op)
    DEV.write32(params + 4, addr)
    DEV.write32(params + 8, dst)
    DEV.write32(params + 12, size)
    DEV.write32(params + 16, passes)
    DEV.call(code)
    return (DEV.read32(params + 24) - DEV.read32(params + 20)) % 0x100000000
end

-- Bytes per second read, written or copied. The caches are flushed before
-- each call: timing n and 2n passes and taking the difference removes the
-- cold start and the call itself. A call is kept well under hwstub's 1 s
-- USB timeout.
function JZ.x1000mem.measure(addr, size, op)
    load()
    local o = OPS[op or "read"]
    if not o then error("op must be read, write or copy") end
    if size % 64 ~= 0 then error("size must be a multiple of 64") end
    local dst = addr
    if o == 2 then
        size = size / 2
        dst = addr + size
    end
    local t1 = ticks(o, addr, dst, size, 1)
    local n = math.max(1, math.floor(0.1 * OST_HZ / math.max(t1, 1)))
    local tn = ticks(o, addr, dst, size, n)
    local t2n = ticks(o, addr, dst, size, 2 * n)
    local dt = t2n - tn
    if dt <= 0 then dt = tn end
    return n * size * OST_HZ / dt
end

-- DRAM test buffer: hwstub's buffer, past what a USB transfer uses at its
-- start, aligned to a page
local function dram()
    local buf = hwstub.dev.layout.buffer
    return bit32.band(buf.start + 0x1fff, bit32.bnot(0xfff))
end

local function regions()
    local d = dram()
    return {
        { "L1, 8 KB (cached DRAM)", d, 8 * 1024 },
        { "L2, 64 KB (cached DRAM)", d, 64 * 1024 },
        { "DRAM, 512 KB, cached", d, 512 * 1024 },
        { "DRAM, 64 KB, uncached", bit32.bor(d, 0x20000000), 64 * 1024 },
        { "TCSM, 8 KB", 0xf4002000, 8 * 1024 },
    }
end

local function mbs(v) return string.format("%8.1f", v / 1e6) end

function JZ.x1000mem.run()
    local f = JZ.x1000clk.freqs()
    print(string.format("CPU %.0f, L2 %.0f, AHB0 %.0f, AHB2 %.0f MHz (DDR not changed)",
                        f.cpu / 1e6, f.l2 / 1e6, f.ahb0 / 1e6, f.ahb2 / 1e6))
    print(string.format("  %-26s %8s %8s %8s  (MB/s)", "", "read", "write", "copy"))
    for _, r in ipairs(regions()) do
        print(string.format("  %-26s %s %s %s", r[1],
                            mbs(JZ.x1000mem.measure(r[2], r[3], "read")),
                            mbs(JZ.x1000mem.measure(r[2], r[3], "write")),
                            mbs(JZ.x1000mem.measure(r[2], r[3], "copy"))))
    end
end

function JZ.x1000mem.table()
    local clk = JZ.x1000clk
    local settings = {
        function() clk.bus(3) clk.cpu_apll(1) end,      -- 1008 / 504, AHB 200
        function() clk.bus(3) clk.cpu_mpll(1) end,      -- 600 / 300, AHB 200
        function() clk.bus(3) clk.cpu_mpll(3, 3) end,   -- 200 / 200, AHB 200
        function() clk.bus(3) clk.cpu_mpll(3) end,      -- 200 / 100, AHB 200
        function() clk.bus(6) clk.cpu_apll(1) end,      -- 1008 / 504, AHB 100
        function() clk.bus(6) clk.cpu_mpll(3, 3) end,   -- 200 / 200, AHB 100
    }
    for _, set in ipairs(settings) do
        set()
        JZ.x1000mem.run()
    end
    clk.bus(3)
    clk.cpu_apll(1)
end
