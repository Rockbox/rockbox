--
-- Sony NWZ-E370
--
NWZE370 = {}

function NWZE370.lcd_send(cmd, data)
    STMP.lcdif.set_data_swizzle(0)
    STMP.lcdif.set_byte_packing_format(0xf)
    STMP.lcdif.set_word_length(8)
    STMP.lcdif.send_pio(false, {cmd})
    if #data ~= 0 then
        STMP.lcdif.send_pio(true, data)
    end
end

function NWZE370.lcd_set_update_rect(x, y, w, h)
    NWZE370.lcd_send(0x2a, {0, x, 0, x + w - 1})
    NWZE370.lcd_send(0x2b, {0, y, 0, y + h - 1})
    NWZE370.lcd_send(0x2c, {})
end

function NWZE370.lcd_init()
    STMP.lcdif.setup_clock()
    STMP.pinctrl.lcdif.setup_system(8, false)
    STMP.lcdif.init()
    STMP.lcdif.set_databus_width(8)
    STMP.lcdif.set_word_length(8)
    STMP.lcdif.set_system_timing(2, 2, 2, 2)
    STMP.lcdif.set_byte_packing_format(0xf)
    STMP.lcdif.set_reset(1)
    STMP.lcdif.set_reset(0)
    STMP.lcdif.set_reset(1)

    STMP.digctl.mdelay(150)
    NWZE370.lcd_send(1, {}) -- software reset
    NWZE370.lcd_send(0x11, {}) -- slee out
    STMP.digctl.mdelay(150)
    NWZE370.lcd_send(0x26, {4}) -- gamma set (curve 3)
    NWZE370.lcd_send(0xb1, {9, 0xd}) -- frame rate (DIVA=9, VPA=13) => 88.9 Hz ?
    NWZE370.lcd_send(0xc0, {8, 0}) -- power control 1 (GVDD=4.4 V, VCI1=2.75 V)
    NWZE370.lcd_send(0xc1, {5}) -- power control 2 (setting 5)
    NWZE370.lcd_send(0xc5, {0x31, 0x40}) -- VCOM control 1 (0x31, 0x40)
    NWZE370.lcd_send(0xc7, {0xc8}) -- VCOM offset control (0xc8)
    NWZE370.lcd_send(0xec, {0xc}) -- 
    NWZE370.lcd_send(0x3a, {5}) -- interface pixel format (16 bit/pixel)
    NWZE370.lcd_send(0x2a, {0, 0, 0, 0x7f}) -- column address (0, 127)
    NWZE370.lcd_send(0x2b, {0, 0, 0, 0x9f}) -- page address set (0, 159)
    NWZE370.lcd_send(0x35, {0}) -- tear effect line on (M=0)
    NWZE370.lcd_send(0x36, {0xc8}) -- memory access control (MH=0, BGR, ML=MV=0, MX=MY=1)
    NWZE370.lcd_send(0xb4, {0}) -- display inversion (NLA=NLB=NLC=0)
    NWZE370.lcd_send(0xb7, {0}) -- source driver direction control (CRL=0)
    NWZE370.lcd_send(0xb8, {0}) -- gate driver direction control (CTB=0)
    NWZE370.lcd_send(0xf2, {1}) -- gamma adjustment (enable)
    NWZE370.lcd_send(0xe0, {0x3f, 0x20, 0x1d, 0x2d, 0x26, 0xc, 0x4b, 0xb7,
       0x39, 0x17, 0x1d, 0x16, 0x16, 0x10, 0}) -- positive gamma
    NWZE370.lcd_send(0xe1, {0, 0x1f, 0x21, 0x12, 0x18, 0x13, 0x34, 0x48,
       0x46, 8, 0x21, 0x29, 0x28, 0x2f, 0x3f}) --negative gamma
    NWZE370.lcd_send(0x29, {}) -- display on

    NWZE370.lcd_set_update_rect(0, 0, 128, 160)
    STMP.lcdif.set_data_swizzle(3)
    STMP.lcdif.set_word_length(8)
    for i = 0, 10 do
        data = {}
        for j = 0, 127 do
            r = 0x1f
            g = 0x0
            b = 0x0
            pix = bit32.bor(b, bit32.bor(bit32.lshift(g, 6), bit32.lshift(r, 11)))
            data[#data + 1] = bit32.band(pix, 0xff)
            data[#data + 1] = bit32.rshift(pix, 8)
            --data[#data + 1] = pix
        end
        STMP.lcdif.send_pio(true, data)
    end
end

function NWZE370.set_backlight(val)
    STMP.pinctrl.pin(0, 10).muxsel('GPIO')
    STMP.pinctrl.pin(0, 10).enable()
    STMP.pinctrl.pin(0, 10).set()
end

function NWZE370.init()
    NWZE370.set_backlight(100)
    NWZE370.lcd_init()
    --[[
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
    ]]--
end

 
