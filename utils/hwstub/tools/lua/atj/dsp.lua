ATJ.dsp = {}

-- Ungate DSP clock
function ATJ.dsp.init()
    HW.CMU.DEVCLKEN.write(bit32.bor(HW.CMU.DEVCLKEN.read(), 0x10010))
end

-- Connect memory to DSP bus
-- Start DSP core
function ATJ.dsp.start()
    HW.SRAMOC.CTL.write(0)  -- connect memory to DSP bus
    HW.DSP.CTL.write(0x187) -- reset DSP
    hwstub.mdelay(100)
    HW.DSP.CTL.write(0x18f) -- run DSP from PM@0 clocked from HOSC
end

-- Stop DSP core
-- Connect memory to MIPS bus
function ATJ.dsp.stop()
    HW.DSP.CTL.write(0x107)
    HW.DSP.CTL.write(0x0f)
    HW.SRAMOC.CTL.write(0xf0) -- connect memory to MIPS bus
end

function ATJ.dsp.setclock(mhz)
    local dpck = math.floor(mhz/6)
    if dpck < 2 then dpck = 2 end

    HW.DSP.CTL.write(0x0f)         -- stop DSP clock
    HW.CMU.DSPPLL.DPCK.write(dpck) -- setup pll
    HW.CMU.DSPPLL.DPEN.write(1)    -- enable pll
    hwstub.mdelay(10)              -- wait for PLL to settle
    HW.DSP.CTL.write(0x10f)        -- run DSP again clocked from DSP PLL
end

-- Start the execution of DSP program and wait
-- specified number of miliseconds before stoping DSP
-- Then you can inspect DSP memories from MIPS side
function ATJ.dsp.run(msec)
    ATJ.dsp.stop()
    ATJ.dsp.start()
    hwstub.mdelay(msec)
    ATJ.dsp.stop()
end

-- Clear DSP program memory
function ATJ.dsp.clearPM()
    ATJ.dsp.stop()
    for i=0,16*1024-1,4 do DEV.write32(0xb4040000+i, 0) end
end

-- Clear DSP data memory
function ATJ.dsp.clearDM()
    ATJ.dsp.stop()
    for i=0,16*1024-1,4 do DEV.write32(0xb4050000+i, 0) end
end

-- write single 24bit value to DSP memory
-- 0xb4040000 is start address of PM
-- 0xb4050000 is start address of DM
function ATJ.dsp.write(addr,val)
    DEV.write8(addr+0, bit32.band(val, 0xff))
    DEV.write8(addr+1, bit32.band(bit32.rshift(val, 8), 0xff))
    DEV.write8(addr+2, bit32.band(bit32.rshift(val, 16), 0xff))
end

-- This function takes array of opcodes/values and writes it DSP memory
function ATJ.dsp.prog(opcodes, base, type)
    if base > 0x3fff then
         print(string.format("Invalid address 0x%x", base))
         return
    end

    if type == 'p' then
         -- DSP program memory
         base = base + 0xb4040000
    elseif type == 'd' then
         -- DSP data memory
         base = base + 0xb4050000
    else
        print(string.format("Invalid memory type: %c", type))
        return
    end

    local offset=0
    ATJ.dsp.stop()
    for i,opcode in ipairs(opcodes) do
        ATJ.dsp.write(base+4*offset, opcode)
        offset=offset+1
    end
end

-- This function reads the file produced by as2181 and
-- uploads it to the DSP memory
function ATJ.dsp.progfile(path)
    local opcodes={}
    local addr=nil
    local type=nil

    local fh=io.open(path)
    if fh == nil then
        print(string.format("Unable to open %s", path))
        return
    end

    while true do
        line = fh:read()
        if line == nil then
            break
        end

        -- Search for header describing target memory
        if string.find(line, '@PA') ~= nil then
            type = 'p'
        elseif string.find(line, '@PD') ~= nil then
            type = 'd'
         end

         if type ~= nil then
            -- Next line after the header is the address
            addr = fh:read()
            if addr ~= nil then
                addr = tonumber(addr, 16)
            else
                break;
            end

            while true do
                line = fh:read()
                if line == nil then
                    break
                end

                 -- Check ending clause
                 -- We don't check embedded checksum
                 if string.find(line, '#123') then
                     break
                 end

                 -- Read operand and store in array
                 opcodes[#opcodes+1] = tonumber(line,16)
             end

             if (type == 'p') then
                 print(string.format("Writing %d opcodes PM @ 0x%0x", #opcodes, addr))
             elseif (type == 'd') then
                 print(string.format("Writing %d values DM @ x0%0x", #opcodes, addr))
             end

             -- Write to DSP memory
             ATJ.dsp.prog(opcodes, addr, type)
             opcodes={}
             addr = nil
             type = nil
         end
    end
    fh:close()
end
