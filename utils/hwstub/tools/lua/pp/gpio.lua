---
--- GPIO
--- 
PP.gpio = {}

local h = HELP:get_topic("PP"):create_topic("gpio")
h:add("TODO")

local hh = h:create_topic("pin")
hh:add("TODO")

function PP.gpio.pin(bank,pin)
    if type(bank) == "string" then
        if string.len(bank) ~= 1 then
            error("Invalid bank " .. bank)
        end
        bank = string.byte(bank)
        if bank < string.byte("A") or bank > string.byte("Z") then
            error("Invalid bank " .. bank)
        end
        bank = bank - string.byte("A")
    end
    if pin < 0 or pin >= 8 then
        error("invalid pin " .. pin)
    end
    PP.debug(string.format("gpio: get pin B%dP%d", bank, pin))
    local t = {
        read = function()
            return bit32.extract(HW.GPIO.INPUT_VALn[bank].read(), pin)
        end,

        write = function(val)
            local v = HW.GPIO.OUTPUT_VALn[bank].read()
            v = bit32.replace(v, val and 1 or 0, pin)
            HW.GPIO.OUTPUT_VALn[bank].write(v)
        end,

        enable = function(val)
            if val == nil then
                val = false
            end
            local v = HW.GPIO.ENABLEn[bank].read()
            v = bit32.replace(v, val and 1 or 0, pin)
            HW.GPIO.ENABLEn[bank].write(v)
        end,

        output_enable = function(val)
            if val == nil then
                val = false
            end
            local v = HW.GPIO.OUTPUT_ENn[bank].read()
            v = bit32.replace(v, val and 1 or 0, pin)
            HW.GPIO.OUTPUT_ENn[bank].write(v)
        end,

        muxsel = function(x)
            
        end,

        pull = function(val)
            
        end,
    }
    return t
end 
