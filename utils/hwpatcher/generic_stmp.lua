--[[
Generic STMP hacking
required argument (in order):
- path to firmware
- path to output firmware
- path to blob
- path to stub
]]--
require("lib")
require("arm")

if #arg < 4 then
    error("usage: <fw file> <out file> <blob> <stub>")
end

-- compute MD5
print("Computing MD5 sum of the firmware...")
local md5 = hwp.md5sum(arg[1])
print("=> " .. hwp.md5str(md5))

local md5_db =
{
    ["d0047f8a87d456a0032297b3c802a1ff"] =
    {
        model = "Sony NWZ-E3600 1.0.0",
        irq_addr_pool = 0x40A314E4,
        irq_addr_pool_sec = "play.1",
        -- proxy_addr = 0x4005C1E0,
        -- proxy_addr_sec = "play.1"
        proxy_addr = 0x4007C258,
        proxy_addr_sec = "play.1",
        -- stub_addr = 0x1971C8,
        -- stub_addr_virt = 0x2971C8,
        -- stub_addr_sec = "pvmi",
    },
    ["f42742d4d90d88e2fb6ff468c1389f5f"] =
    {
        model = "Creative ZEN X-Fi Style 1.03.04",
        irq_addr_pool = 0x402D3A64,
        irq_addr_pool_sec = "play.1",
        proxy_addr = 0x402E076C,
        proxy_addr_sec = "play.1"
    },
    ["c180f57e2b2d62620f87a1d853f349ff"] =
    {
        model = "Creative ZEN X-Fi3 1.00.25e",
        irq_addr_pool = 0x405916f0,
        proxy_addr = 0x40384674,
    }
}

local db_entry = md5_db[hwp.md5str(md5)]
if db_entry == nil then
    error("Cannot find device in the DB")
    os.exit(1)
end
print("Model: " .. db_entry.model)

local fw = hwp.load_file(arg[1])
local irq_addr_pool = hwp.make_addr(db_entry.irq_addr_pool, db_entry.irq_addr_pool_sec)
local proxy_addr = arm.to_arm(hwp.make_addr(db_entry.proxy_addr, db_entry.proxy_addr_sec))
-- read old IRQ address pool
local old_irq_addr = hwp.make_addr(hwp.read32(fw, irq_addr_pool))
print(string.format("Old IRQ address: %s", old_irq_addr))
-- put stub at the beginning of the proxy
local stub = hwp.load_bin_file(arg[4])
local stub_info = hwp.section_info(stub, "")
local stub_data = hwp.read(stub, hwp.make_addr(stub_info.addr, ""), stub_info.size)
local stub_addr = nil
local stub_addr_virt = nil
if db_entry.stub_addr ~= nil then
    stub_addr = arm.to_arm(hwp.make_addr(db_entry.stub_addr, db_entry.stub_addr_sec))
    if db_entry.stub_addr_virt ~= nil then
        stub_addr_virt = arm.to_arm(hwp.make_addr(db_entry.stub_addr_virt, db_entry.stub_addr_sec)) 
    else
        stub_addr_virt = stub_addr
    end
    hwp.write(fw, stub_addr, stub_data)
else
    stub_addr = proxy_addr
    stub_addr_virt = stub_addr
    hwp.write(fw, stub_addr, stub_data)
    proxy_addr = hwp.inc_addr(proxy_addr, stub_info.size)
end
-- modify irq
hwp.write32(fw, irq_addr_pool, proxy_addr.addr)
print(string.format("New IRQ address: %s", proxy_addr))
-- in proxy, save registers
arm.write_save_regs(fw, proxy_addr)
proxy_addr = hwp.inc_addr(proxy_addr, 4)
-- load blob
local blob = hwp.load_bin_file(arg[3])
local blob_info = hwp.section_info(blob, "")
-- patch blob with stub address
hwp.write32(blob, hwp.make_addr(blob_info.addr + 4, ""), stub_addr_virt.addr)
-- write it !
local blob_data = hwp.read(blob, hwp.make_addr(blob_info.addr, ""), blob_info.size)
hwp.write(fw, proxy_addr, blob_data)
proxy_addr = hwp.inc_addr(proxy_addr, blob_info.size)
-- restore registers
arm.write_restore_regs(fw, proxy_addr)
proxy_addr = hwp.inc_addr(proxy_addr, 4)
-- branch to old code
local branch_to_old = arm.make_branch(old_irq_addr, false)
arm.write_branch(fw, proxy_addr, branch_to_old, hwp.inc_addr(proxy_addr, 4))
-- save
hwp.save_file(fw, arg[2])
 
