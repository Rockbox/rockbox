ATJ.lcm = {}

function ATJ.lcm.wait_fifo_empty()
    while (bit32.band(HW.YUV2RGB.CTL.read(), 0x04) == 0) do
    end
end

function ATJ.lcm.rs_command()
    ATJ.lcm.wait_fifo_empty()
    HW.YUV2RGB.CTL.write(0x802ae)
end

function ATJ.lcm.rs_data()
    ATJ.lcm.wait_fifo_empty()
    HW.YUV2RGB.CTL.write(0x902ae)
end

function ATJ.lcm.fb_data()
    ATJ.lcm.rs_command()
    HW.YUV2RGB.FIFODATA.write(0x22)
    HW.YUV2RGB.CTL.write(0xa02ae)
end

function ATJ.lcm.init()
    HW.CMU.DEVCLKEN.write(bit32.bor(HW.CMU.DEVCLKEN.read(), 0x102))
    ATJ.gpio.muxsel("LCM")
    hwstub.udelay(1)
    ATJ.lcm.rs_command()
    HW.YUV2RGB.CLKCTL.write(0x102)
end

