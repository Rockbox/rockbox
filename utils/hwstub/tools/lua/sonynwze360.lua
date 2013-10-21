--
-- Sony NWZ-E370
--
NWZE360 = {}

function NWZE360.lcd_send(cmd, data)
    STMP.lcdif.set_data_swizzle(0)
    STMP.lcdif.set_word_length(8)
    STMP.lcdif.set_byte_packing_format(0xf)
    STMP.lcdif.send_pio(false, {cmd})
    if cmd ~= 0x22 then
        STMP.lcdif.send_pio(true, {data})
    end
end

function NWZE360.lcd_set_update_rect(x, y, w, h)
    NWZE360.lcd_send(2, bit32.rshift(x, 8))
    NWZE360.lcd_send(3, bit32.band(x, 0xff))
    NWZE360.lcd_send(4, bit32.rshift(x + w - 1, 8))
    NWZE360.lcd_send(5, bit32.band(x + w - 1, 0xff))
    NWZE360.lcd_send(6, bit32.rshift(y, 8))
    NWZE360.lcd_send(7, bit32.band(y, 0xff))
    NWZE360.lcd_send(8, bit32.rshift(y + h - 1, 8))
    NWZE360.lcd_send(9, bit32.band(y + h - 1, 0xff))
    NWZE360.lcd_send(0x22, 0)
end

function NWZE360.lcd_enable(en)
    if not en then
        NWZE360.lcd_send(0x1f, 0xd1)
        NWZE360.lcd_send(1, 0x40)
        NWZE360.lcd_send(1, 0xc0)
        NWZE360.lcd_send(0x19, 1)
    else
        NWZE360.lcd_send(0x19, 0x81)
        STMP.digctl.udelay(5000)
        NWZE360.lcd_send(1, 0x40)
        STMP.digctl.udelay(20000)
        NWZE360.lcd_send(1, 0)
        NWZE360.lcd_send(0x1f, 0xd0)
    end
end

function NWZE360.lcd_init()
    STMP.lcdif.setup_clock()
    STMP.pinctrl.lcdif.setup_system(8, false)
    STMP.lcdif.init()
    STMP.lcdif.set_databus_width(8)
    STMP.lcdif.set_system_timing(1, 1, 1, 1)
    STMP.lcdif.set_byte_packing_format(0xf)
    STMP.lcdif.set_reset(1)
    STMP.lcdif.set_reset(0)
    STMP.lcdif.set_reset(1)

    NWZE360.lcd_send(0xEA, 0)
    NWZE360.lcd_send(0xEB, 0x20)
    NWZE360.lcd_send(0xEC, 0xC)
    NWZE360.lcd_send(0xED, 0xC4)
    NWZE360.lcd_send(0xE8, 0x38)
    NWZE360.lcd_send(0xE9, 0xE)
    NWZE360.lcd_send(0xF1, 1)
    NWZE360.lcd_send(0xF2, 8)
    NWZE360.lcd_send(0x2E, 0x86)
    NWZE360.lcd_send(0x29, 0xFF)
    NWZE360.lcd_send(0xE4, 1)
    NWZE360.lcd_send(0xE5, 0x20)
    NWZE360.lcd_send(0xE7, 1)
    NWZE360.lcd_send(0x40, 0)
    NWZE360.lcd_send(0x41, 0)
    NWZE360.lcd_send(0x42, 0)
    NWZE360.lcd_send(0x43, 0x14)
    NWZE360.lcd_send(0x44, 0x14)
    NWZE360.lcd_send(0x45, 0x28)
    NWZE360.lcd_send(0x46, 0x11)
    NWZE360.lcd_send(0x47, 0x57)
    NWZE360.lcd_send(0x48, 5)
    NWZE360.lcd_send(0x49, 0x16)
    NWZE360.lcd_send(0x4A, 0x19)
    NWZE360.lcd_send(0x4B, 0x1A)
    NWZE360.lcd_send(0x4C, 0x1A)
    NWZE360.lcd_send(0x50, 0x17)
    NWZE360.lcd_send(0x51, 0x2B)
    NWZE360.lcd_send(0x52, 0x2B)
    NWZE360.lcd_send(0x53, 0x3F)
    NWZE360.lcd_send(0x54, 0x3F)
    NWZE360.lcd_send(0x55, 0x3F)
    NWZE360.lcd_send(0x56, 0x28)
    NWZE360.lcd_send(0x57, 0x6E)
    NWZE360.lcd_send(0x58, 5)
    NWZE360.lcd_send(0x59, 5)
    NWZE360.lcd_send(0x5A, 6)
    NWZE360.lcd_send(0x5B, 9)
    NWZE360.lcd_send(0x5C, 0x1A)
    NWZE360.lcd_send(0x5D, 0xCC)
    NWZE360.lcd_send(0x1B, 0x1B)
    NWZE360.lcd_send(0x1A, 1)
    NWZE360.lcd_send(0x24, 0x2F) -- something special here
    NWZE360.lcd_send(0x25, 0x57) -- something special here
    NWZE360.lcd_send(0x23, 0x8A)
    NWZE360.lcd_send(0x2F, 1)
    NWZE360.lcd_send(0x60, 0)
    NWZE360.lcd_send(0x16, 8)
    NWZE360.lcd_send(0x18, 0x36) -- something special here
    NWZE360.lcd_send(0x19, 1)
    STMP.digctl.udelay(5000)
    NWZE360.lcd_send(1, 0)
    NWZE360.lcd_send(0x1F, 0x88)
    STMP.digctl.udelay(5000)
    NWZE360.lcd_send(0x1F, 0x80)
    STMP.digctl.udelay(5000)
    NWZE360.lcd_send(0x1F, 0x90)
    STMP.digctl.udelay(5000)
    NWZE360.lcd_send(0x1F, 0xD0)
    STMP.digctl.udelay(5000)
    NWZE360.lcd_send(0x17, 6)
    NWZE360.lcd_send(0x37, 0)
    NWZE360.lcd_send(0x28, 0x38)
    STMP.digctl.udelay(40000)
    NWZE360.lcd_send(0x28, 0x3C)

    --NWZE360.lcd_send(0x36, 0xc0) -- no effect ?
    --NWZE360.lcd_send(0x16, 8 + 0x60) -- redraw with landscape orientation
    --NWZE360.lcd_send(0x28, 0x34) -- display control
    --NWZE360.lcd_send(0x60, 0x8) -- no effect ?
    NWZE360.lcd_send(0x16, 0) -- BGR <-> RGB

    NWZE360.set_backlight(100)

    NWZE360.lcd_set_update_rect(0, 0, 240, 320)
    STMP.lcdif.set_word_length(16)
    for i = 0, 319 do
        data = {}
        for j = 0, 239 do
            r = 0x1f
            g = 0x0
            b = 0x0
            pix = bit32.bor(b, bit32.bor(bit32.lshift(g, 6), bit32.lshift(r, 11)))
            data[#data + 1] = pix
        end
        STMP.lcdif.send_pio(true, data)
    end
end

function NWZE360.set_backlight(val)
    STMP.pinctrl.pin(0, 10).muxsel('GPIO')
    STMP.pinctrl.pin(0, 10).enable()
    STMP.pinctrl.pin(0, 10).set()
end

function NWZE360.init()
    NWZE360.lcd_init()
    NWZE360.set_backlight(100)
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
    print("gnuplot -persist")
    print("> plot \"data.txt\" using 1:2")
    ]]--
end

 
 
