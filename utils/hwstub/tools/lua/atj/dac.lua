ATJ.DAC = {}

function ATJ.DAC.init()
    -- 1) setup clock gates for DAC and DMA
    HW.CMU.DEVCLKEN.write(bit32.bor(HW.CMU.DEVCLKEN.read(), 0x20100))

    -- 2) setup dac pll for 48kHz
    HW.CMU.AUDIOPLL.DACCLK.write(1)
    HW.CMU.AUDIOPLL.APEN.write(1)

    HW.DAC.FIFOCTL.write(0x141)

    -- DAC enable, 
    HW.DAC.CTL.write(0x90b1)

    HW.DAC.ANALOG.write(0x847)
end

function ATJ.DAC.set_samplerate(samplerate)
    local pll_values = {[96000] = {DACPLL=0, DACCLK=0},
                        [48000] = {DACPLL=0, DACCLK=1},
                        [44100] = {DACPLL=1, DACCLK=1},
                        [32000] = {DACPLL=0, DACCLK=2},
                        [24000] = {DACPLL=0, DACCLK=3},
                        [22050] = {DACPLL=1, DACCLK=3},
                        [16000] = {DACPLL=0, DACCLK=4},
                        [12000] = {DACPLL=0, DACCLK=5},
                        [11025] = {DACPLL=1, DACCLK=5},
                        [8000] =  {DACPLL=0, DACCLK=6}}

    for k, v in pairs(pll_values) do
        if k == samplerate then
            -- disable audio pll
            HW.CMU.AUDIOPLL.APEN.write(0)

            -- 0 = 24MHz base or 1 = 22MHz base
            HW.CMU.AUDIOPLL.DACPLL.write(v.DACPLL)

            -- base divider
            HW.CMU.AUDIOPLL.DACCLK.write(v.DACCLK)

            -- reenable audio pll
            HW.CMU.AUDIOPLL.APEN.write(1)

            return
        end
    end

    error("Invalid samplerate requested\n")
end

-- output programable amplifier gain
-- step is 1.8dB according to datasheet
-- the range is 0-31 where 0 means -55.8dB
function ATJ.DAC.pa_volume(volume)
    local tmp = HW.DAC.ANALOG.read()
    tmp = bit32.band(tmp, 0xffffff07)

    if (volume > 0x1f) or (volume < 0) then
        error("PA volume out of range\n")
    end

    tmp = bit32.bor(tmp, bit32.lshift(volume, 3))
    HW.DAC.ANALOG.write(tmp)
end

-- mute mask for analog output mixer
function ATJ.DAC.mixer_mute(dac_mute, fm_mute, linein_mute, mic_mute)
    local mute = 0

    if dac_mute == false then mute = bit32.bor(mute, 8) end
    if fm_mute == false then mute = bit32.bor(mute, 4) end
    if linein_mute == false then mute = bit32.bor(mute, 2) end
    if mic_mute == false then mute = bit32.bor(mute, 1) end

    mute = bit32.lshift(mute, 8)

    local tmp = HW.DAC.ANALOG.read()
    tmp = bit32.band(tmp, 0xfffff0ff)
    tmp = bit32.bor(tmp, mute)
    HW.DAC.ANALOG.write(tmp)
end

-- This function generate sample tone where right channel
-- has 16 times lower amplitude then left (easy test to
-- distinguish data order
--
-- samplerate - samplerate of the DAC
-- frequency - tone frequency in Hz
-- amplitude - <0,1>
--
-- returns number of generated samples
function generate_tone(samplerate, frequency, amplitude)
    local n = math.floor(samplerate/frequency)
    local amp = amplitude * 0x7fff

    -- uncached DRAM address    
    local addr = 0xa0000000

    for i=0,n-1,1 do
        local sample = math.floor(amp * math.sin(2*i*math.pi/n))

        -- Left channel sample
        DEV.write32(addr + 8*i, bit32.lshift(sample, 16))

        -- Right channel sample (attenuated 16 times)
        DEV.write32(addr + 8*i + 4, bit32.lshift(sample, 12))
    end

    return n
end

function ATJ.DAC.fifo_reset()
    HW.DAC.FIFOCTL.FIRT.write(0)
    hwstub.mdelay(1)
    HW.DAC.FIFOCTL.FIRT.write(1)
end

function test_tone(samplerate, frequency, amplitude)
    -- reset DMA0
    HW.DMAC.CTL.write(0x10000)

    -- reset DAC fifo
    ATJ.DAC.fifo_reset()

    -- setup desired DAC samplerate
    ATJ.DAC.set_samplerate(samplerate)

    -- generate sin
    local n = generate_tone(samplerate, frequency, amplitude)

    -- fire DMA0 in loop mode
    HW.DMAC.DMA_CNT[0].write(8*n)
    HW.DMAC.DMA_MODE[0].write(0x11340084)
    HW.DMAC.DMA_SRC[0].write(0x00000000)
    HW.DMAC.DMA_DST[0].write(0x10100008)
    HW.DMAC.DMA_CMD[0].write(1)
end
