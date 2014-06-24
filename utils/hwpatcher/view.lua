--[[
Sansa View bootloader hacking
required argument (in order):
- path to bootloader
- path to output bootloader
- path to stub
]]-- 
require("lib")
require("arm")

if #arg < 3 then
    error("not enough argument to fuzep patcher")
end

local md5 = hwp.md5sum(arg[1])
if hwp.md5str(md5) ~= "4bc1760327c37b9ffd00315c8aa7f376" then
    error("MD5 sum of the file doesn't match")
end

local fw = hwp.load_file(arg[1])
local jump_instr_addr = arm.to_thumb(hwp.make_addr(0x753C))
local stub_addr = hwp.make_addr(0x137B0)
-- read old jump address
--local old_jump = arm.parse_branch(fw, jump_instr_addr)
--print(string.format("Old jump address: %s", old_jump))
-- put stub at the right place
local stub = hwp.load_bin_file(arg[3])
local stub_info = hwp.section_info(stub, "")
local stub_data = hwp.read(stub, hwp.make_addr(stub_info.addr, ""), stub_info.size)
hwp.write(fw, stub_addr, stub_data)
-- patch jump
local branch_to_stub = arm.make_branch(arm.to_arm(stub_addr), true)
arm.write_branch(fw, jump_instr_addr, branch_to_stub, hwp.inc_addr(stub_addr, stub_info.size))
-- read jump address
local new_jump = arm.parse_branch(fw, jump_instr_addr)
print(string.format("New jump address: %s", new_jump))
-- save
hwp.save_file(fw, arg[2]) 
