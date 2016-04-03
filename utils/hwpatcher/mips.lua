--[[
hwpatcher mips decoding/encoding library 

]]--
mips = {}

-- sign extend a value to 32-bits
-- only the lower 'bits' bits are considered, everything else is trashed
-- watch out arithmetic vs logical shift !
function mips.sign32(v)
    if bit32.extract(v, 31) == 1 then
        return -1 - bit32.bnot(v)
    else
        return v
    end
end

function mips.sign_extend(val, bits)
    return mips.sign32(bit32.arshift(bit32.lshift(val, 32 - bits), 32 - bits))
end

-- check that a signed value fits in some field
function mips.check_sign_truncation(val, bits)
    return val == mips.sign_extend(val, bits)
end

-- generate the encoding of a PC relative branch
-- if the branch cannot be encoded using an immediate branch, an error is thrown
function mips.write_pc_branch(fw, addr, dest)
    local offset = dest.addr - addr.addr
    local opcode = 0
    offset = offset - 4
    offset = offset / 4
    if not mips.check_sign_truncation(offset, 16) then
        error("destination is too far for PC relative branch")
    end
    opcode = 0x10000000 + -- BEQ opcode
        0 + 0 + -- rs=r0 and rt=0
        offset -- offset
    return hwp.write32(fw, addr, opcode)
end

function mips.write_nop(fw, addr)
    local opcode = 0 -- sll r0, r0, 0
    return hwp.write32(fw, addr, opcode)
end

function mips.write_branch_reg(fw, addr, reg)
    local opcode = 0
    opcode = 0x8 -- JR opcode
        + bit32.lshift(reg, 21)
    return hwp.write32(fw, addr, opcode)
end

function mips.write_move(fw, addr, dst, src)
    local opcode = 0
    opcode = 0x25 -- OR opcode
        + bit32.lshift(dst, 11) -- rd = dst
        + bit32.lshift(src, 16) -- rt = src
        + 0 -- rs = $zero
    return hwp.write32(fw, addr, opcode)
end