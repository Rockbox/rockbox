---
--- ATJ213x NAND controller access (read only)
---
--- The ATJ2137 NAND controller differs from the ATJ2135 programming guide.
--- It is driven by command words written to +0x24 (NAND_BUF0 in rockbox
--- naming), reverse engineered from BROM and BREC96R1:
---   [7:0]   NAND command byte
---   [19:16] phase: 1 = command, 5 = command + wait R/B, 9 = command + data in
---   [23:20] address cycles: 8 = column (2 cycles), 3 = row (3 cycles),
---           b = column + row, 1 = single byte (READ ID)
---   [24]    use spare column (+0x0c) instead of main column (+0x10)
---   [31]    more command words follow
--- Row address goes to +0x14 (+0x18 high), data phase length to +0x1c,
--- small results are read from +0x28 which is also the DMA FIFO.
---
ATJ.nand = {}

local NAND_BASE = 0xb00a0000
local R = {
    CTL      = NAND_BASE + 0x00,
    STATUS   = NAND_BASE + 0x04,
    FIFOTIM  = NAND_BASE + 0x08,
    COLSPARE = NAND_BASE + 0x0c, -- NAND_CLKCTL in rockbox headers
    COLMAIN  = NAND_BASE + 0x10, -- NAND_BYTECNT
    ROW      = NAND_BASE + 0x14, -- NAND_ADDR01
    ROWHI    = NAND_BASE + 0x18, -- NAND_ADDR23
    DATALEN  = NAND_BASE + 0x1c, -- NAND_ADDR45
    USERDATA = NAND_BASE + 0x20, -- NAND_ADDR67
    FSM      = NAND_BASE + 0x24, -- NAND_BUF0
    FIFO     = NAND_BASE + 0x28, -- NAND_BUF1
    ECCMODE  = NAND_BASE + 0x2c, -- NAND_CMD
}
ATJ.nand.R = R

local CMU_NANDCLK = 0xb0010018
local CMU_DEVCLKEN = 0xb0010080
local CMU_DEVRST = 0xb0010084
local GPIO_MFCTL0 = 0xb01c0018
local GPIO_MFCTL1 = 0xb01c001c

local DMAC_CTL = 0xb0060000
local DMAC_IRQPD = 0xb0060008
local DMA_CH = 1
local DMA_BASE = 0xb0060100 + 0x20 * DMA_CH
local DMA_MODE, DMA_SRC, DMA_DST, DMA_CNT, DMA_CMD =
    DMA_BASE, DMA_BASE + 4, DMA_BASE + 8, DMA_BASE + 0xc, DMA_BASE + 0x14

-- DMA target buffer in SDRAM (uncached view)
ATJ.nand.buf = 0xa0100000

ATJ.nand.ce = 0
ATJ.nand.verbose = false

local function rd(a) return DEV.read32(a) end
local function wr(a, v) DEV.write32(a, v) end
local function bor(a, v) wr(a, bit32.bor(rd(a), v)) end

local function wait(cond, what)
    for i = 1, 10000 do
        if cond() then return true end
    end
    error("timeout waiting for " .. what .. string.format(" (STATUS=%#x)", rd(R.STATUS)))
end

-- FSM busy is STATUS[31], data transfer busy STATUS[7]
local function wait_fsm()
    wait(function() return bit32.band(rd(R.STATUS), 0x80000000) == 0 end, "FSM idle")
end

local function wait_xfer()
    wait(function() return bit32.band(rd(R.STATUS), 0x80) == 0 end, "transfer idle")
end

-- write command word; OF reads FSM register back before polling
function ATJ.nand.fsm(word)
    if ATJ.nand.verbose then print(string.format("FSM <- %#010x", word)) end
    wr(R.FSM, word)
end

-- NAND timing, entry from BREC96R1 table @0xb4056064 (NANDCLK divider,
-- FIFOTIM). Default is the slowest entry, safe for any core clock.
ATJ.nand.timing = { nandclk = 8, fifotim = 4 }

function ATJ.nand.init()
    -- clocks: NAND (bit 9), DMA (same bits BROM sets)
    bor(CMU_DEVCLKEN, 0x4000314)
    -- pulse NAND block reset (CMU_DEVRST bit 5, active low) as BREC does
    wr(CMU_DEVRST, bit32.band(rd(CMU_DEVRST), bit32.bnot(0x20)))
    hwstub.udelay(10)
    wr(CMU_DEVRST, bit32.bor(rd(CMU_DEVRST), 0x20))
    hwstub.udelay(10)
    wr(CMU_NANDCLK, ATJ.nand.timing.nandclk)
    hwstub.udelay(10)
    -- pin mux: NAND_CLE/ALE/RB, CEB1, NAND_D[15:0]
    local m = rd(GPIO_MFCTL0)
    wr(GPIO_MFCTL0, bit32.bor(bit32.band(m, 0xfe3ff300), 0x400449))
    bor(GPIO_MFCTL1, 0x80000000)
    -- controller setup as done by BREC (EDO mode, 8bit bus)
    wr(R.FIFOTIM, ATJ.nand.timing.fifotim)
    hwstub.udelay(10)
    wr(R.CTL, 0x40903)
    wr(R.ECCMODE, 1)
    wr(R.COLSPARE, 0)
    wr(R.COLMAIN, 0)
    wr(R.ROW, 0)
    wr(R.ROWHI, 0)
    wr(R.USERDATA, 0)
    wr(R.DATALEN, 0)
    print(string.format("NAND: CTL=%#x STATUS=%#x", rd(R.CTL), rd(R.STATUS)))
end

function ATJ.nand.select(ce)
    ce = ce or ATJ.nand.ce
    local c = bit32.band(rd(R.CTL), 0xffffff86)
    wr(R.CTL, bit32.bor(c, bit32.lshift(1, ce + 4), 1))
end

function ATJ.nand.deselect()
    wr(R.CTL, bit32.band(rd(R.CTL), 0xffffff86))
end

function ATJ.nand.reset(ce)
    ATJ.nand.select(ce)
    wait_fsm()
    ATJ.nand.fsm(0x100ff)
    rd(R.FSM)
    wait_fsm()
    hwstub.mdelay(5)
    ATJ.nand.deselect()
end

-- read n (<= 4) bytes from data phase of a register command
local function reg_read(word, n)
    wr(R.DATALEN, n)
    ATJ.nand.fsm(word)
    rd(R.FSM)
    wait_xfer()
    local v = rd(R.FIFO)
    rd(R.FSM)
    wait_fsm()
    return v
end

-- READ ID, returns table of bytes; reads 8 bytes as two 4 byte phases
-- at address 0x00 and with offset (some chips need full 5-6 bytes)
function ATJ.nand.id(ce)
    ATJ.nand.select(ce)
    wait_fsm()
    wr(R.ROW, 0)
    local v = reg_read(0x190090, 4)
    ATJ.nand.deselect()
    local id = {}
    for i = 0, 3 do id[#id + 1] = bit32.band(bit32.rshift(v, 8 * i), 0xff) end
    print(string.format("CE%d ID: %02x %02x %02x %02x", ce or ATJ.nand.ce,
        id[1], id[2], id[3], id[4]))
    return id
end

function ATJ.nand.status(ce)
    ATJ.nand.select(ce)
    wait_fsm()
    local v = bit32.band(reg_read(0x90070, 1), 0xff)
    ATJ.nand.deselect()
    print(string.format("status: %#04x", v))
    return v
end

local function dma_setup(len)
    wr(DMAC_CTL, bit32.lshift(0x10000, DMA_CH)) -- reset channel
    wr(DMAC_CTL, bit32.lshift(1, DMA_CH))       -- enable channel
    wr(DMAC_IRQPD, 0xffffffff)
    -- mode word taken from BROM: FIFO -> SDRAM 0x8401a4, -> IRAM 0x8c01a4
    local dst = ATJ.nand.buf
    if bit32.band(dst, 0x10000000) ~= 0 then
        wr(DMA_MODE, 0x8c01a4)
    else
        wr(DMA_MODE, 0x8401a4)
    end
    wr(DMA_SRC, bit32.band(R.FIFO, 0x1fffffff))
    wr(DMA_DST, bit32.band(dst, 0x1fffffff))
    wr(DMA_CNT, len)
    -- channel is started after FSM command words, as BROM does
end

local function dma_wait()
    for i = 1, 10000 do
        if bit32.band(rd(DMA_CMD), 1) == 0 then return end
    end
    print(string.format("warning: DMA still busy, CMD=%#x CNT=%#x", rd(DMA_CMD), rd(DMA_CNT)))
end

-- Raw read of len bytes (multiple of 4, <= 0x200 per chunk is what OF uses)
-- from page row at column col into ATJ.nand.buf. No ECC processing.
-- The page is loaded to chip cache with 00h/30h and then streamed out with
-- 05h/E0h random data output, chunk by chunk.
function ATJ.nand.read_raw(row, col, len, ce, chunk)
    chunk = chunk or 0x200
    ATJ.nand.select(ce)
    wait_fsm()
    -- page to cache: 00h col row 30h, wait busy
    wr(R.COLMAIN, col)
    wr(R.ROW, row)
    wr(R.ROWHI, 0)
    ATJ.nand.fsm(0x80b10000)
    ATJ.nand.fsm(0x50030)
    rd(R.FSM)
    wait_fsm()
    local buf = ATJ.nand.buf
    local done = 0
    while done < len do
        local n = math.min(chunk, len - done)
        ATJ.nand.buf = buf + done
        dma_setup(n)
        wr(R.DATALEN, n)
        wr(R.COLMAIN, col + done)
        ATJ.nand.fsm(0x80810005)
        ATJ.nand.fsm(0x900e0)
        wr(DMA_CMD, 1)
        dma_wait()
        rd(R.FSM)
        wait_fsm()
        done = done + n
    end
    ATJ.nand.buf = buf
    ATJ.nand.deselect()
end

-- copy n bytes of the DMA buffer to a file (slow, word by word)
function ATJ.nand.save(path, n, offset)
    offset = offset or 0
    local f = io.open(path, "wb")
    for i = 0, n - 1, 4 do
        local v = rd(ATJ.nand.buf + offset + i)
        f:write(string.char(bit32.band(v, 0xff), bit32.band(bit32.rshift(v, 8), 0xff),
            bit32.band(bit32.rshift(v, 16), 0xff), bit32.band(bit32.rshift(v, 24), 0xff)))
    end
    f:close()
end

-- print n bytes of the DMA buffer
function ATJ.nand.hexdump(n, offset)
    offset = offset or 0
    for i = 0, n - 1, 16 do
        local s = string.format("%04x:", offset + i)
        for j = 0, 15, 4 do
            s = s .. string.format(" %08x", rd(ATJ.nand.buf + offset + i + j))
        end
        print(s)
    end
end

-- fill buffer with pattern so stale data is not mistaken for a read
function ATJ.nand.clear(n, pattern)
    pattern = pattern or 0xdeadbeef
    for i = 0, n - 1, 4 do wr(ATJ.nand.buf + i, pattern) end
end

-- Quick sanity test: page 0 must contain MBRC ("ActBrm" at 0x1f4)
function ATJ.nand.test(ce)
    ATJ.nand.init()
    ATJ.nand.reset(ce)
    ATJ.nand.id(ce)
    ATJ.nand.status(ce)
    ATJ.nand.clear(0x200)
    ATJ.nand.read_raw(0, 0, 0x200, ce)
    ATJ.nand.hexdump(0x20)
    ATJ.nand.hexdump(0x10, 0x1f0)
    local sig = rd(ATJ.nand.buf + 0x1f4)
    if sig == 0x42746341 then
        print("MBRC signature found at page 0 - raw read works")
    else
        print(string.format("no MBRC signature (got %#x)", sig))
    end
end

-- Print first 16 bytes at column 0 and 2048 of pages first..first+n-1
-- (slow path, used to cross-check the bulk dumper)
function ATJ.nand.peek(first, n, ce)
    first = first or 0
    n = n or 8
    for p = first, first + n - 1 do
        local s = string.format("%d", p)
        for _, col in ipairs({0, 2048}) do
            ATJ.nand.clear(16)
            ATJ.nand.read_raw(p, col, 16, ce)
            s = s .. " |"
            for i = 0, 12, 4 do s = s .. string.format(" %08x", rd(ATJ.nand.buf + i)) end
        end
        print(s)
    end
end
