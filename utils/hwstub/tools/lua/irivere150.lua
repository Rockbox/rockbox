E150 = {}

function E150.lcd_reg_write(reg, val)
    ATJ.lcm.rs_command()
    HW.YUV2RGB.FIFODATA.write(reg)
    ATJ.lcm.rs_data()
    HW.YUV2RGB.FIFODATA.write(val)
end

function E150.lcd_init()
    ATJ.lcm.init()

    ATJ.gpio.outen("PORTA", 16, 1)
    ATJ.gpio.set("PORTA", 16 , 1)
    hwstub.mdelay(10)
    ATJ.gpio.set("PORTA", 16, 0)
    hwstub.mdelay(10)
    ATJ.gpio.set("PORTA", 16, 1)
    hwstub.mdelay(10)

    -- lcd controller init sequence matching HX8347-D
    E150.lcd_reg_write(0xea, 0x00)
    E150.lcd_reg_write(0xeb, 0x20)
    E150.lcd_reg_write(0xec, 0x0f)
    E150.lcd_reg_write(0xed, 0xc4)
    E150.lcd_reg_write(0xe8, 0xc4)
    E150.lcd_reg_write(0xe9, 0xc4)
    E150.lcd_reg_write(0xf1, 0xc4)
    E150.lcd_reg_write(0xf2, 0xc4)
    E150.lcd_reg_write(0x27, 0xc4)
    E150.lcd_reg_write(0x40, 0x00) -- gamma block start
    E150.lcd_reg_write(0x41, 0x00)
    E150.lcd_reg_write(0x42, 0x01)
    E150.lcd_reg_write(0x43, 0x13)
    E150.lcd_reg_write(0x44, 0x10)
    E150.lcd_reg_write(0x45, 0x26)
    E150.lcd_reg_write(0x46, 0x08)
    E150.lcd_reg_write(0x47, 0x51)
    E150.lcd_reg_write(0x48, 0x02)
    E150.lcd_reg_write(0x49, 0x12)
    E150.lcd_reg_write(0x4a, 0x18)
    E150.lcd_reg_write(0x4b, 0x19)
    E150.lcd_reg_write(0x4c, 0x14)
    E150.lcd_reg_write(0x50, 0x19)
    E150.lcd_reg_write(0x51, 0x2f)
    E150.lcd_reg_write(0x52, 0x2c)
    E150.lcd_reg_write(0x53, 0x3e)
    E150.lcd_reg_write(0x54, 0x3f)
    E150.lcd_reg_write(0x55, 0x3f)
    E150.lcd_reg_write(0x56, 0x2e)
    E150.lcd_reg_write(0x57, 0x77)
    E150.lcd_reg_write(0x58, 0x0b)
    E150.lcd_reg_write(0x59, 0x06)
    E150.lcd_reg_write(0x5a, 0x07)
    E150.lcd_reg_write(0x5b, 0x0d)
    E150.lcd_reg_write(0x5c, 0x1d)
    E150.lcd_reg_write(0x5d, 0xcc) -- gamma block end
    E150.lcd_reg_write(0x1b, 0x1b)
    E150.lcd_reg_write(0x1a, 0x01)
    E150.lcd_reg_write(0x24, 0x2f)
    E150.lcd_reg_write(0x25, 0x57)
    E150.lcd_reg_write(0x23, 0x86)
    E150.lcd_reg_write(0x18, 0x36) -- 70Hz framerate
    E150.lcd_reg_write(0x19, 0x01) -- osc enable
    E150.lcd_reg_write(0x01, 0x00)
    E150.lcd_reg_write(0x1f, 0x88)
    hwstub.mdelay(5)
    E150.lcd_reg_write(0x1f, 0x80)
    hwstub.mdelay(5)
    E150.lcd_reg_write(0x1f, 0x90)
    hwstub.mdelay(5)
    E150.lcd_reg_write(0x1f, 0xd0)
    hwstub.mdelay(5)
    E150.lcd_reg_write(0x17, 0x05) -- 16bpp
    E150.lcd_reg_write(0x36, 0x00)
    E150.lcd_reg_write(0x28, 0x38)
    hwstub.mdelay(40)
    E150.lcd_reg_write(0x28, 0x3c)

    E150.lcd_reg_write(0x02, 0x00) -- column start MSB
    E150.lcd_reg_write(0x03, 0x00) -- column start LSB
    E150.lcd_reg_write(0x04, 0x00) -- column end MSB
    E150.lcd_reg_write(0x05, 0xef) -- column end LSB
    E150.lcd_reg_write(0x06, 0x00) -- row start MSB
    E150.lcd_reg_write(0x07, 0x00) -- row start LSB
    E150.lcd_reg_write(0x08, 0x01) -- row end MSB
    E150.lcd_reg_write(0x09, 0x3f) -- row end LSB

    ATJ.lcm.fb_data() -- prepare for write to fifo

    -- clear lcd gram
    for y=0, 319 do
        for x=0, 239/2 do
            HW.YUV2RGB.FIFODATA.write(0)
        end
    end

end

function E150.set_backlight(val)
    local fmclk = HW.CMU.FMCLK.read()
    fmclk = bit32.band(fmclk, bit32.bnot(0x1c))
    fmclk = bit32.bor(fmclk, 0x22)
    HW.CMU.FMCLK.write(fmclk)

    HW.PMU.CTL.write(bit32.bor(HW.PMU.CTL.read(), 0x8000))
    local chg = HW.PMU.CHG.read()
    chg = bit32.band(chg, bit32.bnot(0x1f00))
    chg = bit32.bor(chg, bit32.bor(0xc000, bit32.lshift(val, 8)))
    HW.PMU.CHG.write(chg)
end

function E150.init()
    E150.lcd_init()
    E150.set_backlight(24);
end
