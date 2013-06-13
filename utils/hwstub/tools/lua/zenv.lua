--
-- ZEN V
--
ZENV = {}

function ZENV.lcd_send(cmd, data)
    if #cmd == 1 and cmd[1] == 0x5c then
        STMP.lcdif.set_word_length(16)
        STMP.lcdif.set_data_swizzle("NONE")
    else
        STMP.lcdif.set_word_length(8)
        STMP.lcdif.set_data_swizzle("NONE")
    end
    STMP.lcdif.send_pio(false, cmd)
    STMP.lcdif.send_pio(true, data)
end

function ZENV.lcd_init()
    STMP.pinctrl.lcdif.setup_system(16, false)
    STMP.pinctrl.pin(3, 16).muxsel("GPIO")
    STMP.pinctrl.pin(3, 16).disable()
    if STMP.pinctrl.pin(3, 16).read() == 0 then
        STMP.debug("ZENV: need lcd power init")
        STMP.pinctrl.pin(0, 27).muxsel("GPIO")
        STMP.pinctrl.pin(0, 27).enable()
        STMP.pinctrl.pin(0, 27).set()
    end

    STMP.lcdif.init()
    STMP.lcdif.set_system_timing(2, 2, 2, 2)
    STMP.lcdif.set_reset(1)
    STMP.lcdif.set_reset(0)
    STMP.lcdif.set_reset(1)

    ZENV.lcd_send({0xca}, {0x7f})
    ZENV.lcd_send({0xa0}, {0x75})
    ZENV.lcd_send({0xc7}, {0x08})
    ZENV.lcd_send({0xbe}, {0x18})
    ZENV.lcd_send({0xc1}, {0x7b, 0x69, 0x9f})
    ZENV.lcd_send({0xb1}, {0x1f})
    ZENV.lcd_send({0xb3}, {0x80})
    ZENV.lcd_send({0xbb}, {0x00, 0x00, 0x00})
    ZENV.lcd_send({0xad}, {0x8a})
    ZENV.lcd_send({0xb0}, {0x00})
    ZENV.lcd_send({0xd1}, {0x02})
    ZENV.lcd_send({0xb9}, {})
    ZENV.lcd_send({0x92}, {0x01})
    ZENV.lcd_send({0xa2}, {0x80})
    ZENV.lcd_send({0x9e}, {})
    ZENV.lcd_send({0xa6}, {})
    ZENV.lcd_send({0xaf}, {})
end

function ZENV.init()
    ZENV.lcd_init()
end
