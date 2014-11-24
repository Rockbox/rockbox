--
-- Samsung YP-CP3
--
SAMSUNGYPCP3 = {}

function SAMSUNGYPCP3.backlight_init()
    -- PD4 as output
    HW.GPIO0.PDCON.write(bit32.replace(HW.GPIO0.PDCON.read(), 1, 4))
--    HW.GPIO0.PDCON.write(bit32.bor(HW.GPIO0.PDCON.read(), bit32.lshift(1,4)))

    -- low (backlight off)
    HW.GPIO0.PDDR.write(bit32.replace(HW.GPIO0.PDDR.read(), 0, 4))
--    HW.GPIO0.PDDR.write(bit32.band(HW.GPIO0.PDDR.read(),bit32.bnot(bit32.lshift(1,4))))

    -- IOMUXB - set PWM0 pin as GPIO
    HW.SCU.IOMUXB_CON.write(bit32.replace(HW.SCU.IOMUXB_CON.read(), 0, 11))
--    HW.SCU.IOMUXB_CON.write(bit32.band(HW.SCU.IOMUXB_CON.read(), bit32.bnot(bit32.lshift(1,11))))

    -- DIV/2, PWM reset
    HW.PWM0.CTRL.write(bit32.lshift(1,7))

    -- taken from generic
    -- set pwm frequency
    -- (apb_freq/pwm_freq)/pwm_div = (50 000 000/pwm_freq)/2
    HW.PWM0.LRC.write(50000)
    HW.PWM0.HRC.write(20000)
    
    -- reset counter
    HW.PWM0.CNTR.write(0)
    
    -- DIV/2, PWM output enable, PWM timer enable
    HW.PWM0.CTRL.write(bit32.bor(bit32.lshift(1,3), bit32.lshift(1,0)))
end

function SAMSUNGYPCP3.backlight_on()
    HW.SCU.IOMUXB_CON.write(bit32.replace(HW.SCU.IOMUXB_CON.read(), 1, 11))
    HW.PWM0.CTRL.write(bit32.bor(HW.PWM0.CTRL.read(), bit32.lshift(1,3), bit32.lshift(1,0)))
end

function SAMSUNGYPCP3.backlight_set(val)
    if val > 50000 then
        val = 50000
    elseif val < 0 then
        val = 0
    end

    HW.PWM0.HRC.write(val)
end

function SAMSUNGYPCP3.lcd_init()
    RK27XX.lcdif.iomux()
    RK27XX.lcdif.lcdctrl_init()

    -- reset
    RK27XX.lcdif.write_reg(0x600, 1)       -- RESET, 1
    hwstub.udelay(100000);
    -- delay
    RK27XX.lcdif.write_reg(0x600, 0)       -- RESET, 0
    hwstub.udelay(100000);
    -- delay

    RK27XX.lcdif.write_reg(0x606, 0)       -- IF_ENDIAN, 0
    RK27XX.lcdif.write_reg(1, 0)           -- DRIVER_OUT_CTRL, 0
    RK27XX.lcdif.write_reg(2, 0x100)       -- WAVEFORM_CTRL, 0x100
    RK27XX.lcdif.write_reg(3, 0x1038)--0x10b8)      -- ENTRY_MODE, 0x10b8 (diff)
    RK27XX.lcdif.write_reg(6, 0)           -- SHAPENING_CTRL, 0
    RK27XX.lcdif.write_reg(8, 0x808)       -- DISPLAY_CTRL2, 0x808
    RK27XX.lcdif.write_reg(9, 1)           -- LOW_PWR_CTRL1, 1
    RK27XX.lcdif.write_reg(0x0b, 0x10)     -- LOW_PWR_CTRL2, 0x10
    RK27XX.lcdif.write_reg(0x0c, 0)        -- EXT_DISP_CTRL1, 0
    RK27XX.lcdif.write_reg(0x0f, 0)        -- EXT_DISP_CTRL2, 0
    RK27XX.lcdif.write_reg(0x400, 0x3100)  -- BASE_IMG_SIZE, 0x3100
    RK27XX.lcdif.write_reg(0x401, 1)       -- BASE_IMG_CTRL, 1
    RK27XX.lcdif.write_reg(0x404, 0)       -- VSCROLL_CTRL, 0
    RK27XX.lcdif.write_reg(0x500, 0)       -- PART1_POS, 0
    RK27XX.lcdif.write_reg(0x501, 0)       -- PART1_START, 0
    RK27XX.lcdif.write_reg(0x502, 0x18f)   -- PART1_END, 0x18f
    RK27XX.lcdif.write_reg(0x503, 0)       -- PART2_POS, 0
    RK27XX.lcdif.write_reg(0x504, 0)       -- PART2_START, 0
    RK27XX.lcdif.write_reg(0x505, 0)       -- PART2_END, 0
    RK27XX.lcdif.write_reg(0x10, 0x11)     -- PANEL_IF_CTRL1, 0x11
    -- delay_nops(1000)
    hwstub.udelay(100)
    RK27XX.lcdif.write_reg(0x11, 0x202)    -- PANEL_IF_CTRL2, 0x202
    RK27XX.lcdif.write_reg(0x12, 0x300)    -- PANEL_IF_CTRL3, 0x300
    -- delay_nops(1000)
    hwstub.udelay(100)
    RK27XX.lcdif.write_reg(0x20, 0x21e)    -- PANEL_IF_CTRL4, 0x21e
    RK27XX.lcdif.write_reg(0x21, 0x202)    -- PANEL_IF_CTRL5, 0x202
    RK27XX.lcdif.write_reg(0x22, 0x100)    -- PANEL_IF_CTRL6, 0x100
    RK27XX.lcdif.write_reg(0x90, 0)--0x80d8)   -- FRAME_MKR_CTRL, 0x80d8 (diff)
    RK27XX.lcdif.write_reg(0x92, 0)        -- MDDI_CTRL, 0

    -- gamma calibration
    RK27XX.lcdif.write_reg(0x300, 0x101)
    RK27XX.lcdif.write_reg(0x301, 0x24)
    RK27XX.lcdif.write_reg(0x302, 0x1321)
    RK27XX.lcdif.write_reg(0x303, 0x2613)
    RK27XX.lcdif.write_reg(0x304, 0x2400)
    RK27XX.lcdif.write_reg(0x305, 0x100)
    RK27XX.lcdif.write_reg(0x306, 0x1704)
    RK27XX.lcdif.write_reg(0x307, 0x417)
    RK27XX.lcdif.write_reg(0x308, 7)
    RK27XX.lcdif.write_reg(0x309, 0x101)
    RK27XX.lcdif.write_reg(0x30a, 0xf05)
    RK27XX.lcdif.write_reg(0x30b, 0xf01)
    RK27XX.lcdif.write_reg(0x30c, 0x10f)
    RK27XX.lcdif.write_reg(0x30d, 0x50f)
    RK27XX.lcdif.write_reg(0x30e, 0x501)
    RK27XX.lcdif.write_reg(0x30f, 0x700)

    -- power on sequence
    RK27XX.lcdif.write_reg(7, 1)           -- DISPLAY_CTRL1, 1
    RK27XX.lcdif.write_reg(0x110, 1)       -- PWR_CTRL6, 1
    RK27XX.lcdif.write_reg(0x112, 0x60)    -- PWR_CTRL7, 0x60
    -- delay_nops(5000)
    hwstub.udelay(500)
    RK27XX.lcdif.write_reg(0x100, 0x16b0)  -- PWR_CTRL1, 0x16b0
    -- delay_nops(1000)
    hwstub.udelay(100)
    RK27XX.lcdif.write_reg(0x101, 0x147)   -- PWR_CTRL2, 0x147
    -- delay_nops(1000)
    hwstub.udelay(100)
    RK27XX.lcdif.write_reg(0x102, 0x117)--0x119)   -- PWR_CTRL3, 0x119 (diff)
    -- delay_nops(1000)
    hwstub.udelay(100)
    RK27XX.lcdif.write_reg(0x103, 0x2f00)  -- PWR_CTRL4, 0x2f00
    -- delay_nops(5000)
    hwstub.udelay(500)
    RK27XX.lcdif.write_reg(0x282, 0)--0x87)    -- VCOM_HV2, 0x87 (diff)
    -- delay_nops(1000)
    hwstub.udelay(100)
    RK27XX.lcdif.write_reg(0x102, 0x1be)   -- PWR_CTRL3, 0x1be
    -- delay_nops(1000)
    hwstub.udelay(100)

    -- address setup
    RK27XX.lcdif.write_reg(0x210, 0)
    RK27XX.lcdif.write_reg(0x211, 0xef)
    RK27XX.lcdif.write_reg(0x212, 0)
    RK27XX.lcdif.write_reg(0x213, 0x18f)
    RK27XX.lcdif.write_reg(0x200, 0)
    RK27XX.lcdif.write_reg(0x201, 0)

    -- display on
    RK27XX.lcdif.write_reg(7, 0x21)       -- DISPLAY_CTRL1, 0x21
    -- delay_nops(4000)
    hwstub.udelay(400)
    RK27XX.lcdif.write_reg(7, 0x61)       -- DISPLAY_CTRL1, 0x61
    -- delay_nops(10000)
    hwstub.udelay(1000)
    RK27XX.lcdif.write_reg(7, 0x173)      -- DISPLAY_CTRL1, 0x173
    -- delay_nops(30000)
    hwstub.udelay(3000)

    -- GRAM write
    RK27XX.lcdif.cmd(0x202)               -- GRAM_WRITE

    for x=0, 399, 1 do
        for y=0, 239, 1 do
            RK27XX.lcdif.data(0)
        end
    end
end 
