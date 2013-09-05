---
--- DIGCTL
---
STMP.digctl = {}

local h = HELP:get_topic("STMP"):create_topic("digctl")
h:add("The STMP.digctl table handles the digctl device for all STMPs.")

local hh = h:create_topic("package")
hh:add("The STMP.digctl.package() function returns the name of the package.")
hh:add("The following packages can be returned:")
hh:add("* bga100")
hh:add("* bga169")
hh:add("* tqfp100")
hh:add("* lqfp100")
hh:add("* lqfp128")

function STMP.digctl.package()
    local pack = nil
    if STMP.is_stmp3600() then
        HW.DIGCTL.CTRL.PACKAGE_SENSE_ENABLE.set()
        if HW.DIGCTL.STATUS.PACKAGE_TYPE.read() == 1 then
            pack = "lqfp100"
        else
            pack = "bga169"
        end
        HW.DIGCTL.CTRL.PACKAGE_SENSE_ENABLE.clr()
    elseif STMP.is_stmp3700() or STMP.is_stmp3770() or STMP.is_imx233() then
        local t = HW.DIGCTL.STATUS.PACKAGE_TYPE.read()
        if t == 0 then pack = "bga169"
        elseif t == 1 then pack = "bga100"
        elseif t == 2 then pack = "tqfp100"
        elseif t == 3 then pack = "tqfp128"
        end
    end

    return pack
end

function STMP.digctl.udelay(us)
    local tend = HW.DIGCTL.MICROSECONDS.read() + us
    while HW.DIGCTL.MICROSECONDS.read() < tend do
        
    end
end

function STMP.digctl.mdelay(ms)
    STMP.digctl.udelay(ms * 1000)
end