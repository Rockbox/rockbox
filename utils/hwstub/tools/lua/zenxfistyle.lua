--
-- ZEN X-Fi Style
--
ZENXFISTYLE = {}

function ZENXFISTYLE.lcd_encode(data)
    return bit32.bor(
        bit32.lshift(bit32.band(data, 0xff00), 2),
        bit32.lshift(bit32.band(data, 0xff), 1))
end

function ZENXFISTYLE.lcd_write(reg, data)
    STMP.lcdif.send_pio(false, {ZENXFISTYLE.lcd_encode(reg)})
    if reg ~= 0x22 then
        STMP.lcdif.send_pio(true, {ZENXFISTYLE.lcd_encode(data)})
    end
end

function ZENXFISTYLE.lcd_init()
    STMP.lcdif.setup_clock()
    STMP.pinctrl.lcdif.setup_system(18, false)
    STMP.lcdif.init()
    STMP.lcdif.set_word_length(18)
    STMP.lcdif.set_databus_width(18)
    STMP.lcdif.set_system_timing(2, 2, 3, 3)
    STMP.lcdif.set_reset(1)
    STMP.lcdif.set_reset(0)
    STMP.lcdif.set_reset(1)
    STMP.lcdif.set_byte_packing_format(0xf)

    STMP.pinctrl.pin(2, 8).muxsel('GPIO')
    STMP.pinctrl.pin(2, 8).disable()
    ZENXFISTYLE.lcd_model = STMP.pinctrl.pin(2, 8).read()
    print(string.format("LCD model: %d", ZENXFISTYLE.lcd_model))

    if ZENXFISTYLE.lcd_model == 0 then
        ZENXFISTYLE.lcd_write(0xe3, 0x3008)
        ZENXFISTYLE.lcd_write(0xe7, 0x12)
        ZENXFISTYLE.lcd_write(0xef, 0x1231)
        ZENXFISTYLE.lcd_write(0x01, 0x100)
        ZENXFISTYLE.lcd_write(0x02, 0x700)
        ZENXFISTYLE.lcd_write(0x03, 0x1028)
        ZENXFISTYLE.lcd_write(0x04, 0)
        ZENXFISTYLE.lcd_write(0x08, 0x207)
        ZENXFISTYLE.lcd_write(0x09, 0)
        ZENXFISTYLE.lcd_write(0x0a, 0)
        ZENXFISTYLE.lcd_write(0x0c, 0)
        ZENXFISTYLE.lcd_write(0x0d, 0)
        ZENXFISTYLE.lcd_write(0x0f, 0)
        ZENXFISTYLE.lcd_write(0x10, 0)
        ZENXFISTYLE.lcd_write(0x11, 7)
        ZENXFISTYLE.lcd_write(0x12, 0)
        ZENXFISTYLE.lcd_write(0x13, 0)
        hwstub.mdelay(200)
        ZENXFISTYLE.lcd_write(0x10, 0x1490)
        ZENXFISTYLE.lcd_write(0x11, 0x227)
        hwstub.mdelay(50)
        ZENXFISTYLE.lcd_write(0x12, 0x9c)
        hwstub.mdelay(50)
        ZENXFISTYLE.lcd_write(0x13, 0xc00)
        ZENXFISTYLE.lcd_write(0x29, 5)
        ZENXFISTYLE.lcd_write(0x2b, 0xc)
        ZENXFISTYLE.lcd_write(0x20, 0xef)
        ZENXFISTYLE.lcd_write(0x21, 0)
        ZENXFISTYLE.lcd_write(0x30, 6)
        ZENXFISTYLE.lcd_write(0x31, 0x703)
        ZENXFISTYLE.lcd_write(0x32, 0x206)
        ZENXFISTYLE.lcd_write(0x35, 4)
        ZENXFISTYLE.lcd_write(0x36, 0x1a05)
        ZENXFISTYLE.lcd_write(0x37, 0x600)
        ZENXFISTYLE.lcd_write(0x38, 0x307)
        ZENXFISTYLE.lcd_write(0x39, 0x707)
        ZENXFISTYLE.lcd_write(0x3c, 0x400)
        ZENXFISTYLE.lcd_write(0x3d, 0x50f)
        ZENXFISTYLE.lcd_write(0x50, 0)
        ZENXFISTYLE.lcd_write(0x51, 0xef)
        ZENXFISTYLE.lcd_write(0x52, 0)
        ZENXFISTYLE.lcd_write(0x53, 0x13f)
        ZENXFISTYLE.lcd_write(0x60, 0xa700)
        ZENXFISTYLE.lcd_write(0x61, 1)
        ZENXFISTYLE.lcd_write(0x6a, 0)
        ZENXFISTYLE.lcd_write(0x80, 0)
        ZENXFISTYLE.lcd_write(0x81, 0)
        ZENXFISTYLE.lcd_write(0x82, 0)
        ZENXFISTYLE.lcd_write(0x83, 0)
        ZENXFISTYLE.lcd_write(0x84, 0)
        ZENXFISTYLE.lcd_write(0x85, 0)
        ZENXFISTYLE.lcd_write(0x90, 0x10)
        ZENXFISTYLE.lcd_write(0x92, 0x600)
        ZENXFISTYLE.lcd_write(0x07, 0x133)
        ZENXFISTYLE.lcd_write(0x22, 0)
    else
        ZENXFISTYLE.lcd_write(0x01, 0x100)
        ZENXFISTYLE.lcd_write(0x02, 0x700)
        ZENXFISTYLE.lcd_write(0x03, 0x1028)
        ZENXFISTYLE.lcd_write(0x04, 0)
        ZENXFISTYLE.lcd_write(0x08, 0x207)
        ZENXFISTYLE.lcd_write(0x09, 0)
        ZENXFISTYLE.lcd_write(0x0a, 0)
        ZENXFISTYLE.lcd_write(0x0c, 0)
        ZENXFISTYLE.lcd_write(0x0d, 0)
        ZENXFISTYLE.lcd_write(0x0f, 0)
        ZENXFISTYLE.lcd_write(0x10, 0)
        ZENXFISTYLE.lcd_write(0x11, 7)
        ZENXFISTYLE.lcd_write(0x12, 0)
        ZENXFISTYLE.lcd_write(0x13, 0)
        hwstub.mdelay(200)
        ZENXFISTYLE.lcd_write(0x10, 0x1290)
        ZENXFISTYLE.lcd_write(0x11, 0x227)
        hwstub.mdelay(50)
        ZENXFISTYLE.lcd_write(0x12, 0x9c)
        hwstub.mdelay(50)
        ZENXFISTYLE.lcd_write(0x13, 0x1f00)
        ZENXFISTYLE.lcd_write(0x29, 0x30)
        ZENXFISTYLE.lcd_write(0x2b, 0xd)
        ZENXFISTYLE.lcd_write(0x20, 0xef)
        ZENXFISTYLE.lcd_write(0x21, 0)
        ZENXFISTYLE.lcd_write(0x30, 0x404)
        ZENXFISTYLE.lcd_write(0x31, 0x404)
        ZENXFISTYLE.lcd_write(0x32, 0x404)
        ZENXFISTYLE.lcd_write(0x37, 0x303)
        ZENXFISTYLE.lcd_write(0x38, 0x303)
        ZENXFISTYLE.lcd_write(0x39, 0x303)
        ZENXFISTYLE.lcd_write(0x35, 0x103)
        ZENXFISTYLE.lcd_write(0x3c, 0x301)
        ZENXFISTYLE.lcd_write(0x36, 0x1e00)
        ZENXFISTYLE.lcd_write(0x3d, 0xf)
        ZENXFISTYLE.lcd_write(0x50, 0)
        ZENXFISTYLE.lcd_write(0x51, 0xef)
        ZENXFISTYLE.lcd_write(0x52, 0)
        ZENXFISTYLE.lcd_write(0x53, 0x13f)
        ZENXFISTYLE.lcd_write(0x60, 0xa700)
        ZENXFISTYLE.lcd_write(0x61, 0)
        ZENXFISTYLE.lcd_write(0x6a, 0)
        ZENXFISTYLE.lcd_write(0x80, 0)
        ZENXFISTYLE.lcd_write(0x81, 0)
        ZENXFISTYLE.lcd_write(0x82, 0)
        ZENXFISTYLE.lcd_write(0x83, 0)
        ZENXFISTYLE.lcd_write(0x84, 0)
        ZENXFISTYLE.lcd_write(0x85, 0)
        ZENXFISTYLE.lcd_write(0x2b, 0xd)
        hwstub.mdelay(50)
        ZENXFISTYLE.lcd_write(0x90, 0x17)
        ZENXFISTYLE.lcd_write(0x92, 0)
        ZENXFISTYLE.lcd_write(0x93, 3)
        ZENXFISTYLE.lcd_write(0x95, 0x110)
        ZENXFISTYLE.lcd_write(0x97, 0)
        ZENXFISTYLE.lcd_write(0x98, 0)
        ZENXFISTYLE.lcd_write(0x07, 0x133)
        ZENXFISTYLE.lcd_write(0x22, 0)
    end

    for i = 0, 319 do
        STMP.lcdif.send_pio(true, {0x3f})
    end
    for i = 0, 319 do
        STMP.lcdif.send_pio(true, {0xfc0})
    end
    for i = 0, 319 do
        STMP.lcdif.send_pio(true, {0x3f000})
    end
end

function ZENXFISTYLE.backlight_init()
    STMP.pinctrl.pin(1, 30).muxsel('GPIO')
    STMP.pinctrl.pin(1, 30).enable()
end

function ZENXFISTYLE.set_backlight(val)
    if val == 0 then
        STMP.pinctrl.pin(1, 30).clr()
    else
        STMP.pinctrl.pin(1, 30).set()
    end
end

function ZENXFISTYLE.init()
    ZENXFISTYLE.lcd_init()
    ZENXFISTYLE.backlight_init()
    ZENXFISTYLE.set_backlight(50)
end

 
