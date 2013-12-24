--
-- LCDIF
-- 

STMP.lcdif = {}

function STMP.lcdif.setup_clock()
    if not STMP.is_stmp3600() then
        HW.CLKCTRL.CLKSEQ.BYPASS_PIX.set()
        HW.CLKCTRL.PIX.CLKGATE.write(0)
        HW.CLKCTRL.PIX.DIV.write(1)
    end
end

function STMP.lcdif.init()
    HW.LCDIF.CTRL.SFTRST.clr()
    HW.LCDIF.CTRL.CLKGATE.clr()
    HW.LCDIF.CTRL.SFTRST.set()
    HW.LCDIF.CTRL.SFTRST.clr()
    HW.LCDIF.CTRL.CLKGATE.clr()
end

function STMP.lcdif.set_system_timing(data_setup, data_hold, cmd_setup, cmd_hold)
    HW.LCDIF.TIMING.CMD_HOLD.write(cmd_hold)
    HW.LCDIF.TIMING.CMD_SETUP.write(cmd_setup)
    HW.LCDIF.TIMING.DATA_HOLD.write(data_hold)
    HW.LCDIF.TIMING.DATA_SETUP.write(data_setup)
end

function STMP.lcdif.set_byte_packing_format(val)
    HW.LCDIF.CTRL1.BYTE_PACKING_FORMAT.write(val)
end

function STMP.lcdif.set_reset(val)
    if STMP.is_stmp3600() then
        HW.LCDIF.CTRL.RESET.write(val)
    else
        HW.LCDIF.CTRL1.RESET.write(val)
    end
end

function STMP.lcdif.set_databus_width(bus_width)
    local v = 0
    if bus_width == 8 then
        v = 1
    elseif bus_width == 18 then
        v = 2
    elseif bus_width == 24 then
        v = 3
    end
    HW.LCDIF.CTRL.LCD_DATABUS_WIDTH.write(v)
end

function STMP.lcdif.set_word_length(bus_width)
    if STMP.is_stmp3600() or STMP.is_stmp3700() then
        if bus_width == 8 then
            HW.LCDIF.CTRL.WORD_LENGTH.set()
        else
            HW.LCDIF.CTRL.WORD_LENGTH.clr()
        end
    else
        local v = 0
        if bus_width == 8 then
            v = 1
        elseif bus_width == 18 then
            v = 2
        elseif bus_width == 24 then
            v = 3
        end
        HW.LCDIF.CTRL.WORD_LENGTH.write(v)
    end
end

function STMP.lcdif.get_word_length()
    if STMP.is_stmp3600() or STMP.is_stmp3700() then
        if HW.LCDIF.CTRL.WORD_LENGTH.read() == 1 then
            return 8
        else
            return 16
        end
    else
        local v = HW.LCDIF.CTRL.WORD_LENGTH.read()
        if v == 0 then return 16
        elseif v == 1 then return 8
        elseif v == 2 then return 18
        else return 24 end
    end
end

function STMP.lcdif.set_data_swizzle(swizzle)
    local v = swizzle
    if type(swizzle) == "string" then
        if swizzle == "NONE" then
            v = 0
        else
            error("unimplemented")
        end
    end
    if STMP.is_stmp3600() or STMP.is_stmp3700() then 
        HW.LCDIF.CTRL.DATA_SWIZZLE.write(v)
    else
        HW.LCDIF.CTRL.INPUT_DATA_SWIZZLE.write(v)
    end
end

function STMP.lcdif.is_busy()
    if STMP.is_stmp3600() then
        return HW.LCDIF.CTRL.FIFO_STATUS.read() == 0
    else
        return HW.LCDIF.STAT.TXFIFO_FULL.read() == 1
    end
end

function STMP.lcdif.wait_ready()
    while HW.LCDIF.CTRL.RUN.read() == 1 do
    end
end

function STMP.lcdif.send_pio(data_mode, data)
    local wl = STMP.lcdif.get_word_length()
    if data_mode then
        HW.LCDIF.CTRL.DATA_SELECT.set()
    else
        HW.LCDIF.CTRL.DATA_SELECT.clr()
    end
    STMP.debug(string.format("lcdif: count = %d", #data))
    if STMP.is_imx233() then
        HW.LCDIF.CTRL.LCDIF_MASTER.clr()
    end
    HW.LCDIF.CTRL.RUN.clr()
    if STMP.is_stmp3600() or STMP.is_stmp3700() then
        HW.LCDIF.CTRL.COUNT.write(#data)
    else
        HW.LCDIF.TRANSFER_COUNT.V_COUNT.write(1)
        HW.LCDIF.TRANSFER_COUNT.H_COUNT.write(#data)
    end
    HW.LCDIF.CTRL.RUN.set()
    if wl == 18 then
        wl = 32
    end
    local i = 1
    while i <= #data do
        local v = 0
        local v_size = 0
        while i <= #data and v_size + wl <= 32 do
            v = bit32.bor(v, bit32.lshift(data[i], v_size))
            v_size = v_size + wl
            i = i + 1
        end
        STMP.debug(string.format("lcdif: i=%d send 0x%x", i, v))
        while STMP.lcdif.is_busy() do STMP.debug("lcdif: fifo full") end
        STMP.debug(string.format("lcdif: write 0x%x", v))
        HW.LCDIF.DATA.write(v)
    end
    STMP.debug("lcdif: wait end of command")
    STMP.lcdif.wait_ready()
end

function STMP.lcdif.set_mode86(mode86)
    if mode86 then
        HW.LCDIF.CTRL.MODE86.set()
    else
        HW.LCDIF.CTRL.MODE86.clr()
    end
end