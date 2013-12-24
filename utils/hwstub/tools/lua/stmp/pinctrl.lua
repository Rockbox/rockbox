---
--- PINCTRL
--- 
STMP.pinctrl = {}

local h = HELP:get_topic("STMP"):create_topic("pinctrl")
h:add("The STMP.pinctrl table handles the pinctrl device for all STMPs.")
h:add("It provides a simple abstraction to set individual pins.")

local hh = h:create_topic("pin")
hh:add("The STMP.pinctrl.pin(x,yy) function returns a table for the pin BxPyy.")
hh:add("Depending on the STMP family, the following function might be available.")
hh:add("* read() returns the value of the pin (the pin does not need to be configured as GPIO)")
hh:add("* write(x) sets the value of the pin (provided it is a GPIO with output enabled)")
hh:add("* set() is equivalent to write(1)")
hh:add("* clr() is equivalent to write(0)")
hh:add("* enable() enables the gpio output (provided it is configured as GPIO).")
hh:add("* disable() disables the gpio output (provided it is configured as GPIO)")
hh:add("* muxsel(x) set pin function to x, which can be an integer or one of: MAIN, ALT1, ALT2, GPIO")

function STMP.pinctrl.pin(bank,pin)
    local t = {
        read = function()
            return bit32.extract(HW.PINCTRL.DINn[bank].read(), pin)
        end,

        write = function(val)
            if val then t.set() else t.clr() end
        end,

        set = function()
            HW.PINCTRL.DOUTn[bank].set(bit32.lshift(1, pin))
        end,

        clr = function()
            HW.PINCTRL.DOUTn[bank].clr(bit32.lshift(1, pin))
        end,

        enable = function()
             HW.PINCTRL.DOEn[bank].set(bit32.lshift(1, pin))
        end,

        disable = function()
             HW.PINCTRL.DOEn[bank].clr(bit32.lshift(1, pin))
        end,

        muxsel = function(x)
            if type(x) == "string" then
                if x == "MAIN" then x = 0
                elseif x == "ALT1" then x = 1
                elseif x == "ALT2" then x = 2
                elseif x == "GPIO" then x = 3
                else error("Invalid muxsel string " .. x) end
            end
            local v = nil
            if STMP.is_stmp3600() then
                if pin < 16 then
                    v = HW.PINCTRL.MUXSELLn[bank]
                else
                    v = HW.PINCTRL.MUXSELHn[bank]
                end
            else
                v = HW.PINCTRL.MUXSELn[2 * bank + math.floor(pin / 16)]
            end
            v.write(bit32.replace(v.read(), x, (pin % 16) * 2, 2))
        end,

        pull = function(val)
            if val then
                HW.PINCTRL.PULLn[bank].set(bit32.lshift(1, pin))
            else
                HW.PINCTRL.PULLn[bank].clr(bit32.lshift(1, pin))
            end
        end,
    }
    return t
end

hh = h:create_topic("configure")
hh:add("The STMP.pinctrl.configure(tbl) function configures pins according to a table.")
hh:add("The table must contain a list of subtable, each corresponding to a pin.")
hh:add("Each subtable can configure one or more aspects of a specific pin.")
hh:add("The following characteristics are understood:")
hh:add("* bank: pin bank (mandatory)")
hh:add("* pin: pin number (mandatory) ")
hh:add("* muxsel: same values as STMP.pinctrl.pin().muxsel (optional)")
hh:add("* enable: enable/disable output (optional)")
hh:add("* output: set/clear output (optional)")
hh:add("* pull: enable/disable pullup (optional)")
hh:add("All non-subtable entries are ignored")
hh:add("All unknown parameters in subtablkes are ignored")
hh:add("")
hh:add("Example:")
hh:add("STMP.pinctrl.configure({{bank=0,pin=1,muxsel=\"GPIO\",enable=false},{bank=0,pin=2,muxsel=\"MAIN\"}})")

function STMP.pinctrl.configure(tbl)
    if type(tbl) ~= "table" then error("Parameter must be a table") end
    for i,v in pairs(tbl) do
        if type(v) == "table" then
            if v.bank ~= nil and v.pin ~= nil then
                local pin = STMP.pinctrl.pin(v.bank,v.pin)
                if v.muxsel ~= nil then
                    STMP.debug(string.format("cfg B%dP%02d muxsel %s", v.bank, v.pin, v.muxsel))
                    pin.muxsel(v.muxsel)
                end
                if v.enable ~= nil then
                    STMP.debug(string.format("cfg B%dP%02d enable %s", v.bank, v.pin, v.enable))
                    if v.enable then pin.enable()
                    else pin.disable() end
                end
                if v.output ~= nil then
                    STMP.debug(string.format("cfg B%dP%02d output %s", v.bank, v.pin, v.output))
                    if v.output then pin.set()
                    else pin.clr() end
                end
                if v.pull ~= nil then
                    STMP.debug(string.format("cfg B%dP%02d pull %s", v.bank, v.pin, v.pull))
                    pin.pull(v.pull)
                end
            end
        end
    end
end

hh = h:create_topic("configure_ex")
hh:add("The STMP.pinctrl.configure_ex(tbl) function configures pins according to a table.")
hh:add("It is an enhanced version of configure which handles the different families and packages.")
hh:add("The argument must be a table with one entry per STMP family.")
hh:add("Each entry must a subtable with one subentry per package.")
hh:add("Each subentry must be a valid argument to STMP.pinctrl.configure().")
hh:add("The family names are the one returned by STMP.family.")
hh:add("The table can also contain an entry  \"all\" which apply to all families")
hh:add("The package names are the one returned by STMP.digctl.package().")
hh:add("The table can also contain an entry  \"all\" which apply to all packages.")

function STMP.pinctrl.configure_ex(tbl)
    if type(tbl) ~= "table" then error("Parameter must be a table") end
    local pack = STMP.digctl.package()
    if tbl[STMP.family] ~= nil then
        if tbl[STMP.family][pack] ~= nil then STMP.pinctrl.configure(tbl[STMP.family][pack]) end
        if tbl[STMP.family]["all"] ~= nil then STMP.pinctrl.configure(tbl[STMP.family]["all"]) end
    end
    if tbl["all"] ~= nil then
        if tbl["all"][pack] ~= nil then STMP.pinctrl.configure(tbl["all"][pack]) end
        if tbl["all"]["all"] ~= nil then STMP.pinctrl.configure(tbl["all"]["all"]) end
    end
end

hh = h:create_topic("lcdif")
hh:add("The STMP.pinctrl.lcdif tables provides some high level routines to configure the pins.")
hh:add("It is specialised to configure the LCDIF pins correctly.")
hh:add("Some of the modes may not be available on all STMP families.")

STMP.pinctrl.lcdif = {}

local hhh = hh:create_topic("setup_system")
hhh:add("The STMP.pinctrl.lcdif.setup_system(bus_width,busy) functions configure the LCDIF pins.")
hhh:add("It only take cares of the pins used in system mode and for a specified bus width.")
hhh:add("The handled bus widths are 8, 16 and 18")
hhh:add("The busy pin is configured only if busy is true")

function STMP.pinctrl.lcdif.setup_system(bus_width, busy)
    local common =
    {
        stmp3600 =
        {
            all =
            {
                lcd_reset = { bank = 1, pin = 16, muxsel = "MAIN"},
                lcd_rs = { bank = 1, pin = 17, muxsel = "MAIN"},
                lcd_wr = { bank = 1, pin = 18, muxsel = "MAIN"},
                lcd_cs = { bank = 1, pin = 19, muxsel = "MAIN"},
                lcd_d0 = {bank = 1, pin = 0, muxsel = "MAIN"},
                lcd_d1 = {bank = 1, pin = 1, muxsel = "MAIN"},
                lcd_d2 = {bank = 1, pin = 2, muxsel = "MAIN"},
                lcd_d3 = {bank = 1, pin = 3, muxsel = "MAIN"},
                lcd_d4 = {bank = 1, pin = 4, muxsel = "MAIN"},
                lcd_d5 = {bank = 1, pin = 5, muxsel = "MAIN"},
                lcd_d6 = {bank = 1, pin = 6, muxsel = "MAIN"},
                lcd_d7 = {bank = 1, pin = 7, muxsel = "MAIN"}
            }
        },
        stmp3700 =
        {
            all =
            {
                lcd_reset = { bank = 1, pin = 16, muxsel = "MAIN"},
                lcd_rs = { bank = 1, pin = 17, muxsel = "MAIN"},
                lcd_wr = { bank = 1, pin = 18, muxsel = "MAIN"},
                lcd_rd = { bank = 1, pin = 19, muxsel = "MAIN"},
                lcd_cs = { bank = 1, pin = 20, muxsel = "MAIN"},
                lcd_d0 = {bank = 1, pin = 0, muxsel = "MAIN"},
                lcd_d1 = {bank = 1, pin = 1, muxsel = "MAIN"},
                lcd_d2 = {bank = 1, pin = 2, muxsel = "MAIN"},
                lcd_d3 = {bank = 1, pin = 3, muxsel = "MAIN"},
                lcd_d4 = {bank = 1, pin = 4, muxsel = "MAIN"},
                lcd_d5 = {bank = 1, pin = 5, muxsel = "MAIN"},
                lcd_d6 = {bank = 1, pin = 6, muxsel = "MAIN"},
                lcd_d7 = {bank = 1, pin = 7, muxsel = "MAIN"}
            }
        },
        imx233 =
        {
            all =
            {
                lcd_reset = { bank = 1, pin = 18, muxsel = "MAIN"},
                lcd_rs = { bank = 1, pin = 19, muxsel = "MAIN"},
                lcd_wr = { bank = 1, pin = 20, muxsel = "MAIN"},
                lcd_cs = { bank = 1, pin = 21, muxsel = "MAIN"},
                lcd_d0 = {bank = 1, pin = 0, muxsel = "MAIN"},
                lcd_d1 = {bank = 1, pin = 1, muxsel = "MAIN"},
                lcd_d2 = {bank = 1, pin = 2, muxsel = "MAIN"},
                lcd_d3 = {bank = 1, pin = 3, muxsel = "MAIN"},
                lcd_d4 = {bank = 1, pin = 4, muxsel = "MAIN"},
                lcd_d5 = {bank = 1, pin = 5, muxsel = "MAIN"},
                lcd_d6 = {bank = 1, pin = 6, muxsel = "MAIN"},
                lcd_d7 = {bank = 1, pin = 7, muxsel = "MAIN"}
            }
        }
    }
    local bus8_15 =
    {
        stmp3600 =
        {
            all =
            {
                lcd_d8 = {bank = 1, pin = 8, muxsel = "MAIN"},
                lcd_d9 = {bank = 1, pin = 9, muxsel = "MAIN"},
                lcd_d10 = {bank = 1, pin = 10, muxsel = "MAIN"},
                lcd_d11 = {bank = 1, pin = 11, muxsel = "MAIN"},
                lcd_d12 = {bank = 1, pin = 12, muxsel = "MAIN"},
                lcd_d13 = {bank = 1, pin = 13, muxsel = "MAIN"},
                lcd_d14 = {bank = 1, pin = 14, muxsel = "MAIN"},
                lcd_d15 = {bank = 1, pin = 15, muxsel = "MAIN"}
            }
        },
        stmp3700 =
        {
            all =
            {
                lcd_d8 = {bank = 1, pin = 8, muxsel = "MAIN"},
                lcd_d9 = {bank = 1, pin = 9, muxsel = "MAIN"},
                lcd_d10 = {bank = 1, pin = 10, muxsel = "MAIN"},
                lcd_d11 = {bank = 1, pin = 11, muxsel = "MAIN"},
                lcd_d12 = {bank = 1, pin = 12, muxsel = "MAIN"},
                lcd_d13 = {bank = 1, pin = 13, muxsel = "MAIN"},
                lcd_d14 = {bank = 1, pin = 14, muxsel = "MAIN"},
                lcd_d15 = {bank = 1, pin = 15, muxsel = "MAIN"}
            }
        },
        imx233 =
        {
            all =
            {
                lcd_d8 = {bank = 1, pin = 8, muxsel = "MAIN"},
                lcd_d9 = {bank = 1, pin = 9, muxsel = "MAIN"},
                lcd_d10 = {bank = 1, pin = 10, muxsel = "MAIN"},
                lcd_d11 = {bank = 1, pin = 11, muxsel = "MAIN"},
                lcd_d12 = {bank = 1, pin = 12, muxsel = "MAIN"},
                lcd_d13 = {bank = 1, pin = 13, muxsel = "MAIN"},
                lcd_d14 = {bank = 1, pin = 14, muxsel = "MAIN"},
                lcd_d15 = {bank = 1, pin = 15, muxsel = "MAIN"}
            }
        }
    }
    local bus16_17 =
    {
        imx233 =
        {
            all =
            {
                lcd_d16 = {bank = 1, pin = 16, muxsel = "MAIN"},
                lcd_d17 = {bank = 1, pin = 17, muxsel = "MAIN"},
            }
        }
    }
    local busy_pin =
    {
        stmp3600 =
        {
            all =
            {
                lcd_busy = { bank = 1, pin = 21, muxsel = "MAIN"},
            }
        },
        stmp3700 =
        {
            all =
            {
                lcd_busy = { bank = 1, pin = 21, muxsel = "MAIN"},
            }
        },
    }

    STMP.pinctrl.configure_ex(common)
    if bus_width > 8 then
        STMP.pinctrl.configure_ex(bus8_15)
    end
    if bus_width > 16 then
        STMP.pinctrl.configure_ex(bus16_17)
    end
    if busy then
        STMP.pinctrl.configure_ex(busy_pin)
    end
end

STMP.pinctrl.i2c = {}

local hhh = hh:create_topic("setup")
hhh:add("The STMP.pinctrl.i2c.setup() functions configure the I2C pins.")

function STMP.pinctrl.i2c.setup()
    local pins =
    {
        stmp3700 =
        {
            all =
            {
                i2c_scl = { bank = 2, pin = 5, muxsel = "MAIN", pull = true},
                i2c_sda = { bank = 2, pin = 6, muxsel = "MAIN", pull = true}
            }
        },
        imx233 =
        {
            all =
            {
                i2c_scl = { bank = 0, pin = 30, muxsel = "MAIN", pull = true},
                i2c_sda = { bank = 0, pin = 31, muxsel = "MAIN", pull = true}
            }
        }
    }

    STMP.pinctrl.configure_ex(pins)
end
