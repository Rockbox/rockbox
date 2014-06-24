--[[
hwpatcher arm decoding/encoding library 

]]--
arm = {}

-- determines whether an address is in Thumb code or not
function arm.is_thumb(addr)
    return bit32.extract(addr.addr, 0) == 1
end

-- translate address to real address (ie without Thumb bit)
-- produces an error if address is not properly aligned in ARM mode
function arm.xlate_addr(addr)
    local res = hwp.deepcopy(addr)
    if arm.is_thumb(addr) then
        res.addr = bit32.replace(addr.addr, 0, 0)
    elseif bit32.extract(addr.addr, 0, 2) ~= 0 then
        error("ARM address is not word-aligned")
    end
    return res
end

-- switch between arm and thumb
function arm.to_thumb(addr)
    local res = hwp.deepcopy(addr)
    res.addr = bit32.bor(addr.addr, 1)
    return res
end

function arm.to_arm(addr)
    return arm.xlate_addr(addr)
end

-- sign extend a value to 32-bits
-- only the lower 'bits' bits are considered, everything else is trashed
-- watch out arithmetic vs logical shift !
function arm.sign32(v)
    if bit32.extract(v, 31) == 1 then
        return -1 - bit32.bnot(v)
    else
        return v
    end
end
function arm.sign_extend(val, bits)
    return arm.sign32(bit32.arshift(bit32.lshift(val, 32 - bits), 32 - bits))
end

-- check that a signed value fits in some field
function arm.check_sign_truncation(val, bits)
    return val == arm.sign_extend(val, bits)
end

-- create a branch description
function arm.make_branch(addr, link)
    local t = {type = "branch", addr = addr, link = link}
    local branch_to_string = function(self)
        return string.format("branch(%s,%s)", self.addr, self.link)
    end
    setmetatable(t, {__tostring = branch_to_string})
    return t
end

-- parse a jump and returns its description
function arm.parse_branch(fw, addr)
    local opcode = hwp.read32(fw, arm.xlate_addr(addr))
    if arm.is_thumb(addr) then
        if bit32.band(opcode, 0xf800) ~= 0xf000 then
            error("first instruction is not a bl(x) prefix")
        end
        local to_thumb = false
        if bit32.band(opcode, 0xf8000000) == 0xf8000000 then
            to_thumb = true
        elseif bit32.band(opcode, 0xf8000000) ~= 0xe8000000 then
            error("second instruction is not a bl(x) suffix")
        end
        local dest = hwp.make_addr(bit32.lshift(arm.sign_extend(opcode, 11), 12) + 
            arm.xlate_addr(addr).addr + 4 +
            bit32.rshift(bit32.band(opcode, 0x7ff0000), 16) * 2, addr.section)
        if to_thumb then
            dest = arm.to_thumb(dest)
        else
            dest.addr = bit32.replace(dest.addr, 0, 0, 2)
        end
        return arm.make_branch(dest, true)
    else
        if bit32.band(opcode, 0xfe000000) == 0xfa000000 then -- BLX
            local dest = hwp.make_addr(arm.sign_extend(opcode, 24) * 4 + 8 +
                bit32.extract(opcode, 24) * 2 + arm.xlate_addr(addr).addr, addr.section)
            return arm.make_branch(arm.to_thumb(dest), true)
        elseif bit32.band(opcode, 0xfe000000) == 0xea000000 then -- B(L)
            local dest = hwp.make_addr(arm.sign_extend(opcode, 24) * 4 + 8 +
                arm.xlate_addr(addr).addr, addr.section)
            return arm.make_branch(arm.to_arm(dest), bit32.extract(opcode, 24))
        else
            error("instruction is not a valid branch")
        end
    end
end

-- generate the encoding of a branch
-- if the branch cannot be encoded using an immediate branch, it is generated
-- with an indirect form like a ldr, using a pool
function arm.write_branch(fw, addr, dest, pool)
    local offset = arm.xlate_addr(dest.addr).addr - arm.xlate_addr(addr).addr
    local exchange = arm.is_thumb(addr) ~= arm.is_thumb(dest.addr)
    local opcode = 0
    if arm.is_thumb(addr) then
        if not dest.link then
            return arm.write_load_pc(fw, addr, dest, pool)
        end
        offset = offset - 4 -- in Thumb, PC+4 relative
        -- NOTE: BLX is undefined if the resulting offset has bit 0 set, follow
        -- the procedure from the manual to ensure correct operation
        if bit32.extract(offset, 1) ~= 0 then
            offset = offset + 2
        end
        offset = offset / 2
        if not arm.check_sign_truncation(offset, 22) then
            error("destination is too far for immediate branch from thumb")
        end
        opcode = 0xf000 + -- BL/BLX prefix
            bit32.band(bit32.rshift(offset, 11), 0x7ff) + -- offset (high part)
            bit32.lshift(exchange and 0xe800 or 0xf800,16) + -- BLX suffix
            bit32.lshift(bit32.band(offset, 0x7ff), 16) -- offset (low part)
    else
        offset = offset - 8 -- in ARM, PC+8 relative
        if exchange and not dest.link then
            return arm.write_load_pc(fw, addr, dest, pool)
        end
        offset = offset / 4
        if not arm.check_sign_truncation(offset, 24) then
            if pool == nil then
                error("destination is too far for immediate branch from arm (no pool available)")
            else
                return arm.write_load_pc(fw, addr, dest, pool)
            end
        end
        opcode = bit32.lshift(exchange and 0xf or 0xe, 28) + -- BLX / AL cond
            bit32.lshift(0xa, 24) + -- branch
            bit32.lshift(exchange and bit32.extract(offset, 1) or dest.link and 1 or 0, 24) + -- link / bit1
            bit32.band(offset, 0xffffff)
    end
    return hwp.write32(fw, arm.xlate_addr(addr), opcode)
end

function arm.write_load_pc(fw, addr, dest, pool)
    -- write pool
    hwp.write32(fw, pool, dest.addr.addr)
    -- write "ldr pc, [pool]"
    local opcode
    if arm.is_thumb(addr) then
        error("unsupported pc load in thumb mode")
    else
        local offset = pool.addr - arm.xlate_addr(addr).addr - 8 -- ARM is PC+8 relative
        local add = offset >= 0 and 1 or 0
        offset = math.abs(offset)
        opcode = bit32.lshift(0xe, 28) + -- AL cond
            bit32.lshift(1, 26) + -- ldr/str
            bit32.lshift(1, 24) + -- P
            bit32.lshift(add, 23) + -- U
            bit32.lshift(1, 20) + -- ldr
            bit32.lshift(15, 16) + -- Rn=PC
            bit32.lshift(15, 12) + -- Rd=PC
            bit32.band(offset, 0xfff)
    end
    return hwp.write32(fw, arm.xlate_addr(addr), opcode)
end

-- generate the encoding of a "bx lr"
function arm.write_return(fw, addr)
    if arm.is_thumb(addr) then
        error("unsupported return from thumb code")
    end
    local opcode = bit32.lshift(0xe, 28) + -- AL cond
        bit32.lshift(0x12, 20) + -- BX
        bit32.lshift(1, 4) + -- BX
        bit32.lshift(0xfff, 8) + -- SBO
        14 -- LR
    hwp.write32(fw, arm.xlate_addr(addr), opcode)
end

function arm.write_xxx_regs(fw, addr, load)
    if arm.is_thumb(addr) then
        error("unsupported save/restore regs from thumb code")
    end
    -- STMFD sp!,{r0-r12, lr}
    local opcode = bit32.lshift(0xe, 28) + -- AL cond
        bit32.lshift(0x4, 25) + -- STM/LDM
        bit32.lshift(load and 0 or 1,24) + -- P
        bit32.lshift(load and 1 or 0, 23) + -- U
        bit32.lshift(1, 21) +-- W
        bit32.lshift(load and 1 or 0, 20) + -- L
        bit32.lshift(13, 16) + -- base = SP
        0x5fff -- R0-R12,LR
    return hwp.write32(fw, addr, opcode)
end

function arm.write_save_regs(fw, addr)
    return arm.write_xxx_regs(fw, addr, false)
end

function arm.write_restore_regs(fw, addr)
    return arm.write_xxx_regs(fw, addr, true)
end