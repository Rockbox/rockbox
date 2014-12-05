ATJ.sd = {}

local STS = {
    ["IDLE"]  = 0,
    ["READY"] = 1,
    ["IDENT"] = 2,
    ["STBY"]  = 3,
    ["TRAN"]  = 4,
    ["DATA"]  = 5,
    ["RCV"]   = 6,
    ["PRG"]   = 7,
    ["DIS"]   = 8
}

local RES = {
    ["NRSP"] = 0,
    ["R1"]   = 1,
    ["R1b"]  = 1,
    ["R2"]   = 2,
    ["R3"]   = 3,
    ["R6"]   = 6
}

--[[ by convention ACMD command number has bit8 set
CMD = {
    ["GO_IDLE_STATE"]          = {0,      RSP.NRSP},
    ["ALL_SEND_CID"]           = {2,      RSP.R2},
    ["SEND_RELATIVE_ADDR"]     = {3,      RSP.R6},
    ["SET_DSR"]                = {4,      RSP.NRSP},
    ["SWITCH_FUNC"]            = {6,      RSP.R1},
    ["SET_BUS_WIDTH"]          = {256+6,  RSP.R1}, -- acmd6
    ["SELECT_CARD"]            = {7,      RSP.R1}, -- with card's rca
    ["DESELECT_CARD"]          = {7,      RSP.R1}, -- with rca = 0
    ["SEND_IF_COND"]           = {8,      RSP.R6},
    ["SEND_CSD"]               = {9,      RSP.R2},
    ["SEND_CID"]               = {10,     RSP.R2},
    ["STOP_TRANSMISSION"]      = {12,     RSP.R1b},
    ["SEND_STATUS"]            = {13,     RSP.R1},
    ["SD_STATUS"]              = {256+13, RSP.R1}, -- acmd13
    ["GO_INACTIVE_STATE"]      = {15,     RSP.NRSP},
    ["SET_BLOCKLEN"]           = {16,     RSP.R1},
    ["READ_SINGLE_BLOCK"]      = {17,     RSP.R1},
    ["READ_MULTIPLE_BLOCK"]    = {18,     RSP.R1},
    ["SEND_NUM_WR_BLOCKS"]     = {256+22, RSP.R1}, -- acmd22
    ["SET_WR_BLK_ERASE_COUNT"] = {256+23, RSP.R1}, -- acmd23
    ["WRITE_BLOCK"]            = {24,     RSP.R1},
    ["WRITE_MULTIPLE_BLOCK"]   = {25,     RSP.R1},
    ["PROGRAM_CSD"]            = {27,     RSP.R1},
    ["ERASE_WR_BLK_START"]     = {32,     RSP.R1},
    ["ERASE_WR_BLK_END"]       = {33,     RSP.R1},
    ["ERASE"]                  = {38,     RSP.R1b},
    ["APP_OP_COND"]            = {256+41, RSP.R3}, -- acmd41
    ["LOCK_UNLOCK"]            = {42,     RSP.R1},
    ["SET_CLR_CARD_DETECT"]    = {256+42, RSP.R1}, -- acmd42
    ["SEND_SCR"]               = {256+51, RSP.R1}, -- acmd51
    ["APP_CMD"]                = {55,     RSP.R1}
}
]]--

local CMD = {
    ["GO_IDLE_STATE"]          = 0,
    ["ALL_SEND_CID"]           = 2,
    ["SEND_RELATIVE_ADDR"]     = 3,
    ["SET_DSR"]                = 4,
    ["SWITCH_FUNC"]            = 6,
    ["SET_BUS_WIDTH"]          = 6, -- acmd6
    ["SELECT_CARD"]            = 7, -- with card's rca
    ["DESELECT_CARD"]          = 7, -- with rca = 0
    ["SEND_IF_COND"]           = 8,
    ["SEND_CSD"]               = 9,
    ["SEND_CID"]               = 10,
    ["STOP_TRANSMISSION"]      = 12,
    ["SEND_STATUS"]            = 13,
    ["SD_STATUS"]              = 13, -- acmd13
    ["GO_INACTIVE_STATE"]      = 15,
    ["SET_BLOCKLEN"]           = 16,
    ["READ_SINGLE_BLOCK"]      = 17,
    ["READ_MULTIPLE_BLOCK"]    = 18,
    ["SEND_NUM_WR_BLOCKS"]     = 22, -- acmd22
    ["SET_WR_BLK_ERASE_COUNT"] = 23, -- acmd23
    ["WRITE_BLOCK"]            = 24,
    ["WRITE_MULTIPLE_BLOCK"]   = 25,
    ["PROGRAM_CSD"]            = 27,
    ["ERASE_WR_BLK_START"]     = 32,
    ["ERASE_WR_BLK_END"]       = 33,
    ["ERASE"]                  = 38,
    ["APP_OP_COND"]            = 41, -- acmd41
    ["LOCK_UNLOCK"]            = 42,
    ["SET_CLR_CARD_DETECT"]    = 42, -- acmd42
    ["SEND_SCR"]               = 51, -- acmd51
    ["APP_CMD"]                = 55
}

local card_info = {
    ["ocr"] = 0,
    ["csd"] = {0, 0, 0, 0},
    ["cid"] = {0, 0, 0, 0},
    ["rca"] = 0,
    ["numblocks"] = 0,
    ["blocksize"] = 0,
    ["speed"] = 0,
    ["nsac"] = 0,
    ["taac"] = 0,
    ["r2w_factor"] = 0
}

local function card_extract_bits(p, start, size)
    local long_index = nil
    local bit_index = nil
    local result = nil

    start = 127 - start

    long_index = math.floor(start / 32) + 1
    bit_index = start % 32

    result = bit32.lshift(p[long_index], bit_index)

    if (bit_index + size > 32) then
        result = bit32.bor(result,bit32.rshift(p[long_index+1],(32-bit_index)))
    end

    result = bit32.rshift(result, (32 - size))

    return result
end

local function sd_parse_csd(info)
    local sd_mantissa = { 0,  10, 12, 13, 15, 20, 25, 30, 
                          35, 40, 45, 50, 55, 60, 70, 80 }
    local sd_exponent = { 1,10,100,1000,10000,100000,
                          1000000,10000000,100000000,1000000000 }

    local c_size = nil
    local c_mult = nil
    local max_read_bl_len = nil
    local csd_version = card_extract_bits(info.csd, 127, 2)

    if (csd_version == 0) then
        -- CSD version 1.0
        c_size = card_extract_bits(info.csd, 73, 12) + 1
        c_mult = bit32.lshift(4, card_extract_bits(info.csd, 49, 3))
        max_read_bl_len = bit32.lshift(1, card_extract_bits(info.csd, 83, 4))
        info.numblocks = c_size * c_mult * (max_read_bl_len/512)
    elseif (csd_version == 1) then
        -- CSD version 2.0
        c_size = card_extract_bits(info.csd, 69, 22) + 1
        info.numblocks = bit32.lshift(c_size, 10)
    else error("Unknown csd version " .. csd_version)
    end

    info.blocksize = 512 -- always use 512 blocks
    info.speed = sd_mantissa[card_extract_bits(info.csd, 102, 4) + 1] *
                 sd_exponent[card_extract_bits(info.csd,  98, 3) + 4 + 1]
    info.nsac = 100 * card_extract_bits(info.csd, 111, 8)
    info.taac = sd_mantissa[card_extract_bits(info.csd, 118, 4) + 1] *
                sd_exponent[card_extract_bits(info.csd, 114, 3) + 1]
    info.r2w_factor = card_extract_bits(info.csd, 28, 3)

    HWLIB.printf("CSD%d.0 numblocks:%d speed:%d\n",
                 csd_version+1, info.numblocks, info.speed)
    HWLIB.printf("nsac: %d taac: %d r2w: %d\n",
                 info.nsac, info.taac, info.r2w_factor)
end

local function fat_mbr(buf)
    if buf[511] ~= 0x55 or buf[512] ~= 0xaa then
        error("MBR wrong magic bytes")
    end

    for i=0, 3 do
        local offset = 446 + i*16
        HWLIB.printf("Partition %d\n", i)
        HWLIB.printf("Bootflag: 0x%02x\n", buf[offset + 1])
        HWLIB.printf("Type: 0x%02x\n", buf[offset + 5])
        HWLIB.printf("LBA: 0x%02x%02x%02x%02x\n",
                     buf[offset + 12],buf[offset + 11],
                     buf[offset + 10],buf[offset + 9])
        HWLIB.printf("Number of sectors: 0x%02x%02x%02x%02x\n\n",
                     buf[offset + 16],buf[offset + 15],
                     buf[offset + 14],buf[offset + 13])
    end
end

-- mips specific
local function hwaddr(addr)
    return bit32.band(addr, 0x1fffffff)
end

local function uncachedaddr(addr)
    return bit32.bor(0xa0000000)
end

-- clk in HZ
function ATJ.sd.set_clock(clk)
    local corepllfreq = bit32.band(HW.CMU.COREPLL.read(), 0x1f)
    corepllfreq = corepllfreq * 6000000

    -- we need to use proper rounding
    local sddiv = (corepllfreq + clk - 1)/clk
    sddiv = math.floor(sddiv - 1)

    if sddiv > 15 then
        sddiv = (corepllfreq/128 + clk - 1)/clk
        sddiv = math.floor(sddiv - 1)

        -- bit4 is DIV128
        -- needed for low clock speed during initialization
        sddiv = bit32.bor(sddiv, 0x10)
    end

    -- bit5 is clock enable
    HW.CMU.SDCLK.write(bit32.bor(sddiv, 0x20))
end

function ATJ.sd.get_clock()
    local sddiv = HW.CMU.SDCLK.SDDIV.read() + 1
    if HW.CMU.SDCLK.D128.read() == 1 then
        sddiv = 128 * sddiv
    end

    local corepllfreq = HW.CMU.COREPLL.CPCK.read() * 6000000

    return math.floor(corepllfreq/sddiv)
end

function ATJ.sd.send_cmd(cmd, arg, restype, response)
    HWLIB.printf("send_cmd(CMD%d, 0x%0x, %d)\n", cmd, arg, restype)
    HW.SD.ARG.write(arg)
    HW.SD.CMD.write(cmd)

    -- this sets bit0 to clear STAT
    -- and mark response type
    local cmdrsp = nil
    if restype == RES.NRSP then                 -- NRSP
        cmdrsp = 0x05
    elseif restype == RES.R1 or restype == RES.R6 then -- RSP1 or RSP6
        cmdrsp = 0x03
    elseif restype == RES.R2 then             -- RSP2
        cmdrsp = 0x11
    elseif restype == RES.R3 then             -- RSP3
        cmdrsp = 0x09
    else error("Invalid response number " .. restype)
    end

    HW.SD.CMDRSP.write(cmdrsp)

    -- command finish wait
    -- should have proper timeout here
    local tmp = HW.SD.CMDRSP.read()
    local tmo = 10
    while (bit32.band(tmp, cmdrsp) ~= 0) do
        -- HWLIB.printf("CMDRSP: 0x%08x", tmp))
        tmp = HW.SD.CMDRSP.read()
        tmo = tmo - 1
        if tmo < 0 then
            HWLIB.printf("send_cmd(CMD%d, 0x%0x, %d) timeout\n",cmd,arg,restype)
            error("send_cmd failed")
            return -1
        end
    end

    -- check crc
    if (restype ~= RES.NRSP) then
        if (restype == RES.R1) then
            local crc7 = HW.SD.CRC7.read()
            local rescrc = bit32.band(HW.SD.RSPBUF0.read(), 0xff)
            if (bit32.bxor(crc7, rescrc) ~=0) then
                HWLIB.printf("CRC: 0x%02x != 0x%02x\n", rescrc, crc7)
                error("CRC error")
                return -2
            end
        end
        
        -- copy response with striped crc + end bit
        if (restype == RES.R2) then
             response[1] = HW.SD.RSPBUF3.read()
             response[2] = HW.SD.RSPBUF2.read()
             response[3] = HW.SD.RSPBUF1.read()
             response[4] = HW.SD.RSPBUF0.read()
            HWLIB.printf("RSP: 0x%08x 0x%08x 0x%08x 0x%08x\n",
                         response[1], response[2], response[3], response[4])
        else
            response[1] = bit32.bor(bit32.lshift(HW.SD.RSPBUF1.read(), 24),
                                    bit32.rshift(HW.SD.RSPBUF0.read(), 8))
            HWLIB.printf("RSP: 0x%08x\n", response[1])
        end
    end

    return 0
end

function ATJ.sd.wait_for_state(state)
    local rsp = {0}
    local retry = 50

    while (retry > 0) do
        retry = retry - 1
        ATJ.sd.send_cmd(CMD.SEND_STATUS, card_info.rca, RES.R1, rsp)

        if (bit32.band(bit32.rshift(rsp[1], 9), 0xf) == state) then
            return 0
        end

        hwstub.mdelay(100)
    end

    return -1
end

function ATJ.sd.init()
    HW.CMU.DEVCLKEN.SD.write(1) -- bit11 SD clock
    HW.SD.CTL.write(0x4c1)
    HW.SD.FIFOCTL.write(0x25c)  -- reset fifo
    HW.SD.BYTECNT.write(0)
    HW.SD.RW.write(0x340)       -- RCST , WCEF, WCST
    HW.SD.CMDRSP.write(0x10000) -- reserved bit

    HW.SD.CTL.BSEL.write(0) -- 0 for DMA channels 0-3; 1 for DMA channels 4-7

end

function ATJ.sd.send_clk(cnt)
    local tmp = nil
    while (cnt > 0) do
        HW.SD.CLK.write(0xff)
        tmp = HW.SD.CLK.read()
        while (bit32.band(tmp, 1) ~= 0) do
            print(string.format("SD_CLK: 0x%08x", tmp))
            tmp = HW.SD.CLK.read()
        end
        cnt = cnt - 1
    end
end

function ATJ.sd.card_init()
    local sd_v2 = false
    local arg = nil
    local rsp = { 0 }

    -- init at max 400kHz
    ATJ.sd.set_clock(400000)

    ATJ.sd.send_clk(5)
    ATJ.sd.send_cmd(CMD.GO_IDLE_STATE, 0, RES.NRSP)
    ATJ.sd.send_clk(5)

    -- CMD8 Check for v2 sd card. Must be sent before using ACMD41
    -- Non v2 cards will not respond to this command
    -- bit [7:1] are crc, bit0 is 1
    ATJ.sd.send_cmd(CMD.SEND_IF_COND, 0x1AA, RES.R6, rsp)
    if (bit32.band(rsp[1], 0xfff) == 0x1aa) then
        sd_v2 = true
        HWLIB.printf("sd_v2 = true\n")
    end

    -- 2s init timeout
    while (bit32.band(card_info.ocr, 0x80000000) == 0) do
        -- hwstub.mdelay(100)
        ATJ.sd.send_cmd(CMD.APP_CMD, card_info.rca, RES.R1, rsp)
        -- ACMD41 For v2 cards set HCS bit[30] & send host voltage range to all
        if sd_v2 then
            arg = 0x40FF8000
        else
            arg = 0x00FF8000
        end

        ATJ.sd.send_cmd(CMD.APP_OP_COND, arg, RES.R3, rsp)
        card_info.ocr = rsp[1]
    end

    ATJ.sd.send_cmd(CMD.ALL_SEND_CID, 0, RES.R2, card_info.cid)

    ATJ.sd.send_cmd(CMD.SEND_RELATIVE_ADDR, 0, RES.R6, rsp)
    card_info.rca = rsp[1]

    -- End of Card Identification Mode

    ATJ.sd.send_cmd(CMD.SEND_CSD, card_info.rca, RES.R2, card_info.csd)
    sd_parse_csd(card_info)

    ATJ.sd.send_cmd(CMD.SELECT_CARD, card_info.rca, RES.R1b, rsp)

    -- wait for tran state
    if (ATJ.sd.wait_for_state(STS.TRAN) ~= 0) then
        error("Error waiting for TRAN")
    end

    -- switch card to 4bit interface
    ATJ.sd.send_cmd(CMD.APP_CMD, card_info.rca, RES.R1, rsp)
    ATJ.sd.send_cmd(CMD.SET_BUS_WIDTH, 2, RES.R1, rsp)

    -- disconnect the pull-up resistor on CD/DAT3
    ATJ.sd.send_cmd(CMD.APP_CMD, card_info.rca, RES.R1, rsp)
    ATJ.sd.send_cmd(CMD.SET_CLR_CARD_DETECT, 0, RES.R1, rsp)

    -- try switching to HS timing
    -- non-HS cards seems to ignore this
    ATJ.sd.send_cmd(CMD.SWITCH_FUNC, 0x80fffff1, RES.R1, rsp)
    -- get 64bytes from the card
    -- DMA buf in iram
    -- I shrinked buffer by 1kB in linker script
    -- and use that space as safe DMA buf
    local dma_buf_addr = uncachedaddr(hwstub.dev.layout.buffer.start +
                                      hwstub.dev.layout.buffer.size)
  
    ATJ.sd.data_rd(dma_buf_addr, 64)

    -- rise SD clock
    ATJ.sd.set_clock(25000000)

    -- WARNING!!!
    -- HERE ARE A FEW TESTS WHICH ARE NOT SD INIT RELATED
    HWLIB.printf("read sectors\n")

    -- read MBR
    ATJ.sd.read_sectors(0, 1, dma_buf_addr)

    local dat = { }
    for i=0, 511 do
        dat[i+1] = DEV.read8(dma_buf_addr + i)
    end

    fat_mbr(dat)

    -- sector 0x87 is beginning of 1st partition on my SD
    -- so to test readout of multiple sectors we transfer
    -- 8 and dump the last one which should contain some
    -- data
    ATJ.sd.read_sectors(0x80, 8, dma_buf_addr)

    for i=0, 511 do
        HWLIB.printf("0x%02x ", DEV.read8(dma_buf_addr + i))
    end
    -- CMD23 support ???
end

function ATJ.sd.data_rd(buf, size)
    HW.CMU.DEVCLKEN.DMAC.write(1)        -- bit8 DMAC clock gate

    HW.DMAC.DMA0_MODE.write(0x008c01b4)  -- aligned iram dst, sd module src
    HW.DMAC.DMA0_SRC.write(0x100b0030)   -- hw_addr of SD_DAT
    HW.DMAC.DMA0_DST.write(hwaddr(buf))  -- hw_addr of DMA buf
    HW.DMAC.DMA0_CNT.write(size)         -- count

    HW.SD.BYTECNT.write(size)
    HW.SD.FIFOCTL.write(0x259)
    HW.SD.RW.write(0x3c0)

    HW.DMAC.DMA0_CMD.write(1) -- DMA kick in

    -- wait for DMA transfer finish
    while (bit32.band(HW.DMAC.DMA0_CMD.read(), 1) == 1) do end
end

function ATJ.sd.read_sectors(start, count, dma_buf_addr)
    local rsp = { 0 }

    if (count <= 0) or (start + count > card_info.numblocks) then
        error("SD: Out of bound read resquest")
    end

    if(bit32.band(card_info.ocr, 0x40000000) == 0) then
        start = start * 512 -- not SDHC
    end

    ATJ.sd.send_cmd(CMD.READ_MULTIPLE_BLOCK, start, RES.R1, rsp)

    while (count > 0) do
        -- transfer data
        ATJ.sd.data_rd(dma_buf_addr, 512)
        count = count - 1
    end

    ATJ.sd.send_cmd(CMD.STOP_TRANSMISSION, 0, RES.R1b, rsp)
end



-- shortcut used in testing
function go()
    ATJ.gpio.muxsel("SD")
    ATJ.sd.init()
    ATJ.sd.card_init()
end
