--
-- Sansa Express
--
SANSAEXPRESS = {} 

function SANSAEXPRESS.set_backlight(val)
    STMP.pinctrl.pin(3, 13).muxsel('GPIO')
    STMP.pinctrl.pin(3, 13).enable()
    STMP.pinctrl.pin(3, 13).clr()
    for i = 0, val - 1 do
        STMP.pinctrl.pin(3, 13).clr()
        STMP.pinctrl.pin(3, 13).set()
    end
end

function SANSAEXPRESS.lcd_init()
    STMP.lcdif.setup_clock()
    STMP.pinctrl.lcdif.setup_system(8, false)
    STMP.lcdif.init()
    STMP.lcdif.set_word_length(8)
    STMP.lcdif.set_system_timing(4, 4, 1, 1)
    STMP.lcdif.set_reset(1)
    STMP.lcdif.set_reset(0)
    STMP.lcdif.set_reset(1)
    STMP.lcdif.send_pio(false,
        {
            0x0a, -- set lower column address: 10
            0xa1, -- set segment ramp: 131 -> 0
            0xda, -- set COM configuration:
            0x12, -- -> use alternative
            0xc0, -- set COM scan dir: normal
            0xa8, -- set multiplex ratio:
            0x3f, -- -> 63MUX
            0xd5, -- set display clock freq:
            0x50, -- -> divide ratio = 1, osc freq = 5
            0xdb, -- set vdcom deselect level
            0x08, -- -> 8
            0x81, -- set contrast register
            0x25, -- -> 0x25
            0xad, -- set DCDC on/off
            0x8a, -- -> off
            0xc8, -- set com output scan: reverse
        })
    for page = 0, 7 do
        STMP.lcdif.send_pio(false,
            {
                0xb0 + page, -- set page address
                0x02, -- set low column address: 2
                0x10, -- set higher column address
            })
        for col = 0, 15 do
            STMP.lcdif.send_pio(true, {0, 0, 0, 0, 0, 0, 0, 0})
        end
        STMP.lcdif.send_pio(true, {0, 0, 0, 0})
    end
    STMP.lcdif.send_pio(false, {0xaf}) -- turn on panel
    -- wait
    STMP.lcdif.send_pio(false,
        {
            0x40, -- set display start line: 0
            0xa4, -- set entire display: normal
            0xa6, -- set normal display: normal
            0xd3, -- set vertical scroll:
            0x00, -- -> 0
            0xd9, -- set precharge period
            0x1f, -- -> 31
        })
    for page = 0, 7 do
        STMP.lcdif.send_pio(false,
            {
                0xb0 + page, -- set page address
                0x02, -- set low column address: 2
                0x10, -- set higher column address
            })
        for col = 0, 15 do
            STMP.lcdif.send_pio(true, {0, 0, 0, 0, 0, 0, 0, 0})
        end
    end
end

function SANSAEXPRESS.set_pixel(x, y, val)
    page = math.floor(y / 8)
    col = 2 + x -- LCD has two column offset
    col_low = bit32.band(col, 0xf)
    col_high = bit32.rshift(col ,4)
    col_off = y % 8
    STMP.lcdif.send_pio(false, {0x00 + col_low, 0x10 + col_high, 0xb0 + page})
    print(col_off)
    byte = bit32.lshift(val, col_off)
    print(byte)
    STMP.lcdif.send_pio(true, {byte})
end

function SANSAEXPRESS.init()
    SANSAEXPRESS.lcd_init()
    SANSAEXPRESS.set_backlight(20)
end