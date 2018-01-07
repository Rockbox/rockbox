MIPS = {}

function MIPS.read_cp0(reg, sel)
    return DEV.read32_cop({0, reg, sel})
end

function MIPS.write_cp0(reg, sel, val)
    DEV.write32_cop({0, reg, sel}, val)
end

MIPS.prid_table = {
    [0x0ad0024f] = "JZ4740",
    [0x1ed0024f] = "JZ4755",
    [0x2ed0024f] = "JZ4760(B)",
    [0x3ee1024f] = "JZ4780"
}

MIPS.at_table = {
    [0] = "MIPS32",
    [1] = "MIPS64 with 32-bit segments",
    [2] = "MIPS64"
}

MIPS.ar_table = {
    [0] = "Release 1",
    [1] = "Release 2 (or more)"
}

MIPS.mt_table = {
    [0] = "None",
    [1] = "Standard TLB",
    [2] = "BAT",
    [3] = "Fixed Mapping",
    [4] = "Dual VTLB and FTLB"
}

MIPS.is_table = {
    [0] = 64,
    [1] = 128,
    [2] = 256,
    [3] = 512,
    [4] = 1024,
    [5] = 2048,
    [6] = 4096,
    [7] = 32
}

MIPS.il_table = {
    [0] = 0,
    [1] = 4,
    [2] = 8,
    [3] = 16,
    [4] = 32,
    [5] = 64,
    [6] = 128
}

MIPS.cache_table = {
    [2] = "Uncached",
    [3] = "Cacheable, non-coherent",
}

function MIPS.get_table_or(tbl, index, dflt)
    if tbl[index] ~= nil then
        return tbl[index]
    else
        return dflt
    end
end

function MIPS.do_ebase_test()
    MIPS.write_cp0(15, 1, 0x80000000)
    print(string.format("    Value after writing 0x80000000: 0x%x", MIPS.read_cp0(15, 1)))
    if MIPS.read_cp0(15, 1) ~= 0x80000000 then
        error("Value 0x8000000 does not stick, EBASE is probably not working")
    end
    MIPS.write_cp0(15, 1, 0x80040000)
    print(string.format("    Value after writing 0x80040000: 0x%x", MIPS.read_cp0(15, 1)))
    if MIPS.read_cp0(15, 1) ~= 0x80040000 then
        error("Value 0x80040000 does not stick, EBASE is probably not working")
    end
    print("EBase seems to work")
end

function MIPS.do_ebase_exc_test(mem_addr)
    if (mem_addr % 0x1000) ~= 0 then
        return "  memory address for exception test must aligned on a 0x1000 boundary";
    end
    print(string.format("Exception test with EBASE at 0x%x...", mem_addr))
    print("  Writing instructions to memory")
    -- create instructions in memory
    exc_addr = mem_addr + 0x180 -- general exception vector
    data_addr = mem_addr + 0x300
    -- lui k0,<low part of data_addr>
    -- ori     k0,k0,<high part>
    DEV.write32(exc_addr + 0, 0x3c1a0000 + bit32.rshift(data_addr, 16))
    DEV.write32(exc_addr + 4, 0x375a0000 + bit32.band(data_addr, 0xffff))
    -- lui     k1,0xdead
    -- ori     k1,k1,0xbeef
    DEV.write32(exc_addr + 8, 0x3c1bdead)
    DEV.write32(exc_addr + 12, 0x377bbeef)
    -- sw      k1,0(k0)
    DEV.write32(exc_addr + 16, 0xaf5b0000)
    -- mfc0    k0,c0_epc
    -- addi    k0,k0,4
    -- mtc0    k0,c0_epc
    DEV.write32(exc_addr + 20, 0x401a7000)
    DEV.write32(exc_addr + 24, 0x235a0004)
    DEV.write32(exc_addr + 28, 0x409a7000)
    -- eret
    -- nop
    DEV.write32(exc_addr + 32, 0x42000018)
    DEV.write32(exc_addr + 36, 0)
    -- fill data with some initial value
    DEV.write32(data_addr, 0xcafebabe)
    -- write instructions to trigger an interrupt
    bug_addr = mem_addr
    -- syscall
    DEV.write32(bug_addr + 0, 0x0000000c)
    -- jr   ra
    -- nop
    DEV.write32(bug_addr + 4, 0x03e00008)
    DEV.write32(bug_addr + 8, 0)

    -- make sure we are the right shape for the test: SR should have BEV cleared,
    -- mask all interrupts, enable interrupts
    ebase_old = MIPS.read_cp0(15, 1)
    old_sr = MIPS.read_cp0(12, 0)
    print(string.format("  Old SR: 0x%x", old_sr))
    MIPS.write_cp0(12, 0, 0xfc00) -- BEV set to 0, all interrupts masked and interrupt disabled
    print(string.format("  New SR: 0x%x", MIPS.read_cp0(12, 0)))
    -- change EBASE
    old_ebase = MIPS.read_cp0(15, 1)
    MIPS.write_cp0(15, 1, mem_addr)
    print(string.format("  EBASE: %x", MIPS.read_cp0(15, 1)))
    -- test
    print(string.format("  Before: %x", DEV.read32(data_addr)))
    DEV.call(bug_addr)
    print(string.format("  After: %x", DEV.read32(data_addr)))
    success = DEV.read32(data_addr) == 0xdeadbeef
    -- restore SR and EBASE
    MIPS.write_cp0(12, 0, old_sr)
    MIPS.write_cp0(15, 1, ebase_old)

    print(success and "Exception and EBASE are working" or "Exception and EBASE are NOT working")
end

function MIPS.test_ebase(mem_addr)
    -- EBase
    ebase_old = MIPS.read_cp0(15, 1)
    sr_old = MIPS.read_cp0(12, 0)
    print("Testing EBASE...")
    print("  Disable BEV")
    MIPS.write_cp0(12, 0, bit32.replace(sr_old, 0, 22)) -- clear BEV
    print(string.format("  SR value: 0x%x", MIPS.read_cp0(12, 0)))
    print(string.format("  EBASE value: 0x%x", ebase_old))
    print("  Test result: " .. MIPS.do_ebase_test())
    MIPS.write_cp0(12, 0, sr_old)
    MIPS.write_cp0(15, 1, ebase_old)
    -- now try with actual exceptions
    if mem_addr == nil then
        print("  Not doing exception test, please specify memory to use: sram, ram")
        return
    end
    print("  Exception result: " .. MIPS.do_ebase_exc_test(mem_addr))
end

function MIPS.test_ext_inst(mem_addr)
    data_addr = mem_addr + 0x80
    -----------
    -- test ext
    -----------
    for pos = 0, 31 do
        for size = 1, 32 - pos do
            -- lui     v0,<low part of data_addr>
            -- addiu   v0,v0,<high part>
            -- lw      v1,0(v0)
            DEV.write32(mem_addr + 0, 0x3c020000 + bit32.rshift(data_addr, 16))
            DEV.write32(mem_addr + 4, 0x8c430000 + bit32.band(data_addr, 0xffff))
            DEV.write32(mem_addr + 8, 0x8c430000)
            -- ext     v1, v1, pos, size
            DEV.write32(mem_addr + 12, 0x7c630000 + bit32.rshift(size - 1, 11) + bit32.rshift(pos, 6))
            -- sw      v1,0(v0)
            DEV.write32(mem_addr + 16, 0xac430000)
            -- jr   ra
            -- nop
            DEV.write32(mem_addr + 20, 0x03e00008)
            DEV.write32(mem_addr + 24, 0)
            -- write some random data
            data = math.random(0xffffffff)
            print(string.format("  data: %x", data))
            DEV.write32(data_addr, data)
            DEV.call(mem_addr)
            ext_data = DEV.read32(data_addr)
            print(string.format("  result: %x vs %x", ext_data, bit32.extract(data, pos, size)))
            break
        end
        break
    end
end

function MIPS.test_ei_di_inst(mem_addr)
    -- save SR and disable interrupts
    old_sr = MIPS.read_cp0(12, 0)
    MIPS.write_cp0(12, 0, bit32.replace(old_sr, 0, 0)) -- clear EI
    print("Testing ei")
    print("  Test SR")
    print("    Enable interrupts with CP0")
    MIPS.write_cp0(12, 0, bit32.replace(old_sr, 1, 0)) -- set EI
    print(string.format("    SR: 0x%x", MIPS.read_cp0(12, 0)))
    print("    Disable interrupts with CP0")
    MIPS.write_cp0(12, 0, bit32.replace(old_sr, 0, 0)) -- clear EI
    print(string.format("    SR: 0x%x", MIPS.read_cp0(12, 0)))

    print("  Test ei/di")
    print("    Enable interrupts with ei")
    -- ei
    -- jr   ra
    -- nop
    DEV.write32(mem_addr + 0, 0x41606020)
    DEV.write32(mem_addr + 4, 0x03e00008)
    DEV.write32(mem_addr + 8, 0)
    DEV.call(mem_addr)
    print(string.format("    SR: 0x%x", MIPS.read_cp0(12, 0)))
    print("    Disable interrupts with di")
    -- di
    DEV.write32(mem_addr + 0, 0x41606000)
    DEV.call(mem_addr)
    print(string.format("    SR: 0x%x", MIPS.read_cp0(12, 0)))

    -- restore SR
    MIPS.write_cp0(12, 0, old_sr)
end

function MIPS.size_str(val)
    local suffix = { "", "K", "M", "G" }
    local idx = 1
    while val > 1024 do
        idx = idx + 1
        val = val / 1024.0
    end
    return string.format("%.0f%s", val, suffix[idx])
end

function MIPS.tlbr(mem_addr)
    -- jr   ra
    -- tlbr
    DEV.write32(mem_addr + 0, 0x03e00008)
    DEV.write32(mem_addr + 4, 0x42000001)
    DEV.call(mem_addr)
end

function MIPS.tlbwi(mem_addr)
    -- jr   ra
    -- tlbwi
    DEV.write32(mem_addr + 0, 0x03e00008)
    DEV.write32(mem_addr + 4, 0x42000002)
    DEV.call(mem_addr)
end

-- dump MMU, we need an executable address for tlbr instruction
function MIPS.dump_mmu(mem_addr)
    MIPS.config = MIPS.read_cp0(16, 0)
    MIPS.config1 = MIPS.read_cp0(16, 1)
    if bit32.extract(MIPS.config, 7, 3) ~= 1 then
        error("This function only handles Standard TLB format")
    end
    local nr_entries = bit32.extract(MIPS.config1, 25, 6) + 1
    local nr_wired = MIPS.read_cp0(6, 0)
    print(string.format("MMU (%d entries, %d wired):", nr_entries, nr_wired))
    for i = 0, nr_entries - 1 do
        MIPS.write_cp0(0, 0, i) -- write Index
        MIPS.tlbr(mem_addr)
        local pagemask = MIPS.read_cp0(5, 0)
        local entryhi = MIPS.read_cp0(10, 0)
        local entrylo = {[0] = MIPS.read_cp0(2, 0), [1] = MIPS.read_cp0(3, 0)}
        local pagesize = bit32.rshift(bit32.bor(pagemask + 0x1fff) + 1, 1)
        local vpn2 = bit32.extract(entryhi, 13, 19)
        local asid = bit32.extract(entryhi, 0, 8)
        for j=0,1 do
            -- check Valid
            if bit32.extract(entrylo[j], 1, 1) ~= 0 then
                local cache_type = MIPS.cache_table[bit32.extract(entrylo[j], 3, 3)]
                if cache_type == nil then cache_type = "Unknown" end
                local global = bit32.extract(entrylo[j], 0, 1) == 1
                local writeable = bit32.extract(entrylo[j], 2, 1) == 1
                local pfn = bit32.extract(entrylo[j], 6, 26)
                local virt = bit32.lshift(vpn2, 13) + j * pagesize
                local phys = bit32.lshift(pfn, 12)
                print(string.format("  %08x-%08x => %08x-%08x: %s, %s, %s, %s", virt,
                    virt + pagesize - 1, phys, phys + pagesize - 1, MIPS.size_str(pagesize),
                    cache_type, writeable and "writable" or "read-only",
                    global and "global" or string.format("ASID = %02x", asid)))
            end
        end
    end
end

-- probe MMU to see what is supports
function MIPS.probe_mmu(mem_addr)
    print("MMU/supported page sizes:")
    local list = { { "4K", 0 }, { "16K", 3}, {"64K", 0xf}, {"256K", 0x3f}, {"1M", 0xff},
                   { "4M", 0x3ff}, {"16M", 0xfff} }
    for key,setting in pairs(list) do
        local val = bit32.lshift(setting[2], 13)
        MIPS.write_cp0(5, 0, val)
        rd_val = MIPS.read_cp0(5, 0)
        print(string.format("  %s: %s", setting[1], rd_val == val and "supported" or "unsupported"))
    end
end

function MIPS.init()
    -- enable CP1 in SR
    sr_old = MIPS.read_cp0(12, 0)
    MIPS.write_cp0(12, 0, bit32.replace(sr_old, 1, 29)) -- set CU1
    print("Mips:")
    -- PRId
    MIPS.prid = MIPS.read_cp0(15, 0)
    print(string.format("  PRId: 0x%x", MIPS.prid))
    print("    CPU: " .. MIPS.get_table_or(MIPS.prid_table, MIPS.prid, "unknown"))
    -- Config
    MIPS.config = MIPS.read_cp0(16, 0)
    print(string.format("  Config: 0x%x", MIPS.config))
    print("    Architecture Type: " .. MIPS.get_table_or(MIPS.at_table,
        bit32.extract(MIPS.config, 13, 2), "unknown"))
    print("    Architecture Level: " .. MIPS.get_table_or(MIPS.ar_table,
        bit32.extract(MIPS.config, 10, 3), "unknown"))
    print("    MMU Type: " .. MIPS.get_table_or(MIPS.mt_table,
        bit32.extract(MIPS.config, 7, 3), "unknown"))
    -- Config1
    MIPS.config1 = MIPS.read_cp0(16, 1)
    print(string.format("  Config1: 0x%x", MIPS.config1))
    -- don't print of no MMU
    if bit32.extract(MIPS.config, 7, 3) ~= 0 then
        print(string.format("    MMU Size: %d", bit32.extract(MIPS.config1, 25, 6) + 1))
    end
    print("    ICache")
    print("      Sets per way: " .. MIPS.get_table_or(MIPS.is_table,
        bit32.extract(MIPS.config1, 22, 3), "unknown"))
    print("      Ways: " .. (1 + bit32.extract(MIPS.config1, 16, 3)))
    print("      Line size: " .. MIPS.get_table_or(MIPS.il_table,
        bit32.extract(MIPS.config1, 19, 3), "unknown"))
    print("    DCache")
    print("      Sets per way: " .. MIPS.get_table_or(MIPS.is_table,
        bit32.extract(MIPS.config1, 13, 3), "unknown"))
    print("      Ways: " .. (1 + bit32.extract(MIPS.config1, 7, 3)))
    print("      Line size: " .. MIPS.get_table_or(MIPS.il_table,
        bit32.extract(MIPS.config1, 10, 3), "unknown"))
    print("    FPU: " .. (bit32.extract(MIPS.config1, 0) == 1 and "yes" or "no"))

    -- Config 2
    MIPS.config2 = MIPS.read_cp0(16, 2)
    print(string.format("  Config2: 0x%x", MIPS.config2))

    -- Config 3
    MIPS.config3 = MIPS.read_cp0(16, 3)
    print(string.format("  Config3: 0x%x", MIPS.config3))
    print("    Vectored interrupt: " .. (bit32.extract(MIPS.config2, 5) and "yes" or "no"))

    -- Config 7
    MIPS.config7 = MIPS.read_cp0(16, 7)
    print(string.format("  Config7: 0x%x", MIPS.config7))

    -- restore SR
    MIPS.write_cp0(12, 0, sr_old)
end
