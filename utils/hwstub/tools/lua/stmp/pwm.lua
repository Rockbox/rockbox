--
-- LCDIF
--

STMP.pwm = {}

function STMP.pwm.init()
    HW.LCDIF.CTRL.SFTRST.clr()
    HW.LCDIF.CTRL.CLKGATE.clr()
end 

function STMP.pwm.enable(chan, en)
    if en then
        HW.PWM.CTRL.set(bit32.lshift(1, chan))
    else
        HW.PWM.CTRL.clr(bit32.lshift(1, chan))
    end
end

function STMP.pwm.setup(channel, period, cdiv, active, active_state, inactive, inactive_state)
    -- stop
    STMP.pwm.enable(channel, false)
    -- setup pin
    --FIXME
    -- watch the order ! active THEN period
    -- NOTE: the register value is period-1
    HW.PWM.ACTIVEn[channel].ACTIVE.write(active)
    HW.PWM.ACTIVEn[channel].INACTIVE.write(inactive)
    HW.PWM.PERIODn[channel].PERIOD.write(period - 1)
    HW.PWM.PERIODn[channel].ACTIVE_STATE.write(active_state)
    HW.PWM.PERIODn[channel].INACTIVE_STATE.write(inactive_state)
    HW.PWM.PERIODn[channel].CDIV.write(cdiv)
    -- restore
    STMP.pwm.enable(channel, true)
end