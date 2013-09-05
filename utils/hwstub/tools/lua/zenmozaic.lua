--
-- ZEN MOZAIC
--
ZENMOZAIC = {}

function ZENMOZAIC.lcd_send(cmd, data)
    STMP.lcdif.set_data_swizzle(3)
    STMP.lcdif.send_pio(false, {bit32.band(cmd, 0xff), bit32.rshift(cmd, 8)})
    if cmd ~= 0x22 then
        STMP.lcdif.send_pio(true, {bit32.band(data, 0xff), bit32.rshift(data, 8)})
    end
end

function ZENMOZAIC.lcd_init()
    STMP.pinctrl.lcdif.setup_system(8, false)
    STMP.lcdif.init()
    STMP.lcdif.set_word_length(8)
    STMP.lcdif.set_system_timing(2, 2, 2, 2)
    STMP.lcdif.set_reset(1)
    STMP.lcdif.set_reset(0)
    STMP.lcdif.set_reset(1)
    STMP.lcdif.set_byte_packing_format(0xf)

    ZENMOZAIC.lcd_send(0, 1)
    ZENMOZAIC.lcd_send(3, 0)
    
    ZENMOZAIC.lcd_send(3, 0x510)
    ZENMOZAIC.lcd_send(9, 8)
    ZENMOZAIC.lcd_send(0xc, 0)
    ZENMOZAIC.lcd_send(0xd, 0)
    ZENMOZAIC.lcd_send(0xe, 0)
    ZENMOZAIC.lcd_send(0x5b, 4)
    ZENMOZAIC.lcd_send(0xd, 0x10)
    ZENMOZAIC.lcd_send(9, 0)
    ZENMOZAIC.lcd_send(3, 0x10)
    ZENMOZAIC.lcd_send(0xd, 0x14)
    ZENMOZAIC.lcd_send(0xe, 0x2b12)
    ZENMOZAIC.lcd_send(1, 0x21f)
    ZENMOZAIC.lcd_send(2, 0x700)
    ZENMOZAIC.lcd_send(5, 0x30)
    ZENMOZAIC.lcd_send(6, 0)
    ZENMOZAIC.lcd_send(8, 0x202)
    ZENMOZAIC.lcd_send(0xa, 0xc003)
    ZENMOZAIC.lcd_send(0xb, 0)
    ZENMOZAIC.lcd_send(0xf, 0)
    ZENMOZAIC.lcd_send(0x10, 0)
    ZENMOZAIC.lcd_send(0x11, 0)
    ZENMOZAIC.lcd_send(0x14, 0x9f00)
    ZENMOZAIC.lcd_send(0x15, 0x9f00)
    ZENMOZAIC.lcd_send(0x16, 0x7f00)
    ZENMOZAIC.lcd_send(0x17, 0x9f00)
    ZENMOZAIC.lcd_send(0x20, 0)
    ZENMOZAIC.lcd_send(0x21, 0)
    ZENMOZAIC.lcd_send(0x23, 0)
    ZENMOZAIC.lcd_send(0x24, 0)
    ZENMOZAIC.lcd_send(0x25, 0)
    ZENMOZAIC.lcd_send(0x26, 0)
    ZENMOZAIC.lcd_send(0x30, 0x707)
    ZENMOZAIC.lcd_send(0x31, 0x504)
    ZENMOZAIC.lcd_send(0x32, 7)
    ZENMOZAIC.lcd_send(0x33, 0x307)
    ZENMOZAIC.lcd_send(0x34, 7)
    ZENMOZAIC.lcd_send(0x35, 0x400)
    ZENMOZAIC.lcd_send(0x36, 0x607)
    ZENMOZAIC.lcd_send(0x37, 0x703)
    ZENMOZAIC.lcd_send(0x3a, 0x1a0d)
    ZENMOZAIC.lcd_send(0x3b, 0x1309)

    ZENMOZAIC.lcd_send(9, 4)
    ZENMOZAIC.lcd_send(7, 5)
    ZENMOZAIC.lcd_send(7, 0x25)
    ZENMOZAIC.lcd_send(7, 0x27)
    ZENMOZAIC.lcd_send(0x5b, 0)
    ZENMOZAIC.lcd_send(7, 0x37)

    ZENMOZAIC.lcd_send(0x22, 0)
    for i=0,128 do
        STMP.lcdif.send_pio(true, {0xff, 0})
    end
end

function ZENMOZAIC.set_backlight(val)
    local v = math.floor((val + 200) * val / 1000)
    for i=4,0,-1 do
        if bit32.btest(v,bit32.lshift(1,i)) then
            HW.UARTDBG.DR.write(0xff)
        else
            HW.UARTDBG.DR.write(0xf8)
        end
        while HW.UARTDBG.FR.TXFF.read() == 1 do end
    end
    HW.UARTDBG.DR.write(0)
end

function ZENMOZAIC.init()
    ZENMOZAIC.lcd_init()
    HW.LRADC.CTRL0.SFTRST.clr()
    HW.LRADC.CTRL0.CLKGATE.clr()
    HW.LRADC.CHn[0].ACCUMULATE.clr()
    HW.LRADC.CHn[0].NUM_SAMPLES.write(0)
    HW.LRADC.CHn[0].VALUE.write(0)
    local t = {}
    for i = 1,1000,1 do
        HW.LRADC.CTRL0.SCHEDULE.write(1)
        --local time = HW.DIGCTL.MICROSECONDS.read()
        local time = i * 1000
        local val = HW.LRADC.CHn[0].VALUE.read()
        t[#t + 1] = {time, val}
    end
    local file = io.open("data.txt", "w")
    for i,v in ipairs(t) do
        file:write(string.format("%d %d\n", v[1] / 1000, v[2]))
    end
    file:close()
    print("Display curve using:")
    print("gnuplot -persist");
    print("> plot \"data.txt\" using 1:2")
end

