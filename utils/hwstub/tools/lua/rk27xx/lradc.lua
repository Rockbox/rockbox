RK27XX.adc = {}

function RK27XX.adc.init()
    -- setup ADC clock divider to reach max 1MHz
    HW.SCU.DIVCON1.write(bit32.replace(HW.SCU.DIVCON1.read(), 49, 10, 8))
end

function RK27XX.adc.read(channel)
    HW.ADC.CTRL.write(bit32.bor(bit32.lshift(1,4), bit32.lshift(1,3), bit32.band(channel,3)))
    -- udelay(20)
    return bit32.band(HW.ADC.DATA.read(), 0x3ff)
end
