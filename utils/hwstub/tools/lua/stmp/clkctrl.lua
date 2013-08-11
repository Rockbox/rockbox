---
--- DIGCTL
---
STMP.clkctrl = {} 

local h = HELP:get_topic("STMP"):create_topic("clkctrl")
h:add("The STMP.clkctrl table handles the clkctrl device for all STMPs.")

local hh = h:create_topic("list_all")
hh:add("The STMP.clkctrl.list_all() function returns the list of all clocks names.")

local hh = h:create_topic("is_enabled")
hh:add("The STMP.clkctrl.is_enabled(clk) function returns the state of the clock gate or true is there is none.")
hh:add("Note that some clock is disabled though other means than a clock gate (divider gate).")

local hh = h:create_topic("is_enabled")
hh:add("The STMP.clkctrl.is_enabled(clk) function returns the state of the clock gate or true is there is none.")
hh:add("Note that some clock is disabled though other means than a clock gate (divider gate).")

local hh = h:create_topic("get_div")
hh:add("The STMP.clkctrl.get_div(clk) function returns the integer divider of the clock or 1 if there is none.")

local hh = h:create_topic("get_frac_div")
hh:add("The STMP.clkctrl.get_frac_div(clk) function returns the fractional divider of the clock gate or 1 is there is none.")
hh:add("Note that the effect of a fractional divider might depend on other fields or on the clock itself.")

local hh = h:create_topic("get_bypass")
hh:add("The STMP.clkctrl.get_bypass(clk) function returns the state of the PLL bypass of the clock or false if there is none.")

local hh = h:create_topic("get_freq")
hh:add("The STMP.clkctrl.get_frac(clk) function returns the frequency of the clock in HZ, or 0 if it is disabled.")

function STMP.clkctrl.list_all()
    local list = {"clk_pll", "clk_xtal", "clk_io", "clk_cpu", "clk_hbus", "clk_ssp",
        "clk_emi", "clk_xbus", "clk_filt", "clk_dri", "clk_pwm", "clk_timrot",
        "clk_uart"}
    if hwstub.dev.stmp.chipid >= 0x3700 then
        table.insert(list, "clk_pix")
    end
    return list
end

function STMP.clkctrl.is_enabled(clk)
    if clk == "clk_pll" then return HW.CLKCTRL.PLLCTRL0.POWER.read() == 1
    elseif clk == "clk_pix" then return HW.CLKCTRL.PIX.CLKGATE.read() == 0
    elseif clk == "clk_ssp" then return HW.CLKCTRL.SSP.CLKGATE.read() == 0
    elseif clk == "clk_dri" then return HW.CLKCTRL.XTAL.DRI_CLK24M_GATE.read() == 0
    elseif clk == "clk_pwm" then return HW.CLKCTRL.XTAL.PWM_CLK24M_GATE.read() == 0
    elseif clk == "clk_uart" then return HW.CLKCTRL.XTAL.UART_CLK24M_GATE.read() == 0
    elseif clk == "clk_filt" then return HW.CLKCTRL.XTAL.FILT_CLK24M_GATE.read() == 0
    elseif clk == "clk_timrot" then return HW.CLKCTRL.XTAL.TIMROT_CLK24M_GATE.read() == 0
    else return true end
end

function STMP.clkctrl.get_div(clk)
    if hwstub.dev.stmp.chipid >= 0x3700 then
        if clk == "clk_pix" then return HW.CLKCTRL.PIX.DIV.read()
        elseif clk == "clk_cpu" then return HW.CLKCTRL.CPU.DIV_CPU.read()
        elseif clk == "clk_emi" then return HW.CLKCTRL.EMI.DIV_EMI.read() end
    else
        if clk == "clk_cpu" then return HW.CLKCTRL.CPU.DIV.read()
        elseif clk == "clk_emi" then return HW.CLKCTRL.EMI.DIV.read() end
    end
    if clk == "clk_ssp" then return HW.CLKCTRL.SSP.DIV.read()
    elseif clk == "clk_hbus" then return HW.CLKCTRL.HBUS.DIV.read()
    elseif clk == "clk_xbus" then return HW.CLKCTRL.XBUS.read()
    else return 0 end
end

function STMP.clkctrl.get_frac_div(clk)
    local name = nil
    if hwstub.dev.stmp.chipid >= 0x3700 and clk == "clk_pix" then name = "PIX"
    elseif clk == "clk_io" then name = "IO"
    elseif clk == "clk_cpu" then name = "CPU"
    elseif clk == "clk_emi" then name = "EMI"
    else return 0 end

    if name == nil then return 0 end
    if HW.CLKCTRL.FRAC["CLKGATE" .. name].read() == 1 then return 0
    else return HW.CLKCTRL.FRAC[name .. "FRAC"].read() end
end

function STMP.clkctrl.get_bypass(clk)
    if hwstub.dev.stmp.chipid >= 0x3700 and clk == "clk_pix" then return HW.CLKCTRL.CLKSEQ.BYPASS_PIX.read() == 1
    elseif clk == "clk_ssp" then return HW.CLKCTRL.CLKSEQ.BYPASS_SSP.read() == 1
    elseif clk == "clk_cpu" then return HW.CLKCTRL.CLKSEQ.BYPASS_CPU.read() == 1
    elseif clk == "clk_emi" then return HW.CLKCTRL.CLKSEQ.BYPASS_EMI.read() == 1
    else return false end
end

function STMP.clkctrl.get_freq(clk)
    if clk == "clk_pll" then
        return STMP.clkctrl.is_enabled(clk) and 480000000 or 0
    elseif clk == "clk_xtal" then return 24000000
    elseif clk == "clk_cpu" then
        if hwstub.dev.stmp.chipid >= 0x3700 then
            if STMP.clkctrl.get_bypass(clk) then
                local ref = STMP.clkctrl.get_freq("clk_xtal")
                if HW.CLKCTRL.CPU.DIV_XTAL_FRAC_EN.read() == 1 then
                    return ref * HW.CLKCTRL.CPU.DIV_XTAL.read() / 32
                else
                    return ref / STMP.clkctrl.get_div(clk)
                end
            else
                local ref = STMP.clkctrl.get_freq("clk_pll")
                if STMP.clkctrl.get_frac_div(clk) ~= 0 then
                    ref = ref * 18 / STMP.clkctrl.get_frac_div(clk)
                end
                return ref / STMP.clkctrl.get_div(clk)
            end
        else
            return STMP.CLKCTRL.get_freq("clk_pll") / STMP.clkctrl.get_div(clk)
        end
    elseif clk == "clk_hbus" then
        local ref = STMP.clkctrl.get_freq("clk_cpu")
        if hwstub.dev.stmp.chipid >= 0x3700 and STMP.clkctrl.get_frac_div(clk) ~= 0 then
            ref = ref * STMP.clkctrl.get_frac_div(clk) / 32
        end
        if STMP.clkctrl.get_div(clk) ~= 0 then
            ref = ref / STMP.clkctrl.get_div(clk)
        end
        return ref
    elseif clk == "clk_io" then
        local ref = STMP.clkctrl.get_freq("clk_pll")
        if hwstub.dev.stmp.chipid >= 0x3700 and STMP.clkctrl.get_frac_div(clk) ~= 0 then
            ref = ref * 18 / STMP.clkctrl.get_frac_div(clk)
        end
        return ref
    elseif hwstub.dev.stmp.chipid >= 0x3700 and clk == "clk_pix" then
        local ref = nil
        if not STMP.clkctrl.is_enabled(clk) then
            ref = 0
        elseif STMP.clkctrl.get_bypass(clk) then
            ref = STMP.clkctrl.get_freq("clk_xtal")
        else
            ref = STMP.clkctrl.get_freq("clk_pll")
            if STMP.clkctrl.get_frac_div(clk) ~= 0 then
                ref = ref * 18 / STMP.clkctrl.get_frac_div(clk)
            else
                ref = 0
            end
        end
        return ref / STMP.clkctrl.get_div(clk)
    elseif clk == "clk_ssp" then
        local ref = nil
        if not STMP.clkctrl.is_enabled(clk) then
            ref = 0
        elseif hwstub.dev.stmp.chipid >= 0x3700 and STMP.clkctrl.get_bypass(clk) then
            ref = STMP.clkctrl.get_freq("clk_xtal")
        else
            ref = STMP.clkctrl.get_freq("clk_io")
        end
        return ref / STMP.clkctrl.get_div(clk)
    elseif clk == "clk_emi" then
        if hwstub.dev.stmp.chipid >= 0x3700 then
            if STMP.clkctrl.get_bypass("clk_emi") then
                if HW.CLKCTRL.EMI.CLKGATE.read() then return 0
                else return STMP.clkctrl.get_freq("clk_xtal") / HW.CLKCTRL.EMI.DIV_XTAL.read() end
            else
                local ref = STMP.clkctrl.get_freq("clk_pll")
                if STMP.clkctrl.get_frac_div(clk) ~= 0 then
                    ref = ref * 18 / STMP.clkctrl.get_frac_div(clk)
                end
                return ref / STMP.clkctrl.get_div(clk)
            end
        else
            return STMP.clkctrl.get_freq("clk_pll") / STMP.clkctrl.get_div(clk);
        end
    elseif clk == "clk_xbus" then
        return STMP.clkctrl.get_freq("clk_xtal") / STMP.clkctrl.get_div(clk)
    else
        return 0
    end
end
