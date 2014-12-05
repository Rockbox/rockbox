SDLIB = {}
SDLIB.sdc = {}

SDLIB.STS = {
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

SDLIB.RES = {
    ["NRSP"] = 0,
    ["R1"]   = 1,
    ["R1b"]  = 1,
    ["R2"]   = 2,
    ["R3"]   = 3,
    ["R6"]   = 6
}
-- by convention ACMD command number has bit8 set
SDLIB.CMD = {
    ["GO_IDLE_STATE"]          = {0,      SDLIB.RES.NRSP},
    ["ALL_SEND_CID"]           = {2,      SDLIB.RES.R2},
    ["SEND_RELATIVE_ADDR"]     = {3,      SDLIB.RES.R6},
    ["SET_DSR"]                = {4,      SDLIB.RES.NRSP},
    ["SWITCH_FUNC"]            = {6,      SDLIB.RES.R1},
    ["SET_BUS_WIDTH"]          = {256+6,  SDLIB.RES.R1}, -- acmd6
    ["SELECT_CARD"]            = {7,      SDLIB.RES.R1}, -- with card's rca
    ["DESELECT_CARD"]          = {7,      SDLIB.RES.R1}, -- with rca = 0
    ["SEND_IF_COND"]           = {8,      SDLIB.RES.R6},
    ["SEND_CSD"]               = {9,      SDLIB.RES.R2},
    ["SEND_CID"]               = {10,     SDLIB.RES.R2},
    ["STOP_TRANSMISSION"]      = {12,     SDLIB.RES.R1b},
    ["SEND_STATUS"]            = {13,     SDLIB.RES.R1},
    ["SD_STATUS"]              = {256+13, SDLIB.RES.R1}, -- acmd13
    ["GO_INACTIVE_STATE"]      = {15,     SDLIB.RES.NRSP},
    ["SET_BLOCKLEN"]           = {16,     SDLIB.RES.R1},
    ["READ_SINGLE_BLOCK"]      = {17,     SDLIB.RES.R1},
    ["READ_MULTIPLE_BLOCK"]    = {18,     SDLIB.RES.R1},
    ["SEND_NUM_WR_BLOCKS"]     = {256+22, SDLIB.RES.R1}, -- acmd22
    ["SET_WR_BLK_ERASE_COUNT"] = {256+23, SDLIB.RES.R1}, -- acmd23
    ["WRITE_BLOCK"]            = {24,     SDLIB.RES.R1},
    ["WRITE_MULTIPLE_BLOCK"]   = {25,     SDLIB.RES.R1},
    ["PROGRAM_CSD"]            = {27,     SDLIB.RES.R1},
    ["ERASE_WR_BLK_START"]     = {32,     SDLIB.RES.R1},
    ["ERASE_WR_BLK_END"]       = {33,     SDLIB.RES.R1},
    ["ERASE"]                  = {38,     SDLIB.RES.R1b},
    ["APP_OP_COND"]            = {256+41, SDLIB.RES.R3}, -- acmd41
    ["LOCK_UNLOCK"]            = {42,     SDLIB.RES.R1},
    ["SET_CLR_CARD_DETECT"]    = {256+42, SDLIB.RES.R1}, -- acmd42
    ["SEND_SCR"]               = {256+51, SDLIB.RES.R1}, -- acmd51
    ["APP_CMD"]                = {55,     SDLIB.RES.R1}
}

SDLIB.card_info = {
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

    HWLIB.printf("sd_parse_csd: CSD%d.0, numblocks: %d, speed: %d\n",
                 csd_version+1, info.numblocks, info.speed)
    HWLIB.printf("sd_parse_csd: nsac: %d, taac: %d, r2w: %d\n",
                 info.nsac, info.taac, info.r2w_factor)
end

local function fat_mbr(buf)
    if buf[511] ~= 0x55 or buf[512] ~= 0xaa then
        error("MBR wrong magic bytes")
    end

    for i=0, 3 do
        local offset = 446 + i*16
        HWLIB.printf("\nPartition %d\n", i)
        HWLIB.printf("Bootflag: 0x%02x\n", buf[offset + 1])
        HWLIB.printf("Type: 0x%02x\n", buf[offset + 5])
        HWLIB.printf("LBA: 0x%02x%02x%02x%02x\n",
                     buf[offset + 12],buf[offset + 11],
                     buf[offset + 10],buf[offset + 9])
        HWLIB.printf("Number of sectors: 0x%02x%02x%02x%02x\n",
                     buf[offset + 16],buf[offset + 15],
                     buf[offset + 14],buf[offset + 13])
    end
    HWLIB.printf("\n")
end

-- LIBRARY functions --

function SDLIB.wait_for_state(state)
    local rspdat = {["response"] = {}, ["data"] = {}}
    local retry = 50

    while (retry > 0) do
        retry = retry - 1
        SDLIB.sdc.send_cmd(SDLIB.CMD.SEND_STATUS, SDLIB.card_info.rca, rspdat, 0)

        if (bit32.band(bit32.rshift(rspdat.response[1], 9), 0xf) == state) then
            return 0
        end

        hwstub.mdelay(100)
    end

    return -1
end

function SDLIB.card_init()
    local sd_v2 = false
    local arg = nil
    local rspdat = { ["response"] = {}, ["data"] = {} }

    -- bomb out if the card is not present
    if SDLIB.sdc.card_present() == false then
        error("SD card not inserted\n")
    end

    -- init at max 400kHz
    SDLIB.sdc.set_speed(400000)

    SDLIB.sdc.send_cmd(SDLIB.CMD.GO_IDLE_STATE, 0, rspdat, 0)

    -- CMD8 Check for v2 sd card. Must be sent before using ACMD41
    -- Non v2 cards will not respond to this command
    -- bit [7:1] are crc, bit0 is 1
    SDLIB.sdc.send_cmd(SDLIB.CMD.SEND_IF_COND, 0x1AA, rspdat, 0)
    if (bit32.band(rspdat.response[1], 0xfff) == 0x1aa) then
        sd_v2 = true
        HWLIB.printf("sd_v2 = true\n")
    end

    local arg = nil
    if sd_v2 then
        arg = 0x40FF8000
    else
        arg = 0x00FF8000
    end

    -- 2s init timeout
    while (bit32.band(SDLIB.card_info.ocr, 0x80000000) == 0) do
        -- hwstub.mdelay(100)
        -- ACMD41 For v2 cards set HCS bit[30] & send host voltage range to all

        SDLIB.sdc.send_cmd(SDLIB.CMD.APP_OP_COND, arg, rspdat, 0)
        SDLIB.card_info.ocr = rspdat.response[1]
    end

    SDLIB.sdc.send_cmd(SDLIB.CMD.ALL_SEND_CID, 0, rspdat, 0)
    SDLIB.card_info.cid[1] = rspdat.response[1]
    SDLIB.card_info.cid[2] = rspdat.response[2]
    SDLIB.card_info.cid[3] = rspdat.response[3]
    SDLIB.card_info.cid[4] = rspdat.response[4]

    SDLIB.sdc.send_cmd(SDLIB.CMD.SEND_RELATIVE_ADDR, 0, rspdat, 0)
    SDLIB.card_info.rca = rspdat.response[1]

    -- End of Card Identification Mode

    SDLIB.sdc.send_cmd(SDLIB.CMD.SEND_CSD, SDLIB.card_info.rca, rspdat, 0)
    SDLIB.card_info.csd[1] = rspdat.response[1]
    SDLIB.card_info.csd[2] = rspdat.response[2]
    SDLIB.card_info.csd[3] = rspdat.response[3]
    SDLIB.card_info.csd[4] = rspdat.response[4]
    sd_parse_csd(SDLIB.card_info)

    SDLIB.sdc.send_cmd(SDLIB.CMD.SELECT_CARD, SDLIB.card_info.rca, rspdat, 0)

    -- wait for tran state
    if (SDLIB.wait_for_state(SDLIB.STS.TRAN) ~= 0) then
        error("Error waiting for TRAN")
    end

    -- switch card to 4bit interface
    if (SDLIB.sdc.bus_width == 4) then
        SDLIB.sdc.send_cmd(SDLIB.CMD.SET_BUS_WIDTH, 2, rspdat, 0)
        SDLIB.sdc.set_bus_width(4)
    end

    -- disconnect the pull-up resistor on CD/DAT3
    SDLIB.sdc.send_cmd(SDLIB.CMD.SET_CLR_CARD_DETECT, 0, rspdat, 0)
    -- try switching to HS timing
    -- non-HS cards seems to ignore this
    -- the command returns 64bytes of data
    SDLIB.sdc.send_cmd(SDLIB.CMD.SWITCH_FUNC, 0x80fffff1, rspdat, 64)

    -- rise SD clock
    SDLIB.sdc.set_speed(SDLIB.card_info.speed)

    -- WARNING!!!
    -- HERE ARE A FEW TESTS WHICH ARE NOT SD INIT RELATED
    HWLIB.printf("read sectors\n")

    -- read MBR
    local dat = { }
    SDLIB.read_sectors(0, 1, dat)

    fat_mbr(dat)

    -- sector 0x87 is beginning of 1st partition on my SD
    -- so to test readout of multiple sectors we transfer
    -- 2 and dump the last one which should contain some
    -- data
    SDLIB.read_sectors(0x86, 2, dat)

    for i=513, 1024 do
        HWLIB.printf("0x%02x ", dat[i])
    end
    HWLIB.printf("\n")
end

function SDLIB.read_sectors(start, count, buf)
    local rspdat = { ["response"] = {}, ["data"] = buf }

    if (count <= 0) or (start + count > SDLIB.card_info.numblocks) then
        error("SD: Out of bound read resquest")
    end

    if(bit32.band(SDLIB.card_info.ocr, 0x40000000) == 0) then
        start = start * 512 -- not SDHC
    end

    SDLIB.sdc.send_cmd(SDLIB.CMD.READ_MULTIPLE_BLOCK, start, rspdat, count*512)
    SDLIB.sdc.send_cmd(SDLIB.CMD.STOP_TRANSMISSION, 0, rspdat, 0)
end

function SDLIB.register_driver(driver_info)
    SDLIB.sdc.init          = driver_info.init
    SDLIB.sdc.set_speed     = driver_info.set_speed
    SDLIB.sdc.set_bus_width = driver_info.set_bus_width
    SDLIB.sdc.card_present  = driver_info.card_present
    SDLIB.sdc.send_cmd      = driver_info.send_cmd
    SDLIB.sdc.bus_width     = driver_info.bus_width
end
