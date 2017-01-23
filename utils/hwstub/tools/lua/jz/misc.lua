----------
-- MISC --
----------

JZ.misc = {}

function JZ.misc.enable_sram()
    HW.CPM.CLKGATE1.SRAM.clr()
    HW.CPM.CLKGATE1.AHB1.clr()
end

function JZ.misc.test_sram()
    DEV.write32(0xb32d0000, 0xaaaa5555)
    if DEV.read32(0xb32d0000) ~= 0xaaaa5555 then
        error("SRAM is not working")
    end
    DEV.write32(0xb32d0000, 0xdeadbeef)
    if DEV.read32(0xb32d0000) ~= 0xdeadbeef then
        error("SRAM is not working")
    end
    print("SRAM seems to be working")
    size = 0
    for i=4,64*1024,4 do
        DEV.write32(0xb32d0000, 0xdeadbeef)
        DEV.write32(0xb32d0000 + i, 0xcafebabe)
        if DEV.read32(0xb32d0000 + i) ~= 0xcafebabe or DEV.read32(0xb32d0000) == 0xcafebabe then
            size = i
            break
        end
    end
    print(string.format("SRAM size: 0x%x (%d KiB)", size, size / 1024))
    -- double check
    for i=0,size-1,2 do
        DEV.write16(0xb32d0000 + i, i)
    end
    for i=0,size-1,2 do
        if DEV.read16(0xb32d0000 + i) ~= i then
            error(string.format("SRAM size is not confirmed: read @%x gives %d instead of %d",
                0xb32d0000 + i, DEV.read16(0xb32d0000 + i), i))
        end
    end
    print("SRAM size confirmed and working")
end
