--[[
Creative ZEN hacking
required argument (in order):
- path to firmware
- path to output firmware
- path to blob
- path to stub
]]-- 

if #arg < 4 then
    error("not enough argument to fuzep patcher")
end

local fw = hwp.load_file(arg[1])
local irq_addr_pool = hwp.make_addr(0x38)
local proxy_addr = arm.to_arm(hwp.make_addr(0x402519A0))
-- read old IRQ address pool
local old_irq_addr = hwp.make_addr(hwp.read32(fw, irq_addr_pool))
print(string.format("Old IRQ address: %s", old_irq_addr))
-- put stub at the beginning of the proxy
local stub = hwp.load_bin_file(arg[4])
local stub_info = hwp.section_info(stub, "")
local stub_data = hwp.read(stub, hwp.make_addr(stub_info.addr, ""), stub_info.size)
hwp.write(fw, proxy_addr, stub_data)
local stub_addr = proxy_addr
proxy_addr = hwp.inc_addr(proxy_addr, stub_info.size)
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
hwp.write32(blob, hwp.make_addr(blob_info.addr + 4, ""), stub_addr.addr)
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
