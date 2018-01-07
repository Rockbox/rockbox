require "mips"

XBURST = {}

function XBURST.enable_big_page_mode()
    -- without this, the TLB behaves in a strange way (EntyLo1 is not independent of EntryLo0)
    -- and big pages are not support
    MIPS.write_cp0(5, 4, 0xa9000000)
end

function XBURST.disable_big_page_mode()
    -- without this, the TLB behaves in a strange way (EntyLo1 is not independent of EntryLo0)
    -- and big pages are not support
    MIPS.write_cp0(5, 4, 0)
end

function XBURST.do_ebase_cfg7gate_test()
    -- test gate in config7
    config7_old = MIPS.read_cp0(16, 7)
    MIPS.write_cp0(16, 7, bit32.replace(config7_old, 0, 7)) -- disable EBASE[30] modification
    print(string.format("    Disable config7 gate: write 0x%x to Config7", bit32.replace(config7_old, 0, 7)))
    MIPS.write_cp0(15, 1, 0xfffff000)
    print(string.format("    Value after writing 0xfffff000: 0x%x", MIPS.read_cp0(15, 1)))
    if MIPS.read_cp0(15, 1) == 0xfffff000 then
        error("Config7 gate has no effect but modifications are allowed anyway")
    end
    MIPS.write_cp0(16, 7, bit32.replace(config7_old, 1, 7)) -- enable EBASE[30] modification
    print(string.format("    Enable config7 gate: write 0x%x to Config7", bit32.replace(config7_old, 1, 7)))
    MIPS.write_cp0(15, 1, 0xc0000000)
    print(string.format("    Value after writing 0xc0000000: 0x%x", MIPS.read_cp0(15, 1)))
    if MIPS.read_cp0(15, 1) ~= 0xc0000000 then
        error("Config7 gate does not work")
    end
    MIPS.write_cp0(16, 7, config7_old)
    print("Config7 gate seems to work")
end
