--
-- Fiio X1
--
FIIOX1 = {}

-- 0 is PF3, 1 is PE1
function FIIOX1.get_backlight_type()
    return FIIOX1.bl_type
end

-- 0 is V2, 1 is V1
function FIIOX1.get_hw_type()
    return FIIOX1.hw_type
end

function FIIOX1.hw_detect()
    -- PA12 is used to detect hardware version
    JZ.gpio.pin(0, 12).std_gpio_out(1)
    FIIOX1.hw_type = JZ.gpio.pin(0, 12).read()
    -- PA13 is used to detect backlight type
    JZ.gpio.pin(0, 13).std_gpio_out(1)
    FIIOX1.bl_type = JZ.gpio.pin(0, 13).read()

    if FIIOX1.hw_type == 1 then
        print("Fiio X1: hardware version: V01")
    else
        print("Fiio X1: hardware version: V02")
    end
    print(string.format("Fiio X1: backlight type: %s", FIIOX1.bl_type))
end

function FIIOX1.get_backlight_pin()
    if FIIOX1.get_backlight_type() == 0 then
        -- PF3
        return JZ.gpio.pin(5, 3)
    else
        -- PE1
        return JZ.gpio.pin(4, 1)
    end
end

function FIIOX1.init_backligt()
    -- setup as output, high level to make no change
    FIIOX1.get_backlight_pin().std_gpio_out(1)
end

function FIIOX1.enable_backlight(en)
    local pin = FIIOX1.get_backlight_pin()
    pin.clr()
    hwstub.mdelay(1)
    if en then
        pin.set()
    end
end

function FIIOX1.test_backlight()
    print("backlight test")
    print("enable backlight")
    FIIOX1.enable_backlight(true)
    print("sleep for 1 sec")
    hwstub.mdelay(1000)
    print("disable backlight")
    FIIOX1.enable_backlight(false)
    print("sleep for 1 sec")
    hwstub.mdelay(1000)
    print("enable backlight")
    FIIOX1.enable_backlight(true)
end

function FIIOX1.setup_fiio_lcd_pins()
    -- PE4: reset pin
    JZ.gpio.pin(4, 4).std_gpio_out(1)
    -- PC9: unknown
    JZ.gpio.pin(2, 9).std_gpio_out(0)
    -- PC2: unknown
    JZ.gpio.pin(2, 2).std_gpio_out(1)
    -- PF0: unknown
    JZ.gpio.pin(5, 0).std_gpio_out(1)
end

function FIIOX1.lcd_reset()
    local pin = JZ.gpio.pin(4, 4)
    pin.set()
    hwstub.mdelay(50)
    pin.clr()
    hwstub.mdelay(50)
    pin.set()
    hwstub.mdelay(150)
end

function FIIOX1.init_lcd()
    -- setup Fiio X1 specific pins
    FIIOX1.setup_fiio_lcd_pins()
    -- reset lcd
    JZ.lcd_reset()
end

-- call with nil to get automatic name
function FIIOX1.dump_ipl(file)
    FIIOX1.hw_detect()
    if file == nil then
        file = "fiiox1_ipl_hw_v" .. FIIOX1.hw_type .. ".bin"
    end
    print("Dumping IPL to " .. file .." ...")
    JZ.nand.rom.init()
    JZ.nand.rom.read_flags()
    local ipl = JZ.nand.rom.read_bootloader()
    JZ.nand.rom.write_to_file(file, ipl)
end

-- call with nil to get automatic name
function FIIOX1.dump_spl(file)
    FIIOX1.hw_detect()
    if file == nil then
        file = "fiiox1_spl_hw_v" .. FIIOX1.hw_type .. ".bin"
    end
    print("Dumping SPL to " .. file .." ...")
    -- hardcoded parameters are specific to the Fiio X1
    local nand_params = {
        bus_width = 16,
        row_cycle = 2,
        col_cycle = 2,
        page_size = 2048,
        page_per_block = 64,
        oob_size = 64,
        badblock_pos = 0,
        badblock_page = 0,
        ecc_pos = 4,
        ecc_size = 13,
        ecc_level = 8,
        addr_setup_time = 4,
        addr_hold_time = 4,
        write_strobe_time = 4,
        read_strobe_time = 4,
        recovery_time = 13,
    }
    local spl = JZ.nand.rom.read_spl(nand_params, 0x400, 0x200)
    JZ.nand.rom.write_to_file(file, spl)
end

function FIIOX1.init()
    FIIOX1.init_backligt()
    FIIOX1.test_backlight()
    FIIOX1.init_lcd()
end
