--[[
hwpatcher library

The C code provides the following functions.

At global level:
- quit()    Quit the interactive mode
- exit()    Same as quit()

In the hwp table:
- load_file(filename)           Load a firmware and guess type
- load_elf_file(filename)       Load a firmware as ELF
- load_sb_file(filename)        Load a firmware as SB
- load_sb1_file(filename)       Load a firmware as SB1
- load_bin_file(filename)       Load a firmware as binary
- save_file(fw, filename)       Save a firmware to a file
- read(fw, addr, len)           Read data from a firmware
- write(fw, addr, data)         Write data to a firmware
- section_info(fw, sec)         Return information about a section in a table (or nil)
- md5sum(filename)              Compute the MD5 sum of a file
- crc_buf(crc_type, buf)        Compute the CRC of a byte buffer and return a byte buffer
- crc(crc_type, fw, addr, len)  Compute the CRC of a firmware part

The list of CRCs available is available the hwp.CRC table:
- RKW

Data read/written from/to a firmware must must be an array of bytes.
The address must be a table of the following fields:
- address: contain the address
- section: optional section name
Data section information is a table with the following fields:
- address: first address if the section
- size: size of the section
We provide the following functions to help dealing with addresses:
- make_addr(addr, section)      Build a firmware address from a raw address and a section

]]--

function hwp.deepcopy(o, seen)
    seen = seen or {}
    if o == nil then return nil end
    if seen[o] then return seen[o] end

    local no
    if type(o) == 'table' then
        no = {}
        seen[o] = no

        for k, v in next, o, nil do
            no[hwp.deepcopy(k, seen)] = hwp.deepcopy(v, seen)
        end
        setmetatable(no, hwp.deepcopy(getmetatable(o), seen))
    else -- number, string, boolean, etc
        no = o
    end
    return no
end

function hwp.make_addr(addr, section)
    local t = {addr = addr, section = section}
    local addr_to_string = function(self)
        if self.section == nil then
            return string.format("%#x", self.addr)
        else
            return string.format("%#x@%s", self.addr, self.section)
        end
    end
    setmetatable(t, {__tostring = addr_to_string})
    return t
end

function hwp.inc_addr(addr, amount)
    return hwp.make_addr(addr.addr + amount, addr.section)
end

-- pack an array of bytes in a integer (little-endian)
function hwp.pack(arr)
    local v = 0
    for i = #arr, 1, -1 do
        v = bit32.bor(bit32.lshift(v, 8),bit32.band(arr[i], 0xff))
    end
    return v
end

-- do the converse
function hwp.unpack(v, n)
    local t = {}
    for i = 1, n do
        t[i] = bit32.band(v, 0xff)
        v = bit32.rshift(v, 8)
    end
    return t
end

-- read a 32-bit value
function hwp.read32(obj, addr)
    return hwp.pack(hwp.read(obj, addr, 4))
end

-- write a 32-bit value
function hwp.write32(obj, addr, v)
    return hwp.write(obj, addr, hwp.unpack(v, 4))
end

-- convert a MD5 hash to a string
function hwp.md5str(md5)
    local s = ""
    for i = 1, #md5 do
        s = s .. string.format("%02x", md5[i])
    end
    return s
end

-- compute the CRC of a firmware part
function hwp.crc(crc_type, fw, addr, len)
    return hwp.crc_buf(crc_type, hwp.read(fw, addr, len))
end
