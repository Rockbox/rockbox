---
--- ATJ213x PMU battery charger and battery measurement
---
--- Register layout (regs-pmu.h):
---   PMU_CTL   (+0x00): [31] LBRM low battery reset enable (1 = reset on low battery),
---                      [13] LA6E / [12] LA4E LRADC enables
---   PMU_LRADC (+0x04): [31] DC5V, [27:24] REMOADC4 (keys), [21:16] BATADC6,
---                      [13:8] TEMPADC6
---   PMU_CHG   (+0x08): [31] EN, [30:28] CURRENT (50..500mA), [27] STAT (1 = charging),
---                      [26:25] PHASE (1 precharge, 2 CC, 3 CV), [5:4] TMPSET,
---                      [3:2] LBNMIVS, [1:0] LBRVS
--- CV voltage is fixed to 4.2V in hardware.
---
--- On the E150 the battery is connected to the system through a soft power switch held
--- by GPIOA13 (ATJ.charge.sustain). With the pin released the battery is only fed by
--- the charger, so it reads ~0 with the charger off and the player dies on USB unplug.
--- The MBRC and the OF clear LBRM while on USB.
---
ATJ.charge = {}

local PMU_CTL, PMU_LRADC, PMU_CHG = 0xb0000000, 0xb0000004, 0xb0000008

local CURRENTS = {50, 100, 150, 200, 250, 300, 400, 500}
local PHASES = {[0] = "off", "precharge", "CC", "CV"}

local function rd(a) return DEV.read32(a) end
local function wr(a, v) DEV.write32(a, v) end
local function field(v, pos, width) return bit32.extract(v, pos, width) end

local function now()
    local ok, s = pcall(os.date, "%H:%M:%S")
    return ok and s or ""
end

-- uncalibrated: 6 bit ADC assumed to span 2.1V..4.5V
function ATJ.charge.mv(raw)
    return raw * 38 + 2100
end

-- battery level as reported by the OF (CHARGE.DRV act_getBattery): 0..16
function ATJ.charge.of_level(raw)
    if raw < 31 then return 0 end
    return math.min(raw - 30, 16)
end

-- enable LRADC and return raw readings
function ATJ.charge.adc()
    local ctl = rd(PMU_CTL)
    if bit32.band(ctl, 0x3000) ~= 0x3000 then
        wr(PMU_CTL, bit32.bor(ctl, 0x3000))
        hwstub.mdelay(5)
    end
    local v = rd(PMU_LRADC)
    return {
        dc5v = field(v, 31, 1),
        bat  = field(v, 16, 6),
        temp = field(v, 8, 6),
        key  = field(v, 24, 4),
        raw  = v,
    }
end

-- average battery raw value over n samples
function ATJ.charge.bat(n)
    n = n or 8
    local sum = 0
    for i = 1, n do
        sum = sum + ATJ.charge.adc().bat
        hwstub.mdelay(2)
    end
    return sum / n
end

function ATJ.charge.state()
    local c = rd(PMU_CHG)
    return {
        en = field(c, 31, 1),
        current = CURRENTS[field(c, 28, 3) + 1],
        stat = field(c, 27, 1),
        phase = PHASES[field(c, 25, 2)],
        tmpset = field(c, 4, 2),
        raw = c,
    }
end

function ATJ.charge.status()
    local s = ATJ.charge.state()
    local a = ATJ.charge.adc()
    local lbrm = field(rd(PMU_CTL), 31, 1)
    print(string.format("%s chg en=%d stat=%d phase=%-9s %3dmA | bat raw=%2d ~%dmV lvl=%2d"
        .. " temp=%2d dc5v=%d lbrm=%d (CHG=%08x)",
        now(), s.en, s.stat, s.phase, s.current, a.bat, ATJ.charge.mv(a.bat),
        ATJ.charge.of_level(a.bat), a.temp, a.dc5v, lbrm, s.raw))
    return s, a
end

-- select charge current, nearest setting not above ma
function ATJ.charge.set_current(ma)
    local idx = 0
    for i, c in ipairs(CURRENTS) do
        if c <= ma then idx = i - 1 end
    end
    local c = rd(PMU_CHG)
    wr(PMU_CHG, bit32.replace(c, idx, 28, 3))
    return CURRENTS[idx + 1]
end

-- start charging at ma (default 50mA, the lowest setting)
function ATJ.charge.on(ma)
    if ATJ.charge.adc().dc5v == 0 then
        print("no DC5V, not enabling charger")
        return false
    end
    local set = ATJ.charge.set_current(ma or 50)
    wr(PMU_CHG, bit32.bor(rd(PMU_CHG), 0x80000000))
    for i = 1, 20 do
        if ATJ.charge.state().stat == 1 then break end
        hwstub.mdelay(10)
    end
    print(string.format("charger on, %dmA", set))
    ATJ.charge.status()
    return true
end

function ATJ.charge.off()
    wr(PMU_CHG, bit32.band(rd(PMU_CHG), 0x7fffffff))
    ATJ.charge.status()
end

-- power sustain (soft power switch) on GPIOA13: true keeps the battery connected
-- to the system, false releases it (OF SHUTOFF.APP soft_switching_shutoff). Without
-- it the player only runs from USB and the battery reads ~0 unless charging.
function ATJ.charge.sustain(on)
    local GPIO_AOUTEN, GPIO_AINEN, GPIO_ADAT = 0xb01c0000, 0xb01c0004, 0xb01c0008
    local bit = 0x2000
    if on then
        wr(GPIO_ADAT, bit32.bor(rd(GPIO_ADAT), bit))
    else
        wr(GPIO_ADAT, bit32.band(rd(GPIO_ADAT), bit32.bnot(bit)))
    end
    wr(GPIO_AINEN, bit32.band(rd(GPIO_AINEN), bit32.bnot(bit)))
    wr(GPIO_AOUTEN, bit32.bor(rd(GPIO_AOUTEN), bit))
    print(string.format("sustain (GPIOA13)=%d", field(rd(GPIO_ADAT), 13, 1)))
end

-- low battery hardware reset: false disables it (as OF does on USB power)
function ATJ.charge.lbrm(enable)
    local c = rd(PMU_CTL)
    wr(PMU_CTL, enable and bit32.bor(c, 0x80000000) or bit32.band(c, 0x7fffffff))
    print(string.format("LBRM=%d", field(rd(PMU_CTL), 31, 1)))
end

-- briefly switch the charger off and read the battery, then restore
function ATJ.charge.offtest(ms)
    local was = ATJ.charge.state().en
    wr(PMU_CHG, bit32.band(rd(PMU_CHG), 0x7fffffff))
    hwstub.mdelay(ms or 500)
    local raw = ATJ.charge.bat()
    if was == 1 then
        wr(PMU_CHG, bit32.bor(rd(PMU_CHG), 0x80000000))
    end
    print(string.format("%s off-test: bat raw=%.1f ~%dmV", now(), raw,
        ATJ.charge.mv(raw)))
    return raw
end

-- monitor charging
-- opts: minutes (60), period in seconds (30), stop_raw (none), offtest every
-- n periods (0 = never), log = file name to append to
function ATJ.charge.watch(opts)
    opts = opts or {}
    local minutes = opts.minutes or 60
    local period = opts.period or 30
    local log = opts.log and io.open(opts.log, "a")
    local t_end = os.time() + minutes * 60
    local n = 0
    local was_charging = ATJ.charge.state().stat == 1
    while os.time() < t_end do
        local s, a = ATJ.charge.status()
        if log then
            log:write(string.format("%d %d %d %s %d %d %d\n", os.time(), s.en, s.stat,
                s.phase, s.current, a.bat, a.dc5v))
            log:flush()
        end
        if a.dc5v == 0 then
            print("DC5V lost, stopping")
            ATJ.charge.off()
            break
        end
        if was_charging and s.stat == 0 then
            print("charge cycle ended")
            break
        end
        if opts.stop_raw and a.bat >= opts.stop_raw then
            print("stop_raw reached, charger off")
            ATJ.charge.off()
            break
        end
        n = n + 1
        if opts.offtest and opts.offtest > 0 and n % opts.offtest == 0 then
            local raw = ATJ.charge.offtest()
            if log then
                log:write(string.format("%d offtest %.1f\n", os.time(), raw))
                log:flush()
            end
        end
        hwstub.mdelay(period * 1000)
    end
    if log then log:close() end
end
