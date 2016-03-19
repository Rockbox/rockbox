---
--- GPIO
--- 
JZ.nand = {}
JZ.nand.rom = {}

function JZ.nand.init_pins(buswidth)
    -- PA[21,19,18]: cs1, fre, fwe
    JZ.gpio.pinmask(0, 0x2c0000).std_function(0)
    JZ.gpio.pinmask(0, 0x2c0000).pull(false)
    if buswidth == 16 then
        -- PA[15:0]: d{15-0}
        JZ.gpio.pinmask(0, 0xffff).std_function(0)
    else
        -- PA[7:0]: d{7-0}
        JZ.gpio.pinmask(0, 0xff).std_function(0)
    end
    -- PB[1:0]: ale, cle
    JZ.gpio.pinmask(1, 3).std_function(0)
    JZ.gpio.pinmask(1, 3).pull(false)
    -- PA20: rb#
    JZ.gpio.pin(0, 20).gpio_in()
end

function JZ.nand.send_cmd(cmd)
    DEV.write8(0xba400000, cmd)
end

function JZ.nand.send_addr(addr)
    DEV.write8(0xba800000, addr)
end

function JZ.nand.read_data8()
    return DEV.read8(0xba000000)
end

function JZ.nand.read_data32()
    -- Boot ROM cannot do 16-bit read/write (those end up being 2x8-bit)
    return DEV.read32(0xba000000)
end

function JZ.nand.wait_ready()
    local pin = JZ.gpio.pin(0, 20)
    -- wait ready
    while pin.read() == 0 do end
end

function JZ.nand.set_buswidth(buswidth)
    if buswidth == 8 then
        HW.NEMC.SMC[1].BW.write("8BIT")
    elseif buswidth == 16 then
        HW.NEMC.SMC[1].BW.write("16BIT")
    else
        error("invalid buswidth")
    end
end
-- {row,col}cycle must be 2 or 3
-- buswidth must be 8 or 16
-- count is the number of bytes to read (must be multiple of 2 is buswidth is 16)
-- function returns a table of bytes
function JZ.nand.read_page(col,colcycle,row,rowcycle,buswidth,count)
    JZ.nand.set_buswidth(buswidth)
    -- read page first cycle
    JZ.nand.send_cmd(0)
    -- column
    JZ.nand.send_addr(col)
    JZ.nand.send_addr(bit32.rshift(col, 8))
    if colcycle == 3 then
        JZ.nand.send_addr(bit32.rshift(col, 16))
    end
    -- row
    JZ.nand.send_addr(row)
    JZ.nand.send_addr(bit32.rshift(row, 8))
    if rowcycle == 3 then
        JZ.nand.send_addr(bit32.rshift(row, 16))
    end
    -- read page second cycle
    JZ.nand.send_cmd(0x30)
    -- wait ready
    JZ.nand.wait_ready()
    -- read
    return JZ.nand.read_page_data(buswidth,count)
end

function JZ.nand.read_page2(params,col,row,count)
    return JZ.nand.read_page(col,params.col_cycle,row,params.row_cycle,params.bus_width,count)
end

-- read data, assuming read page command was sent
function JZ.nand.read_page_data(buswidth,count)
    -- read
    data = {}
    if buswidth == 8 then
        for i=0, count-1 do
            data[i] = JZ.nand.read_data8()
        end
    else
        for i=0, count-1, 4 do
            local hw = JZ.nand.read_data32()
            data[i] = bit32.band(hw, 0xff)
            data[i + 1] = bit32.band(bit32.rshift(hw, 8), 0xff)
            data[i + 2] = bit32.band(bit32.rshift(hw, 16), 0xff)
            data[i + 3] = bit32.band(bit32.rshift(hw, 24), 0xff)
        end
    end
    return data
end

function JZ.nand.read_oob(params,page)
--[[
NAND flash are magic, every setup is different, so basically:
- if the page size is 512, that's a special case and we need to use a special
    command to read OOB, also note that 512-byte NAND only have one cycle column address
- otherwise, assume OOB is after the data (so at offset page_size in bytes),
    but beware that for 16-bit bus, column address is in words, not bytes
For simplicity, we do not support 512-byte NAND at the moment
]]
    -- compute column address of OOB data
    local col_addr = params.page_size
    if params.bus_width == 16 then
        col_addr = params.page_size / 2
    end
    -- read page
    return JZ.nand.read_page2(params,col_addr,page,params.oob_size)
end

--[[
setup NAND parameters, the table should contain:
- bus_width: 8 or 16,
- addr_setup_time: in cycles
- addr_hold_time: in cycles
- write_strobe_time: in cycles
- read_strobe_time: in cycles
- recovery_time: in cycles
]]
function JZ.nand.setup(params)
    JZ.nand.init_pins(params.bus_width)
    HW.NEMC.SMC[1].BL.write(params.bus_width == 8 and "8" or "16")
    HW.NEMC.SMC[1].TAS.write(params.addr_setup_time)
    HW.NEMC.SMC[1].TAH.write(params.addr_hold_time)
    HW.NEMC.SMC[1].TBP.write(params.write_strobe_time)
    HW.NEMC.SMC[1].TAW.write(params.read_strobe_time)
    HW.NEMC.SMC[1].STRV.write(params.recovery_time)
end

function JZ.nand.reset()
    print("NAND: reset")
    JZ.nand.send_cmd(0xff)
    JZ.nand.wait_ready()
end

-- init nand like ROM
function JZ.nand.rom.init()
    -- init pins to 16-bit in doubt
    JZ.nand.init_pins(16)
    -- take safest setting: 8-bit, max TAS, max TAH, max read/write strobe wait
    HW.NEMC.SMC[1].write(0xfff7700)
    -- enable flash on CS1 with CS# always asserted
    HW.NEMC.NFC.write(3)
    -- reset
    JZ.nand.reset()
end

function JZ.nand.rom.parse_flag(data, offset)
    local cnt_55 = 0
    local cnt_aa = 0
    for i = offset,offset + 31 do
        if data[i] == 0x55 then
            cnt_55 = cnt_55 + 1
        elseif data[i] == 0xaa then
            cnt_aa = cnt_aa + 1
        end
    end
    if cnt_55 >= 7 then
        return 0x55
    elseif cnt_aa >= 7 then
        return 0xaa
    else
        return 0xff
    end
end

function JZ.nand.rom.read_page(col,row,count)
    return JZ.nand.read_page(col, JZ.nand.rom.colcycle, row, JZ.nand.rom.rowcycle,
        JZ.nand.rom.buswidth, count)
end

function JZ.nand.rom.read_page_data(count)
    return JZ.nand.read_page_data(JZ.nand.rom.buswidth, count)
end

-- read flash parameters
function JZ.nand.rom.read_flags()
    local flags = nil
    -- read first page
    for colcycle = 2,3 do
        flags = JZ.nand.read_page(0,colcycle,0,3,8,160)
        local buswidth_flag = JZ.nand.rom.parse_flag(flags, 0)
        if buswidth_flag == 0x55 then
            -- set to 8-bit
            JZ.nand.rom.colcycle = colcycle
            JZ.nand.rom.buswidth = 8
            break
        elseif buswidth_flag == 0xaa then
            -- set to 16-bit
            JZ.nand.rom.colcycle = colcycle
            JZ.nand.rom.buswidth = 16
            break
        end
    end
    if JZ.nand.rom.buswidth == nil then
        error("Cannot read flags")
    end
    print("NAND: colcycle = " .. JZ.nand.rom.colcycle)
    print("NAND: buswidth = " .. JZ.nand.rom.buswidth)
    -- reread flags correctly now
    flags = JZ.nand.read_page(0,JZ.nand.rom.colcycle,0,3,JZ.nand.rom.buswidth,160)
    -- rowcycle
    local rowcycle_flag = JZ.nand.rom.parse_flag(flags, 64)
    if rowcycle_flag == 0x55 then
        JZ.nand.rom.rowcycle = 2
    elseif rowcycle_flag == 0xaa then
        JZ.nand.rom.rowcycle = 3
    else
        error("invalid rowcycle flag")
    end
    print("NAND: rowcycle = " .. JZ.nand.rom.rowcycle)
    -- pagesize
    local pagesize1_flag = JZ.nand.rom.parse_flag(flags, 96)
    local pagesize0_flag = JZ.nand.rom.parse_flag(flags, 128)
    if pagesize1_flag == 0x55 and pagesize0_flag == 0x55 then
        JZ.nand.rom.pagesize = 512
    elseif pagesize1_flag == 0x55 and pagesize0_flag == 0xaa then
        JZ.nand.rom.pagesize = 2048
    elseif pagesize1_flag == 0xaa and pagesize0_flag == 0x55 then
        JZ.nand.rom.pagesize = 4096
    elseif pagesize1_flag == 0xaa and pagesize0_flag == 0xaa then
        JZ.nand.rom.pagesize = 8192
    else
        error(string.format("invalid pagesize flag: %#x,%#x", pagesize1_flag, pagesize0_flag))
    end
    print("NAND: pagesize = " .. JZ.nand.rom.pagesize)
end

-- read bootloader
function JZ.nand.rom.read_bootloader()
    -- computer number of blocks per page
    local bl_size = 256
    local bl_per_page = JZ.nand.rom.pagesize / bl_size
    local ecc_per_bl = 39
    local bootloader_size = 8 * 1024
    local bootloader = {}

    local page_offset = 0
    while true do
        local all_ok = true
        print("NAND: try at page offset " .. page_offset)
        for page = 0, bootloader_size / JZ.nand.rom.pagesize - 1 do
            print("NAND: page " .. page)
            -- enable randomizer
            HW.NEMC.PNC.write(3)
            -- read ECC
            local ecc = JZ.nand.rom.read_page(0, page_offset + 2 * page + 1, ecc_per_bl * bl_per_page)
            -- disable randomizer
            HW.NEMC.PNC.write(0)
            HW.NEMC.NFC.write(0)
            HW.NEMC.NFC.write(3)
            -- send read page commannd, but don't read the data just yet
            JZ.nand.rom.read_page(0, page_offset + 2 * page, 0)
            -- for each block
            for bl = 0, bl_per_page - 1 do
                print("NAND: block " .. bl)
                -- enable randomizer (except for first block of first page)
                if page ~=0 or bl ~= 0 then
                    HW.NEMC.PNC.write(3)
                end
                -- read data
                local data = JZ.nand.rom.read_page_data(bl_size)
                -- disable randomizer
                HW.NEMC.PNC.write(0)
                -- setup bch
                HW.BCH.INTS.write(0xffffffff)
                HW.BCH.CTRL.SET.write(0x2b)
                HW.BCH.CTRL.CLR.write(0x4)
                HW.BCH.COUNT.DEC.write((bl_size + ecc_per_bl) * 2)
                for i = 0, bl_size - 1 do
                    HW.BCH.DATA.write(data[i])
                end
                for i = 0, ecc_per_bl - 1 do
                    HW.BCH.DATA.write(ecc[bl * ecc_per_bl + i])
                end
                while HW.BCH.INTS.DECF.read() == 0 do
                end
                HW.BCH.CTRL.CLR.write(1)
                print(string.format("NAND: ecc = 0x%x", HW.BCH.INTS.read()))
                -- now fix the errors
                if HW.BCH.INTS.UNCOR.read() == 1 then
                    print("NAND: uncorrectable errors !")
                    all_ok = false
                end
                print(string.format("NAND: correcting %d errors", HW.BCH.INTS.ERRC.read()))
                if HW.BCH.INTS.ERRC.read() > 0 then
                    error("Error correction is not implemented for now")
                end
                for i = 0, bl_size - 1 do
                    bootloader[(page * bl_per_page + bl) * bl_size + i] = data[i]
                end
            end
        end
        if all_ok then
            break
        end
        page_offset = page_offset + 16 * 1024 / JZ.nand.rom.pagesize
    end
    return bootloader
end

--[[
read SPL: offset and size in pages, the param table should contain:
- page_size: page size in bytes (exclusing spare)
- oob_size: spare data size in bytes
- page_per_block: number of pages per block
- ecc_pos: offset within spare of the ecc data
- badblock_pos: offset within spare of the badblock marker (only one page per block is marked)
- badblock_page: page number within block of the page containing badblock marker in spare
- col_cycle: number of cycles for column address
- row_cycle: number of cycles for row address
- ecc_size: ECC size in bytes
- ecc_level: level of error correction in bits: 4, 8, 12, ...
]]
function JZ.nand.rom.read_spl(params, offset, size)
--[[
On-flash format: each block contains page_per_block pages,
where each page contains page_size bytes, follows by oob_size spare bytes.
The spare area contains a bad block marker at offset badblock_pos and
the ECC data at offset ecc_pos. Note that only one page within each block
actually contains the bad block marker: this page is badblock_page. The marker
is 0xff is the block is valid. Any invalid block is skipped.
The ECC is computed on a per-512-block basis. Since a page contains several such
blocks, the ECC data contains consecutive ecc blocks, one for each 512-byte data
block.

+---------------------+
|page0|page1|...|pageN|  <--- block
+---------------------+

+-------------------------------+
|data(page_size)|spare(oob_size)|  <--- page
+-------------------------------+

+---------------------------------------------+
|xxxx|badblock marker(1)|xxxxx|ECC data()|xxxx|  <-- spare
+---------------------------------------------+
]]
    local bootloader = {}
    if (offset % params.page_per_block) ~= 0 then
        print("Warning: SPL is not block-aligned")
    end
    -- setup parameters
    JZ.nand.setup(params)
    -- enable NAND
    HW.NEMC.NFC.write(3)
    -- reset
    JZ.nand.reset()
    -- read SPL !
    local checked_block = false
    local cur_page = offset
    local loaded_pages = 0
    while loaded_pages < size do
        ::load_loop::
        -- if we just crossed a page boundary, reset the block check flag
        if (cur_page % params.page_per_block) == 0 then
            checked_block = false
        end
        -- check block for bad block marker if needed
        if not checked_block then
            print("Reading bad block marker for block " .. (cur_page / params.page_per_block) .. "...")
            -- read OOB data
            local oob_data = JZ.nand.read_oob(params,cur_page + params.badblock_page)
            if oob_data[params.badblock_pos] ~= 0xff then
                print("Bad block at " .. (cur_page / page_per_block))
                -- skip block
                cur_page = ((cur_page + params.page_per_block) / params.page_per_block) * params.page_per_block
                -- lua has no continue...
                goto load_loop
            end
            checked_block = true
        end

        print("Reading page " .. cur_page .. "...")
        -- send read page command
        JZ.nand.read_page2(params,0,cur_page, 0)
        local page_data = JZ.nand.read_page_data(params.bus_width,params.page_size)
        local oob_data = JZ.nand.read_page_data(params.bus_width,params.oob_size)
        -- handle each 512-byte block for ECC
        local bl_size = 512
        for bl = 0,params.page_size/bl_size-1 do
            print("Checking subblock " .. bl .. "...")
                -- setup bch
            HW.BCH.INTS.write(0xffffffff)
            HW.BCH.CTRL.CLR.BSEL.write() -- clear level
            HW.BCH.CTRL.SET.BSEL.write(params.ecc_level / 4 - 1) -- set level
            HW.BCH.CTRL.SET.write(3) -- enable and reset
            HW.BCH.CTRL.CLR.ENCE.write(0x4) -- decode
            -- write ecc data count
            HW.BCH.COUNT.DEC.write((bl_size + params.ecc_size) * 2)
            -- write data
            for j = 0, bl_size - 1 do
                HW.BCH.DATA.write(page_data[bl_size * bl + j])
            end
            -- write ecc data
            for j = 0, params.ecc_size - 1 do
                HW.BCH.DATA.write(oob_data[params.ecc_pos + bl * params.ecc_size + j])
            end
            -- wait until bch is done
            while HW.BCH.INTS.DECF.read() == 0 do
            end
            -- disable bch
            HW.BCH.CTRL.CLR.write(1)
            print(string.format("NAND: ecc = 0x%x", HW.BCH.INTS.read()))
            -- now fix the errors
            if HW.BCH.INTS.UNCOR.read() == 1 then
                error("NAND: uncorrectable errors !")
            end
            print(string.format("NAND: correcting %d errors", HW.BCH.INTS.ERRC.read()))
            if HW.BCH.INTS.ERRC.read() > 0 then
                error("Error correction is not implemented for now")
            end
            for i = 0, bl_size - 1 do
                bootloader[loaded_pages * params.page_size + bl * bl_size + i] = page_data[bl_size * bl + i]
            end
        end
        cur_page = cur_page + 1
        loaded_pages = loaded_pages + 1
    end
    -- disable NAND
    HW.NEMC.NFC.write(0)
    return bootloader
end

-- dump data
function JZ.nand.rom.write_to_file(file, data)
    local f = io.open(file, "w")
    if f == nil then error("Cannot open file or write to nil") end
    local i = 0
    while type(data[i]) == "number" do
        f:write(string.char(data[i]))
        i = i + 1
    end
    io.close(f)
end