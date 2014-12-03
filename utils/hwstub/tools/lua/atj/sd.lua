-- TARGET SPECIFIC STUFF --
local sdc = {}

-- mips specific
local function hwaddr(addr)
    return bit32.band(addr, 0x1fffffff)
end

local function uncachedaddr(addr)
    return bit32.bor(addr, 0xa0000000)
end

sdc.bus_width = 4

-- init controler
function sdc.init()
    HW.CMU.DEVCLKEN.SD.write(1)   -- bit11 SD clock
    HW.CMU.DEVCLKEN.DMAC.write(1) -- bit8 DMAC clock
    HW.SD.CTL.write(0x4c1)
    HW.SD.FIFOCTL.write(0x25c)    -- reset fifo
    HW.SD.BYTECNT.write(0)
    HW.SD.RW.write(0x340)         -- RCST , WCEF, WCST
    HW.SD.CMDRSP.write(0x10000)   -- reserved bit

    HW.SD.CTL.BSEL.write(0)       -- 0 for DMA channels 0-3
                                  -- 1 for DMA channels 4-7

    ATJ.gpio.inen("PORTB", 22, 1)    -- sd detect line active low
end

local function sd_get_clock()
    local sddiv = HW.CMU.SDCLK.SDDIV.read() + 1
    if HW.CMU.SDCLK.D128.read() == 1 then
        sddiv = 128 * sddiv
    end

    local corepllfreq = HW.CMU.COREPLL.CPCK.read() * 6000000

    return math.floor(corepllfreq/sddiv)
end

-- set sd clock frequency
-- freq parameter in HZ
function sdc.set_speed(freq)
    local corepllfreq = bit32.band(HW.CMU.COREPLL.read(), 0x1f)
    corepllfreq = corepllfreq * 6000000

    -- we need to use proper rounding
    local sddiv = (corepllfreq + freq - 1)/freq
    sddiv = math.floor(sddiv - 1)

    if sddiv > 15 then
        sddiv = (corepllfreq/128 + freq - 1)/freq
        sddiv = math.floor(sddiv - 1)

        -- bit4 is DIV128
        -- needed for low clock speed during initialization
        sddiv = bit32.bor(sddiv, 0x10)
    end

    -- bit5 is clock enable
    HW.CMU.SDCLK.write(bit32.bor(sddiv, 0x20))

    HWLIB.printf("SD speed requested: %d, actual set speed: %d\n",
                 freq, sd_get_clock())
end

-- set sd controler bus width
function sdc.set_bus_width(width)
    if width == 1 then width = 0
    elseif width == 4 then width = 1
    elseif width == 8 then width = 2
    else error("Unsuported SD bus width " .. width) end
    HW.SD.CTL.BUSWID.write(width)
end

-- return true if card sits in a slot false otherwise
function sdc.card_present()
    if ATJ.gpio.get("PORTB", 22) == 1 then
        return false
    else
        return true
    end
end

local function data_rd(buf, size)
    local cnt = 0

    while (size > 0) do
        local dma_buf_addr = uncachedaddr(hwstub.dev.layout.buffer.start +
                                          hwstub.dev.layout.buffer.size)

        local xsize = math.min(1024, size)
        HW.DMAC.DMA0_MODE.write(0x008c01b4)           -- aligned iram dst,
                                                      -- sd module src
        HW.DMAC.DMA0_SRC.write(0x100b0030)            -- hw_addr of SD_DAT
        HW.DMAC.DMA0_DST.write(hwaddr(dma_buf_addr))  -- hw_addr of DMA buf
        HW.DMAC.DMA0_CNT.write(xsize)                 -- count

        HW.SD.BYTECNT.write(xsize)
        HW.SD.FIFOCTL.write(0x259)
        HW.SD.RW.write(0x3c0)

        HW.DMAC.DMA0_CMD.write(1) -- DMA kick in

        -- wait for DMA transfer finish
        while (bit32.band(HW.DMAC.DMA0_CMD.read(), 1) == 1) do end

        -- move data from DMA buffer
        for i=1, xsize do
            buf[cnt*1024 + i] = DEV.read8(dma_buf_addr)
            dma_buf_addr = dma_buf_addr + 1
        end

        cnt = cnt + 1
        size = size - xsize
    end
end

-- in case of error the function is fatal i.e
-- it does not retry commands and so on
-- cmd is subtable entry form CMD table
-- arg is optional argument to the command
-- rspdat is table { "response" = {}, "data" = {} }
-- datlen is the lenght of requested data transfer
function sdc.send_cmd(cmd, arg, rspdat, datlen)
    if cmd[1] > 256 then
        -- ACMD
        SDLIB.sdc.send_cmd(SDLIB.CMD.APP_CMD, SDLIB.card_info.rca, rspdat, 0)
    end

    if cmd[1] > 256 then
         HWLIB.printf("send_cmd(ACMD%d, 0x%0x, %d)\t", cmd[1]%256, arg, cmd[2])
    else
         HWLIB.printf("send_cmd(CMD%d, 0x%0x, %d)\t", cmd[1]%256, arg, cmd[2])
    end

    HW.SD.ARG.write(arg)
    HW.SD.CMD.write(cmd[1] % 256)

    -- this sets bit0 to clear STAT
    -- and mark response type
    local cmdrsp = nil
    if cmd[2] == SDLIB.RES.NRSP then                 -- NRSP
        cmdrsp = 0x05
    elseif cmd[2] == SDLIB.RES.R1 or cmd[2] == SDLIB.RES.R6 then -- RSP1 or RSP6
        cmdrsp = 0x03
    elseif cmd[2] == SDLIB.RES.R2 then             -- RSP2
        cmdrsp = 0x11
    elseif cmd[2] == SDLIB.RES.R3 then             -- RSP3
        cmdrsp = 0x09
    else error("Invalid response number " .. cmd[2])
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
            HWLIB.printf("send_cmd(CMD%d, 0x%0x, %d) timeout\n",cmd[1],arg,cmd[2])
            error("send_cmd failed")
            return -1
        end
    end

    -- check crc
    if (cmd[2] ~= SDLIB.RES.NRSP) then
        if (cmd[2] == SDLIB.RES.R1) then
            local crc7 = HW.SD.CRC7.read()
            local rescrc = bit32.band(HW.SD.RSPBUF0.read(), 0xff)
            if (bit32.bxor(crc7, rescrc) ~=0) then
                HWLIB.printf("CRC: 0x%02x != 0x%02x\n", rescrc, crc7)
                error("CRC error")
                return -2
            end
        end
        
        -- copy response accordingly
        if (cmd[2] == SDLIB.RES.R2) then
             rspdat.response[1] = HW.SD.RSPBUF3.read()
             rspdat.response[2] = HW.SD.RSPBUF2.read()
             rspdat.response[3] = HW.SD.RSPBUF1.read()
             rspdat.response[4] = HW.SD.RSPBUF0.read()
            HWLIB.printf("RSP: 0x%08x 0x%08x 0x%08x 0x%08x\n",
                         rspdat.response[1], rspdat.response[2],
                         rspdat.response[3], rspdat.response[4])
        else
            rspdat.response[1] = bit32.bor(bit32.lshift(HW.SD.RSPBUF1.read(), 24),
                                 bit32.rshift(HW.SD.RSPBUF0.read(), 8))
            HWLIB.printf("RSP: 0x%08x\n", rspdat.response[1])
        end
    end

    -- data stage
    if (datlen > 0) then
        data_rd(rspdat.data, datlen)
    end
end


-- shortcut used in testing
function go()
    ATJ.gpio.muxsel("SD")
    SDLIB.register_driver(sdc)
    SDLIB.sdc.init()
    SDLIB.card_init()
end
