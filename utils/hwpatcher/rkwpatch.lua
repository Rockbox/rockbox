--[[
RKW patching tool
required argument (in order):
- path to rkw
- path to stub
- physical address where to put bin blob
- physical address where to put jump
- path to output patched firmware
]]-- 
require("lib")
require("arm")

function printf(...)
    io.write(string.format(...))
end

if #arg < 5 then
    printf("Arguments: source.rkw blob.bin blob_address jump_address output.rkw\n")
    printf("source.rkw\tRKW file to be patched\n")
    printf("blob.bin\tArbitrary binary to be implanted (e.g hwstub.bin)\n")
    printf("blob_address\tPhysical address where to implant blob (e.g 0x6008300c)\n")
    printf("jump_address\tPhysical address where to put jump to implanted binary (e.g 0x60097f2c)\n")
    printf("output.rkw\tResulting RKW file\n")
    os.exit(1)
end

-- return rkw file offset based on physical runtime mem addr
-- sdram base address is 0x60000000 and rkw header is 0x2c long
function addr2rkw(addr)
    return (addr + 0x2c - 0x60000000)
end

-- read input file
local fw = hwp.load_file(arg[1])

-- read and check RKW magic number
local rkw_magic = hwp.read32(fw, hwp.make_addr(0))

if rkw_magic ~=  0x4c44524b then
    printf("error: wrong RKW magic number\n")
    os.exit(1)
end

-- check RKW header size
local rkw_header_size = hwp.read32(fw, hwp.make_addr(0x04))

if rkw_header_size ~= 0x2c then
    printf("error: RKW header size 0x%0x should be 0x2c!\n", rkw_header_size)
    os.exit(1)
end

-- check RKW header CRC
local header_crc = hwp.crc(RKW, fw, hwp.make_addr(0), 0x28)

if hwp.read32(fw, hwp.make_addr(0x28)) ~= header_crc then
    printf("error: RKW header CRC mismatch\n")
    os.exit(1)
end

local firmware_blob = hwp.make_addr(rkw_header_size)
local blob_size = hwp.read32(fw, hwp.make_addr(0x10)) - hwp.read32(fw, hwp.make_addr(0x8))
local rkw_crc = 0

-- check if blob has CRC attached
if blob_size < hwp.section_info(fw, "").size then
    rkw_crc = hwp.crc(RKW, fw, firmware_blob, blob_size)

    if hwp.read32(fw, hwp.make_addr(blob_size+rkw_header_size)) ~= rkw_crc then
        printf("error: RKW CRC mismatch\n")
        os.exit(1)
    end
else
    printf("error: blob reported size: 0x%0x >= actual blob size: 0x%0x\n", blob_size, hwp.section_info(fw, "").size)
    os.exit(1)
end

printf("RKW sanity checks passed\n")
printf("RKW magic:\t0x%0x\n", rkw_magic)
printf("Header size:\t0x%0x\n", rkw_header_size)
printf("Image Base:\t0x%0x\n", hwp.read32(fw, hwp.make_addr(0x8)))
printf("Load Base:\t0x%0x\n", hwp.read32(fw, hwp.make_addr(0xc)))
printf("Load Limit:\t0x%0x\n", hwp.read32(fw, hwp.make_addr(0x10)))
printf("ZI Base:\t0x%0x\n", hwp.read32(fw, hwp.make_addr(0x14)))
printf("Entry point:\t0x%0x\n", hwp.read32(fw, hwp.make_addr(0x20)))
printf("Load flags:\t0x%0x\n", hwp.read32(fw, hwp.make_addr(0x24)))
printf("Header crc:\t0x%0x\n", header_crc)
printf("Blob crc:\t0x%0x\n", rkw_crc)

-- branch instruction
local jump_instr_addr = hwp.make_addr(addr2rkw(arg[4]))
if (jump_instr_addr == nil) then
    printf("error: invalid jump instruction address\n")
    os.exit(1)
end

local stub_addr = hwp.make_addr(addr2rkw(arg[3])) -- some decoder stuff
if (stub_addr == nil) then
    printf("error: invalid blob implant address\n")
    os.exit(1)
end

-- put stub at the right place
local stub = hwp.load_bin_file(arg[2])
if (stub == nil) then
    printf("error: can't load blob file: %s\n", arg[2])
    os.exit(1)
end

local stub_info = hwp.section_info(stub, "")
local stub_data = hwp.read(stub, hwp.make_addr(stub_info.addr, ""), stub_info.size)
hwp.write(fw, stub_addr, stub_data)
printf("Implanting blob at: 0x%0x\n", arg[3])

-- patch jump
local branch_to_stub = arm.make_branch(arm.to_arm(stub_addr), false)
arm.write_branch(fw, jump_instr_addr, branch_to_stub, hwp.inc_addr(stub_addr, stub_info.size))
printf("Patching jump instruction at: 0x%0x\n", arg[4])

-- patch rkw crc
rkw_crc = hwp.crc(RKW, fw, firmware_blob, blob_size)
printf("Patching RKW with new CRC: 0x%0x\n", rkw_crc)
hwp.write32(fw, hwp.make_addr(blob_size+rkw_header_size), rkw_crc)

-- save
hwp.save_file(fw, arg[5])
printf("Saving output to: '%s'\n", arg[5])
