--
-- LCDIF
-- 

STMP.lcdif = {}

function STMP.lcdif.init()
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

function STMP.lcdif.set_word_length(bus_width)
    if STMP.is_stmp3600() or STMP.is_stmp3700() then
        if bus_width == 8 then
            HW.LCDIF.CTRL.WORD_LENGTH.set()
        else
            HW.LCDIF.CTRL.WORD_LENGTH.clr()
        end
    else
        error("STMP.lcdif.set_word_length: unimplemented")
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
        error("STMP.lcdif.get_word_length: unimplemented")
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
    HW.LCDIF.CTRL.DATA_SWIZZLE.write(v)
end

function STMP.lcdif.is_busy()
    if STMP.is_stmp3600() then
        return HW.LCDIF.CTRL.FIFO_STATUS.read() == 0
    else
        return HW.LCDIF.STAT.TXFIFO_FULL.read() == 1
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
    HW.LCDIF.CTRL.RUN.clr()
    HW.LCDIF.CTRL.COUNT.write(#data)
    HW.LCDIF.CTRL.RUN.set()
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
        HW.LCDIF.DATA.write(v)
    end
end