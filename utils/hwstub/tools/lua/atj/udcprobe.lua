---
--- Which UDC endpoints beyond EP1 OUT / EP2 IN exist? (atj2137_usb.md)
---
--- Register layout from the Actions OWL aotg driver, the same core:
---   OUTnBCL/H 0x08+8(n-1), OUTnCON +2, OUTnCS +3, INn… +4
---   OUTnSTADDR 0x300+4n, INnSTADDR 0x340+4n
---   HCINnMAXPCK 0x1e0+2n (device OUT max packet), HCOUTnMAXPCK 0x3e0+2n (device IN)
---   CON: bit 7 VAL, bit 6 STALL, bits 3:2 TYPE (1 iso 2 bulk 3 int), bits 1:0 BUF
---
--- hwstub talks over EP0 only, so the probe leaves EP0, USBCS, USBIRQ/USBIEN and the
--- EP0 interrupt bits alone. It writes CON (VAL kept clear), STADDR, MAXPCK and the
--- IN/OUT interrupt enables, reads them back and restores every value. CS and BC
--- registers are only read: writing CS hands buffers to the USB side.
---
---   ATJ.udcprobe.dump()      read-only view of the endpoint registers
---   ATJ.udcprobe.run()       write/read-back test, prints a table
---   ATJ.udcprobe.bufmem()    endpoint buffer RAM size (loopback through EP3)
---   ATJ.udcprobe.bufdiag()   why the loopback fails (address/stability/IN readback)
---
ATJ.udcprobe = {}

local UDC = 0xb00e0000
local NEP = 5

local function r8(o) return DEV.read8(UDC + o) end
local function w8(o, v) DEV.write8(UDC + o, v) end
local function r16(o) return DEV.read16(UDC + o) end
local function w16(o, v) DEV.write16(UDC + o, v) end
local function r32(o) return DEV.read32(UDC + o) end
local function w32(o, v) DEV.write32(UDC + o, v) end

local function out_base(n) return 0x08 + 8 * (n - 1) end
local function in_base(n) return 0x0c + 8 * (n - 1) end

-- every register the probe may touch, to spot a write landing elsewhere
local function snapshot()
    local s = {}
    for n = 1, NEP do
        s[#s + 1] = { string.format("OUT%dCON", n), r8(out_base(n) + 2) }
        s[#s + 1] = { string.format("IN%dCON", n), r8(in_base(n) + 2) }
        s[#s + 1] = { string.format("OUT%dSTADDR", n), r32(0x300 + 4 * n) }
        s[#s + 1] = { string.format("IN%dSTADDR", n), r32(0x340 + 4 * n) }
        s[#s + 1] = { string.format("HCIN%dMAXPCK", n), r16(0x1e0 + 2 * n) }
        s[#s + 1] = { string.format("HCOUT%dMAXPCK", n), r16(0x3e0 + 2 * n) }
    end
    s[#s + 1] = { "IN04IEN", r8(0x194) }
    s[#s + 1] = { "OUT04IEN", r8(0x196) }
    return s
end

local function changed(before, after, except)
    local list = {}
    for i, e in ipairs(before) do
        if e[1] ~= except and after[i][2] ~= e[2] then
            list[#list + 1] = string.format("%s %x->%x", e[1], e[2], after[i][2])
        end
    end
    return table.concat(list, ", ")
end

function ATJ.udcprobe.dump()
    for n = 1, NEP do
        local o, i = out_base(n), in_base(n)
        print(string.format("EP%d OUT bc %02x%02x con %02x cs %02x  st %08x mpk %04x" ..
            " | IN bc %02x%02x con %02x cs %02x  st %08x mpk %04x", n,
            r8(o + 1), r8(o), r8(o + 2), r8(o + 3), r32(0x300 + 4 * n), r16(0x1e0 + 2 * n),
            r8(i + 1), r8(i), r8(i + 2), r8(i + 3), r32(0x340 + 4 * n), r16(0x3e0 + 2 * n)))
    end
    print(string.format("IN04IEN %02x OUT04IEN %02x IN04IRQ %02x OUT04IRQ %02x IVECT %02x",
        r8(0x194), r8(0x196), r8(0x188), r8(0x18a), r8(0x1a0)))
    print(string.format("FIFOCTRL %02x DMAEPSEL %02x", r8(0x1a8), r8(0x40c)))
end

-- write each pattern, OR together what reads back; restore. Returns the bits that
-- can be set, the bits that can be cleared and what else changed.
local function probe(name, rd, wr, patterns)
    local before = snapshot()
    local orig = rd()
    local set, clr = 0, 0
    for _, p in ipairs(patterns) do
        wr(p)
        local v = rd()
        set = bit32.bor(set, bit32.band(v, p))
        clr = bit32.bor(clr, bit32.band(bit32.bnot(v), bit32.bnot(p)))
    end
    wr(orig)
    local after_restore = rd()
    local side = changed(before, snapshot(), name)
    return orig, set, clr, side, after_restore == orig
end

local function report(name, orig, set, clr, side, restored, width)
    if width < 8 then
        clr = bit32.band(clr, bit32.lshift(1, width * 4) - 1)
    end
    local fmt = "%-14s orig %0" .. width .. "x  settable %0" .. width ..
        "x  clearable %0" .. width .. "x%s%s"
    print(string.format(fmt, name, orig, set, clr,
        restored and "" or "  NOT RESTORED",
        side ~= "" and ("  side effects: " .. side) or ""))
end

local function test(name, rd, wr, patterns, width)
    local orig, set, clr, side, restored = probe(name, rd, wr, patterns)
    report(name, orig, set, clr, side, restored, width)
end

function ATJ.udcprobe.run()
    -- CON without VAL (bit 7): int/iso types, all buffer counts, stall
    local con_pat = { 0x0d, 0x06, 0x4f, 0x00 }
    local st_pat = { 0x00000a5a, 0x000005a5, 0x0000ffff, 0x00000000 }
    local mpk_pat = { 0x05a5, 0x0a5a, 0x0000 }

    print("-- endpoint CON (8 bit)")
    for n = 1, NEP do
        for _, d in ipairs({ { "OUT", out_base(n) }, { "IN", in_base(n) } }) do
            local name = string.format("%s%dCON", d[1], n)
            local o = d[2] + 2
            test(name, function() return r8(o) end,
                function(v) w8(o, v) end, con_pat, 2)
        end
    end

    print("-- buffer start addresses (32 bit access)")
    for n = 1, NEP do
        for _, d in ipairs({ { "OUT", 0x300 }, { "IN", 0x340 } }) do
            local name = string.format("%s%dSTADDR", d[1], n)
            local o = d[2] + 4 * n
            test(name, function() return r32(o) end,
                function(v) w32(o, v) end, st_pat, 8)
        end
    end

    print("-- max packet (16 bit)")
    for n = 1, NEP do
        for _, d in ipairs({ { "HCIN", 0x1e0 }, { "HCOUT", 0x3e0 } }) do
            local name = string.format("%s%dMAXPCK", d[1], n)
            local o = d[2] + 2 * n
            test(name, function() return r16(o) end,
                function(v) w16(o, v) end, mpk_pat, 4)
        end
    end

    -- EP0's bit (bit 0) stays as it is, hwstub needs it
    print("-- interrupt enables, EP1-7 bits")
    for _, d in ipairs({ { "IN04IEN", 0x194 }, { "OUT04IEN", 0x196 } }) do
        local o = d[2]
        local orig = r8(o)
        local keep = bit32.band(orig, 1)
        test(d[1], function() return r8(o) end,
            function(v) w8(o, bit32.bor(bit32.band(v, 0xfe), keep)) end,
            { 0xfe, 0x00 }, 2)
    end
end

--- How much endpoint buffer RAM is there? The buffers are not mapped for the
--- CPU, only reached through the FIFO registers: EP3 IN and EP3 OUT get the same
--- start address, a pattern is written through the IN side and read back through
--- the OUT side. EP3 is not used by hwstub (EP0 only) and the host does not know
--- it, CS and BC are never written. FIFOCTRL (0x1a8: bit 7 CPU access, bit 4 IN,
--- bits 2:0 ep, set by the ADFU server) is tried left alone and set.
---
---   ATJ.udcprobe.bufmem()

local FIFO3, FIFOCTRL, ENDPRST = 0x8c, 0x1a8, 0x1a2
local IN3STADDR, OUT3STADDR = 0x340 + 12, 0x300 + 12
local IN3CON, OUT3CON = in_base(3) + 2, out_base(3) + 2
local NWORDS = 8

local function fifo_reset(sel)
    w8(ENDPRST, sel)
    w8(ENDPRST, bit32.bor(sel, 0x60))
end

local function pattern(addr, i)
    return bit32.bxor(0x5aa50000 + i * 0x1111, addr)
end

local function put(addr, ctrl)
    w32(IN3STADDR, addr)
    fifo_reset(0x13)
    if ctrl then w8(FIFOCTRL, bit32.bor(ctrl, 0x13)) end
    for i = 0, NWORDS - 1 do w32(FIFO3, pattern(addr, i)) end
end

local function get(addr, ctrl)
    w32(OUT3STADDR, addr)
    fifo_reset(0x03)
    if ctrl then w8(FIFOCTRL, bit32.bor(ctrl, 0x03)) end
    local t = {}
    for i = 0, NWORDS - 1 do t[i] = r32(FIFO3) end
    return t
end

-- which address the words read at addr were written for (nil: none of them)
local function owner(t, addrs)
    for _, a in ipairs(addrs) do
        local ok = true
        for i = 0, NWORDS - 1 do
            if t[i] ~= pattern(a, i) then ok = false break end
        end
        if ok then return a end
    end
    return nil
end

function ATJ.udcprobe.bufmem()
    local save = { r8(IN3CON), r8(OUT3CON), r32(IN3STADDR), r32(OUT3STADDR),
                   r8(FIFOCTRL) }
    -- enabled, bulk, single buffer
    w8(IN3CON, 0x88)
    w8(OUT3CON, 0x88)

    local modes = { { "FIFOCTRL untouched", nil }, { "FIFOCTRL ep/dir", 0x00 },
                    { "FIFOCTRL ep/dir + bit 7", 0x80 } }
    local mode
    for _, m in ipairs(modes) do
        w8(FIFOCTRL, save[5])
        put(0x1000, m[2])
        local t = get(0x1000, m[2])
        local ok = owner(t, { 0x1000 }) ~= nil
        print(string.format("loopback at 0x1000, %-24s %s  (read %08x %08x ..)",
            m[1], ok and "OK" or "no", t[0], t[1]))
        if ok and not mode then mode = m end
    end

    if mode then
        print("size test with " .. mode[1] .. ": marker at every 512 bytes, then read back")
        local addrs = {}
        for a = 0, 0x1e00, 0x200 do addrs[#addrs + 1] = a end
        for _, a in ipairs(addrs) do put(a, mode[2]) end
        for _, a in ipairs(addrs) do
            local t = get(a, mode[2])
            local o = owner(t, addrs)
            local what
            if o == a then what = "own marker"
            elseif o then what = string.format("ALIAS of 0x%04x", o)
            else what = string.format("no marker (%08x %08x ..)", t[0], t[1]) end
            print(string.format("  0x%04x: %s", a, what))
        end
    else
        print("no loopback mode works: the size cannot be measured this way")
    end

    -- restore
    w8(IN3CON, save[1])
    w8(OUT3CON, save[2])
    w32(IN3STADDR, save[3])
    w32(OUT3STADDR, save[4])
    fifo_reset(0x13)
    fifo_reset(0x03)
    w8(FIFOCTRL, save[5])
end

--- Why does the loopback fail? Reads only (plus the IN side writes of bufmem):
--- does the OUT side read depend on the start address, is it stable, does an IN
--- write change it, can the IN side read back what it wrote. CS is only read.
---
---   ATJ.udcprobe.bufdiag()

local function words(t, n)
    local s = {}
    for i = 0, (n or 4) - 1 do s[#s + 1] = string.format("%08x", t[i]) end
    return table.concat(s, " ")
end

function ATJ.udcprobe.bufdiag()
    local save = { r8(IN3CON), r8(OUT3CON), r32(IN3STADDR), r32(OUT3STADDR),
                   r8(FIFOCTRL) }
    w8(IN3CON, 0x88)
    w8(OUT3CON, 0x88)
    print(string.format("EP3 OUT cs %02x  IN cs %02x  FIFOCTRL %02x",
        r8(out_base(3) + 3), r8(in_base(3) + 3), r8(FIFOCTRL)))

    print("-- OUT side reads, no writes")
    for _, a in ipairs({ 0x0000, 0x0200, 0x0800, 0x1000, 0x1800, 0x1000 }) do
        print(string.format("  0x%04x: %s", a, words(get(a, nil))))
    end

    print("-- IN write at 0x1000, then reads")
    put(0x1000, nil)
    print(string.format("  IN cs after write %02x", r8(in_base(3) + 3)))
    print(string.format("  OUT side 0x1000: %s", words(get(0x1000, nil))))
    -- read the IN side's own FIFO, from its start
    fifo_reset(0x13)
    local t = {}
    for i = 0, 3 do t[i] = r32(FIFO3) end
    print(string.format("  IN side  0x1000: %s", words(t)))
    w8(FIFOCTRL, 0x13)
    fifo_reset(0x13)
    for i = 0, 3 do t[i] = r32(FIFO3) end
    print(string.format("  IN side  0x1000, FIFOCTRL 13: %s", words(t)))
    print(string.format("  expected:        %08x %08x %08x %08x", pattern(0x1000, 0),
        pattern(0x1000, 1), pattern(0x1000, 2), pattern(0x1000, 3)))

    w8(IN3CON, save[1])
    w8(OUT3CON, save[2])
    w32(IN3STADDR, save[3])
    w32(OUT3STADDR, save[4])
    fifo_reset(0x13)
    fifo_reset(0x03)
    w8(FIFOCTRL, save[5])
end
