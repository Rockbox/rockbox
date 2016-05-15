---
--- GPIO
--- 
JZ.gpio = {}


function JZ.gpio.pinmask(bank, mask)
    local t = {}
    t.read = function()
        return bit32.band(HW.GPIO.IN[bank].read(), mask)
    end

    t.write = function(val)
        if val then t.set() else t.clr() end
    end

    t.set = function()
        HW.GPIO.OUT[bank].SET.write(mask)
    end

    t.clr = function()
        HW.GPIO.OUT[bank].CLR.write(mask)
    end

    t.gpio = function()
        HW.GPIO.FUNCTION[bank].CLR.write(mask)
        HW.GPIO.SELECT[bank].CLR.write(mask)
    end

    t.dir = function(out)
        if out then
            HW.GPIO.DIR[bank].SET.write(mask)
        else
            HW.GPIO.DIR[bank].CLR.write(mask)
        end
    end

    t.pull = function(val)
        if val then
            HW.GPIO.PULL[bank].CLR.write(mask)
        else
            HW.GPIO.PULL[bank].SET.write(mask)
        end
    end

    t.std_gpio_out = function(data)
        t.gpio()
        t.dir(true)
        t.pull(false)
        t.write(data)
    end

    t.gpio_in = function(data)
        t.gpio()
        t.dir(false)
    end

    t.std_function = function(fun_nr)
        HW.GPIO.FUNCTION[bank].SET.write(mask)
        if fun_nr >= 2 then
            HW.GPIO.TRIGGER[bank].SET.write(mask)
            fun_nr = fun_nr - 2
        else
            HW.GPIO.TRIGGER[bank].CLR.write(mask)
        end
        if fun_nr >= 2 then
            HW.GPIO.SELECT[bank].SET.write(mask)
        else
            HW.GPIO.SELECT[bank].CLR.write(mask)
        end
    end
    return t
end

function JZ.gpio.pin(bank,pin)
    local mask = bit32.lshift(1, pin)
    local t = JZ.gpio.pinmask(bank,mask)
    t.read = function()
        return bit32.extract(HW.GPIO.IN[bank].read(), pin)
    end
    return t
end
