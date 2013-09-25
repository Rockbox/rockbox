---
--- I2C
---
STMP.i2c = {} 

local h = HELP:get_topic("STMP"):create_topic("i2c")
h:add("The STMP.clkctrl table handles the i2c device for all STMPs.")

function STMP.i2c.init()
    HW.I2C.CTRL0.SFTRST.set()
    STMP.pinctrl.pin(0, 30).muxsel("MAIN")
    STMP.pinctrl.pin(0, 30).pull(true)
    STMP.pinctrl.pin(0, 31).muxsel("MAIN")
    STMP.pinctrl.pin(0, 31).pull(true)
    STMP.i2c.reset()
    STMP.i2c.set_speed(true)
end

function STMP.i2c.reset()
    HW.I2C.CTRL0.SFTRST.set()
    HW.I2C.CTRL0.SFTRST.clr()
    HW.I2C.CTRL0.CLKGATE.clr()
    -- errata for IMX233
    HW.I2C.CTRL1.ACK_MODE.set();
end

function STMP.i2c.set_speed(fast)
    if fast then
        -- Fast-mode @ 400K
        HW.I2C.TIMING0.write(0x000F0007)  -- tHIGH=0.6us, read at 0.3us
        HW.I2C.TIMING1.write(0x001F000F) -- tLOW=1.3us, write at 0.6us
        HW.I2C.TIMING2.write(0x0015000D)
    else
        -- Slow-mode @ 100K
        HW.I2C.TIMING0.write(0x00780030)
        HW.I2C.TIMING1.write(0x00800030)
        HW.I2C.TIMING2.write(0x00300030)
    end
end

function STMP.i2c.transmit(slave_addr, buffer, send_stop)
    local data = { slave_addr }
    for i, v in ipairs(buffer) do
        table.insert(data, v)
    end
    if #data > 4 then
        error("PIO mode cannot send more than 4 bytes at once")
    end
    HW.I2C.CTRL0.MASTER_MODE.set()
    HW.I2C.CTRL0.PIO_MODE.set()
    HW.I2C.CTRL0.PRE_SEND_START.set()
    HW.I2C.CTRL0.POST_SEND_STOP.write(send_stop and 1 or 0)
    HW.I2C.CTRL0.DIRECTION.set()
    HW.I2C.CTRL0.SEND_NAK_ON_LAST.clr()
    HW.I2C.CTRL0.XFER_COUNT.write(#data)
    local v = 0
    for i,d in ipairs(data) do
        v = v + bit32.lshift(d, (i - 1) * 8)
    end
    HW.I2C.DATA.write(v)
    HW.I2C.CTRL1.clr(0xffff)
    HW.I2C.CTRL0.RUN.set()
    while HW.I2C.CTRL0.RUN.read() == 1 do
    end
    if HW.I2C.CTRL1.NO_SLAVE_ACK_IRQ.read() == 1 then
        HW.I2C.CTRL1.CLR_GOT_A_NAK.set()
        STMP.i2c.reset()
        return false
    end
    if HW.I2C.CTRL1.EARLY_TERM_IRQ.read() == 1 or HW.I2C.CTRL1.MASTER_LOSS_IRQ.read() == 1 then
        return false
    else
        return true
    end
end