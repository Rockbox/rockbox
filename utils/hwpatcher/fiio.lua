--[[
Generic STMP hacking
required argument (in order):
- path to firmware
- path to output firmware
- path to blob
- path to stub
]]--
require("lib")
require("mips")

if #arg < 2 then
    error("usage: <fw file> <out file>")
end

-- compute MD5
print("Computing MD5 sum of the firmware...")
local md5 = hwp.md5sum(arg[1])
print("=> " .. hwp.md5str(md5))

local md5_db =
{
    ["7285eca3475e5b5d69e4a324d9909c88"] =
    {
        model = "Fiio X1 HW V1",
        after_ram_init_addr = 0x800008e8,
        --after_ram_init_addr = 0x800008d8,
        ra_reload_addr = 0x80000944,
        nop_addr_list = { 0x800008f4, 0x80000904, 0x80000930 },
        jump_patch_addr = 0x8000094c
    },
}

local db_entry = md5_db[hwp.md5str(md5)]
if db_entry == nil then
    error("Cannot find device in the DB")
    os.exit(1)
end
print("Model: " .. db_entry.model)
-- load firmware and change virtual address
local fw = hwp.load_bin_file(arg[1])
hwp.move_section(fw, hwp.make_addr(0x80000000, ""))
-- the NAND IPL starts at 0xa0, so we need to insert a jump to start at 0
start_addr = hwp.make_addr(0x80000000, "")
realstart_addr = hwp.make_addr(0x800000a0, "")
mips.write_pc_branch(fw, start_addr, realstart_addr)
mips.write_nop(fw, hwp.inc_addr(start_addr, 4))
-- in the middle of the code, right after RAM init, skip over NAND loading code
if false then
    after_ram_init_addr = hwp.make_addr(db_entry.after_ram_init_addr, "")
    ra_reload_addr = hwp.make_addr(db_entry.ra_reload_addr, "")
    mips.write_pc_branch(fw, after_ram_init_addr, ra_reload_addr)
    mips.write_nop(fw, hwp.inc_addr(after_ram_init_addr, 4))
    -- the previous skip land on the $ra reload, now insert a return to $ra
    mips.write_branch_reg(fw, hwp.inc_addr(ra_reload_addr, 4), 31)
    mips.write_nop(fw, hwp.inc_addr(ra_reload_addr, 8))
end
-- put some nops around
for index,addr in ipairs(db_entry.nop_addr_list) do
    mips.write_nop(fw, hwp.make_addr(addr, ""))
end
-- fix jump addr to return to $ra instead of $t9
mips.write_move(fw, hwp.make_addr(db_entry.jump_patch_addr, ""), 25, 31) -- move $t9, $ra
-- save firmware
hwp.save_file(fw, arg[2])
