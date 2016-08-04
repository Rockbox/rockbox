XBURST = {}

function XBURST.read_cp0(reg, sel)
    return DEV.read32_cop({0, reg, sel})
end

function XBURST.write_cp0(reg, sel, val)
    DEV.write32_cop({0, reg, sel}, val)
end

XBURST.prid_table = {
    [0x0ad0024f] = "JZ4740",
    [0x1ed0024f] = "JZ4755",
    [0x2ed0024f] = "JZ4760(B)",
    [0x3ee1024f] = "JZ4780"
}

XBURST.at_table = {
    [0] = "MIPS32",
    [1] = "MIPS64 with 32-bit segments",
    [2] = "MIPS64"
}

XBURST.ar_table = {
    [0] = "Release 1",
    [1] = "Release 2 (or more)"
}

XBURST.mt_table = {
    [0] = "None",
    [1] = "Standard TLB",
    [2] = "BAT",
    [3] = "Fixed Mapping",
    [4] = "Dual VTLB and FTLB"
}

XBURST.is_table = {
    [0] = 64,
    [1] = 128,
    [2] = 256,
    [3] = 512,
    [4] = 1024,
    [5] = 2048,
    [6] = 4096,
    [7] = 32
}

XBURST.il_table = {
    [0] = 0,
    [1] = 4,
    [2] = 8,
    [3] = 16,
    [4] = 32,
    [5] = 64,
    [6] = 128
}

function XBURST.get_table_or(tbl, index, dflt)
    if tbl[index] ~= nil then
        return tbl[index]
    else
        return dflt
    end
end

function XBURST.do_ebase_test()
    XBURST.write_cp0(15, 1, 0x80000000)
    print(string.format("    Value after writing 0x80000000: 0x%x", XBURST.read_cp0(15, 1)))
    if XBURST.read_cp0(15, 1) ~= 0x80000000 then
        return "Value 0x8000000 does not stick, EBASE is probably not working"
    end
    XBURST.write_cp0(15, 1, 0x80040000)
    print(string.format("    Value after writing 0x80040000: 0x%x", XBURST.read_cp0(15, 1)))
    if XBURST.read_cp0(15, 1) ~= 0x80040000 then
        return "Value 0x80040000 does not stick, EBASE is probably not working"
    end
    return "EBase seems to work"
end

function XBURST.do_ebase_cfg7gate_test()
    -- test gate in config7
    config7_old = XBURST.read_cp0(16, 7)
    XBURST.write_cp0(16, 7, bit32.replace(config7_old, 0, 7)) -- disable EBASE[30] modification
    print(string.format("    Disable config7 gate: write 0x%x to Config7", bit32.replace(config7_old, 0, 7)))
    XBURST.write_cp0(15, 1, 0xfffff000)
    print(string.format("    Value after writing 0xfffff000: 0x%x", XBURST.read_cp0(15, 1)))
    if XBURST.read_cp0(15, 1) == 0xfffff000 then
        return "Config7 gate has no effect but modifications are allowed anyway"
    end
    XBURST.write_cp0(16, 7, bit32.replace(config7_old, 1, 7)) -- enable EBASE[30] modification
    print(string.format("    Enable config7 gate: write 0x%x to Config7", bit32.replace(config7_old, 1, 7)))
    XBURST.write_cp0(15, 1, 0xc0000000)
    print(string.format("    Value after writing 0xc0000000: 0x%x", XBURST.read_cp0(15, 1)))
    if XBURST.read_cp0(15, 1) ~= 0xc0000000 then
        return "Config7 gate does not work"
    end
    XBURST.write_cp0(16, 7, config7_old)
    return "Config7 gate seems to work"
end

function XBURST.do_ebase_exc_test(mem_addr)
    if (mem_addr % 0x1000) ~= 0 then
        return "  memory address for exception test must aligned on a 0x1000 boundary";
    end
    print(string.format("Exception test with EBASE at 0x%x...", mem_addr))
    print("  Writing instructions to memory")
    -- create instructions in memory
    exc_addr = mem_addr + 0x180 -- general exception vector
    data_addr = mem_addr + 0x300
    -- lui k0,<low part of data_addr>
    -- ori     k0,k0,0xbeef
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
    old_sr = XBURST.read_cp0(12, 0)
    print(string.format("  Old SR: 0x%x", old_sr))
    XBURST.write_cp0(12, 0, 0xfc00) -- BEV set to 0, all interrupts masked and interrupt disabled
    print(string.format("  New SR: 0x%x", XBURST.read_cp0(12, 0)))
    -- change EBASE
    old_ebase = XBURST.read_cp0(15, 1)
    XBURST.write_cp0(15, 1, mem_addr)
    print(string.format("  EBASE: %x", XBURST.read_cp0(15, 1)))
    -- test
    print(string.format("  Before: %x", DEV.read32(data_addr)))
    DEV.call(bug_addr)
    print(string.format("  After: %x", DEV.read32(data_addr)))
    success = DEV.read32(data_addr) == 0xdeadbeef
    -- restore SR and EBASE
    XBURST.write_cp0(12, 0, old_sr)
    XBURST.write_cp0(15, 1, ebase_old)

    return success and "Exception and EBASE are working" or "Exception and EBASE are NOT working"
end

function XBURST.test_ebase(mem_addr)
    -- EBase
    ebase_old = XBURST.read_cp0(15, 1)
    sr_old = XBURST.read_cp0(12, 0)
    print("Testing EBASE...")
    print("  Disable BEV")
    XBURST.write_cp0(12, 0, bit32.replace(sr_old, 0, 22)) -- clear BEV
    print(string.format("  SR value: 0x%x", XBURST.read_cp0(12, 0)))
    print(string.format("  EBASE value: 0x%x", ebase_old))
    print("  Test result: " .. XBURST.do_ebase_test())
    print("  Config7 result: " .. XBURST.do_ebase_cfg7gate_test())
    XBURST.write_cp0(12, 0, sr_old)
    XBURST.write_cp0(15, 1, ebase_old)
    -- now try with actual exceptions
    if mem_addr == nil then
        print("  Not doing exception test, please specify memory to use: sram, ram")
        return
    end
    print("  Exception result: " .. XBURST.do_ebase_exc_test(mem_addr))
end

function XBURST.init()
    -- enable CP1 in SR
    sr_old = XBURST.read_cp0(12, 0)
    XBURST.write_cp0(12, 0, bit32.replace(sr_old, 1, 29)) -- set CU1
    print("XBurst:")
    -- PRId
    XBURST.prid = XBURST.read_cp0(15, 0)
    print(string.format("  PRId: 0x%x", XBURST.prid))
    print("    CPU: " .. XBURST.get_table_or(XBURST.prid_table, XBURST.prid, "unknown"))
    -- Config
    XBURST.config = XBURST.read_cp0(16, 0)
    print(string.format("  Config: 0x%x", XBURST.config))
    print("    Architecture Type: " .. XBURST.get_table_or(XBURST.at_table,
        bit32.extract(XBURST.config, 13, 2), "unknown"))
    print("    Architecture Level: " .. XBURST.get_table_or(XBURST.ar_table,
        bit32.extract(XBURST.config, 10, 3), "unknown"))
    print("    MMU Type: " .. XBURST.get_table_or(XBURST.mt_table,
        bit32.extract(XBURST.config, 7, 3), "unknown"))
    -- Config1
    XBURST.config1 = XBURST.read_cp0(16, 1)
    print(string.format("  Config1: 0x%x", XBURST.config1))
    -- don't print of no MMU
    if bit32.extract(XBURST.config, 7, 3) ~= 0 then
        print(string.format("    MMU Size: %d", bit32.extract(XBURST.config1, 25, 6) + 1))
    end
    print("    ICache")
    print("      Sets per way: " .. XBURST.get_table_or(XBURST.is_table,
        bit32.extract(XBURST.config1, 22, 3), "unknown"))
    print("      Ways: " .. (1 + bit32.extract(XBURST.config1, 16, 3)))
    print("      Line size: " .. XBURST.get_table_or(XBURST.il_table,
        bit32.extract(XBURST.config1, 19, 3), "unknown"))
    print("    DCache")
    print("      Sets per way: " .. XBURST.get_table_or(XBURST.is_table,
        bit32.extract(XBURST.config1, 13, 3), "unknown"))
    print("      Ways: " .. (1 + bit32.extract(XBURST.config1, 7, 3)))
    print("      Line size: " .. XBURST.get_table_or(XBURST.il_table,
        bit32.extract(XBURST.config1, 10, 3), "unknown"))
    print("    FPU: " .. (bit32.extract(XBURST.config1, 0) == 1 and "yes" or "no"))

    -- Config 2
    XBURST.config2 = XBURST.read_cp0(16, 2)
    print(string.format("  Config2: 0x%x", XBURST.config2))

    -- Config 3
    XBURST.config3 = XBURST.read_cp0(16, 3)
    print(string.format("  Config3: 0x%x", XBURST.config3))
    print("    Vectored interrupt: " .. (bit32.extract(XBURST.config2, 5) and "yes" or "no"))

    -- Config 7
    XBURST.config7 = XBURST.read_cp0(16, 7)
    print(string.format("  Config7: 0x%x", XBURST.config7))

    -- restore SR
    XBURST.write_cp0(12, 0, sr_old)
end
