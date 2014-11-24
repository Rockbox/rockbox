--
-- LCDIF
--
RK27XX.lcdif = {}
-- 18bit setup only

function RK27XX.lcdif.iomux()
    IOMUX_LCD_VSYNC = bit32.lshift(1,11)
    IOMUX_LCD_DEN = bit32.lshift(1,10)
    IOMUX_LCD_D18 = bit32.lshift(1,4)

    IOMUX_LCD_D20 = bit32.lshift(1,6)
    IOMUX_LCD_D22 = bit32.lshift(1,8)
    IOMUX_LCD_D17 = bit32.lshift(1,2)
    IOMUX_LCD_D16 = bit32.lshift(1,0)

    IOMUX_LCD_D815 = bit32.lshift(1,15)

    muxa = bit32.band(HW.SCU.IOMUXA_CON.read(), bit32.bnot(bit32.bor(IOMUX_LCD_VSYNC,IOMUX_LCD_DEN, 0xff)))
    muxa = bit32.bor(muxa, bit32.bor(IOMUX_LCD_D18,IOMUX_LCD_D20,IOMUX_LCD_D22,IOMUX_LCD_D17,IOMUX_LCD_D16))
    HW.SCU.IOMUXA_CON.write(muxa)
    HW.SCU.IOMUXB_CON.write(bit32.replace(HW.SCU.IOMUXB_CON.read(),1, 15))
end

-- start in bypass mode
function RK27XX.lcdif.lcdctrl_init()
    HW.LCDC.LCDC_CTRL.write(bit32.bor(bit32.lshift(7,9), bit32.lshift(1,0), bit32.lshift(1,6)))
    HW.LCDC.MCU_CTRL.write(bit32.bor(bit32.lshift(0x3f,8), bit32.lshift(1,0)))
    HW.LCDC.VERT_PERIOD.write(bit32.bor(bit32.lshift(1,7), bit32.lshift(1,5), 1))
end

function RK27XX.lcdif.data_transform(data)
    r = bit32.lshift(bit32.band(data, 0x0000fc00), 8)
    g = bit32.bor(bit32.lshift(bit32.band(data, 0x00000300), 6), bit32.lshift(bit32.band(data, 0x000000e0), 5))
    b = bit32.lshift(bit32.band(data, 0x00000001f), 3)

    return bit32.bor(r,g,b)
end

function RK27XX.lcdif.cmd(cmd)
    HW.LCDC.LCD_COMMAND.write(RK27XX.lcdif.data_transform(cmd))
end

function RK27XX.lcdif.data(data)
    HW.LCDC.LCD_DATA.write(RK27XX.lcdif.data_transform(data))
end

function RK27XX.lcdif.write_reg(reg, val)
    RK27XX.lcdif.cmd(reg)
    RK27XX.lcdif.data(val)
end
