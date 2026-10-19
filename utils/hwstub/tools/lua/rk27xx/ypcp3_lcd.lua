--
-- Samsung YP-CP3 LCD: the panel sleep / LCDC clock gating that
-- lcd_enable() does in firmware/target/arm/rk27xx/ypcp3/lcd-ypcp3.c and
-- lcdif-rk27xx.c, as plain register writes.
--
-- Usage (inside hwstub_shell):
--   dofile("lua/rk27xx/ypcp3_lcd.lua")
--   YPLCD.test()                 -- init, pattern A, off, wait, on, pattern B
--   YPLCD.test{ init = false }   -- the panel is already running
--   YPLCD.test{ off_ms = 10000 }
--
-- What to look for:
--   1. after init: four squares along the top - red, green, blue, white
--   2. lcd_enable(false): the picture goes away (the panel sleeps; the
--      backlight stays on to show it)
--   3. lcd_enable(true): the four squares are back - the panel kept its
--      GRAM while asleep
--   4. pattern B: a black/white checkerboard below them, drawn after the
--      LCDC clock came back - writes reach the panel again, in the right
--      place
--
-- Pixels go one by one through the bypass data register (the firmware
-- uses the LCDC buffer and the DWDMA): slow over USB, so the patterns are
-- small and the rest of the screen is left as it is.
--

YPLCD = {}

local DEV = DEV or hwstub.dev
local function rd(a) return DEV.read32(a) end
local function wr(a, v) DEV.write32(a, v) end
local function setb(a, m) wr(a, bit32.bor(rd(a), m)) end
local function clrb(a, m) wr(a, bit32.band(rd(a), bit32.bnot(m))) end

local SCU        = 0x1801c000
local CLKCFG     = SCU + 0x18
local IOMUXA     = SCU + 0x30
local IOMUXB     = SCU + 0x34
local CLK_PWM    = bit32.lshift(1, 29)
local CLK_HLCDC  = bit32.lshift(1, 12)       -- CLKCFG_HCLK_LCDC

local GPIO_PDDR  = 0x1800c018
local GPIO_PDCON = 0x1800c01c

local PWM        = 0x1802c000
local PWMT0_CNTR = PWM + 0x00
local PWMT0_HRC  = PWM + 0x04
local PWMT0_LRC  = PWM + 0x08
local PWMT0_CTRL = PWM + 0x0C

local LCDC       = 0x186E8000
local LCDC_CTRL  = LCDC + 0x00
local MCU_CTRL   = LCDC + 0x04
local VERT_PERIOD = LCDC + 0x0C
local LCDC_STA   = LCDC + 0x7C
local LCD_COMMAND = LCDC + 0x1000
local LCD_DATA   = LCDC + 0x1004

local RGB24B     = bit32.lshift(1, 6)
local MCU_BYPASS = 1
local MCU_IDLE   = bit32.lshift(1, 12)

-- the slower of lcdif-rk27xx.c's two: whatever the bus clock under
-- hwstub, at most 100 MHz, the panel's write cycle is long enough
local LCD_TIMING = bit32.bor(bit32.lshift(2, 7), bit32.lshift(8, 3), 2)

-- SPFD5420A registers (spfd5420a.h)
local R = {
    DRIVER_OUT_CTRL = 0x001, WAVEFORM_CTRL = 0x002, ENTRY_MODE = 0x003,
    SHAPENING_CTRL = 0x006, DISPLAY_CTRL1 = 0x007, DISPLAY_CTRL2 = 0x008,
    LOW_PWR_CTRL1 = 0x009, LOW_PWR_CTRL2 = 0x00B, EXT_DISP_CTRL1 = 0x00C,
    EXT_DISP_CTRL2 = 0x00F, PANEL_IF_CTRL1 = 0x010, PANEL_IF_CTRL2 = 0x011,
    PANEL_IF_CTRL3 = 0x012, PANEL_IF_CTRL4 = 0x020, PANEL_IF_CTRL5 = 0x021,
    PANEL_IF_CTRL6 = 0x022, FRAME_MKR_CTRL = 0x090, MDDI_CTRL = 0x092,
    PWR_CTRL1 = 0x100, PWR_CTRL2 = 0x101, PWR_CTRL3 = 0x102,
    PWR_CTRL4 = 0x103, PWR_CTRL6 = 0x110, PWR_CTRL7 = 0x112,
    GRAM_H_ADDR = 0x200, GRAM_V_ADDR = 0x201, GRAM_WRITE = 0x202,
    WINDOW_H_START = 0x210, WINDOW_H_END = 0x211,
    WINDOW_V_START = 0x212, WINDOW_V_END = 0x213,
    VCOM_HV1 = 0x281, VCOM_HV2 = 0x282,
    BASE_IMG_SIZE = 0x400, BASE_IMG_CTRL = 0x401, VSCROLL_CTRL = 0x404,
    PART1_POS = 0x500, PART1_START = 0x501, PART1_END = 0x502,
    PART2_POS = 0x503, PART2_START = 0x504, PART2_END = 0x505,
    RESET = 0x600, IF_ENDIAN = 0x606,
}

-- RGB565 to the 18-bit bus, as lcd_data_transform()
local function xform(d)
    local r = bit32.lshift(bit32.band(d, 0xfc00), 8)
    local g = bit32.bor(bit32.lshift(bit32.band(d, 0x300), 6),
                        bit32.lshift(bit32.band(d, 0xe0), 5))
    local b = bit32.lshift(bit32.band(d, 0x1f), 3)
    return bit32.bor(r, g, b)
end

local function cmd(c) wr(LCD_COMMAND, xform(c)) end
local function data(d) wr(LCD_DATA, xform(d)) end
local function reg(r, v) cmd(r); data(v) end
local function udelay(us) hwstub.udelay(us) end

local function wait_idle()
    local n = 10000
    while not bit32.btest(rd(LCDC_STA), MCU_IDLE) and n > 0 do n = n - 1 end
    if n == 0 then print("  LCDC never went idle") end
end

-- lcdctrl_bypass(1) and RGB24B: how lcd-ypcp3.c talks to the panel
local function bypass()
    wait_idle()
    setb(MCU_CTRL, MCU_BYPASS)
    setb(LCDC_CTRL, RGB24B)
end

-- lcd_display_init()'s power on
local function power_on()
    reg(R.DISPLAY_CTRL1, 0x0001)
    reg(R.PWR_CTRL6,     0x0001)
    reg(R.PWR_CTRL7,     0x0060); udelay(500)
    reg(R.PWR_CTRL1,     0x16B0); udelay(100)
    reg(R.PWR_CTRL2,     0x0147); udelay(100)
    reg(R.PWR_CTRL3,     0x0117); udelay(100)
    reg(R.PWR_CTRL4,     0x2F00); udelay(500)
    reg(R.VCOM_HV2,      0x0000); udelay(100)
    reg(R.VCOM_HV1,      0x000A)
    reg(R.PWR_CTRL3,     0x01BE); udelay(100)
end

-- ...and its display on, in steps
local function display_on()
    reg(R.DISPLAY_CTRL1, 0x0021); udelay(400)
    reg(R.DISPLAY_CTRL1, 0x0061); udelay(1000)
    reg(R.DISPLAY_CTRL1, 0x0173); udelay(3000)
end

-- lcd_sleep(). Waking, opt (wake_test()'s) may change what the firmware
-- does: settle_ms after leaving sleep (10), power = true to run the whole
-- power on again first
local function panel_sleep(sleep, opt)
    opt = opt or {}
    if sleep then
        reg(R.DISPLAY_CTRL1, 0x0170)
        udelay(50)
        reg(R.DISPLAY_CTRL1, 0x0000)
        udelay(50)
        reg(R.PWR_CTRL1,     0x14B4)
    else
        if opt.power then power_on() end
        reg(R.PWR_CTRL1,     0x14B0)
        hwstub.mdelay(opt.settle_ms or 10)
        display_on()
    end
    cmd(R.GRAM_WRITE)
end

-- iomux_lcd(), lcdctrl_init()'s LCDC setup and lcd_display_init() without
-- its full-screen clear
function YPLCD.init()
    print("init: LCDC, then the panel")
    clrb(CLKCFG, CLK_HLCDC)

    -- 18-bit bus: LCD_D16-D22, D8-D15
    local muxa = bit32.band(rd(IOMUXA), bit32.bnot(bit32.bor(
        bit32.lshift(1, 11), bit32.lshift(1, 10), 0xff)))
    muxa = bit32.bor(muxa, bit32.lshift(1, 4), bit32.lshift(1, 6),
                     bit32.lshift(1, 8), bit32.lshift(1, 2), 1)
    wr(IOMUXA, muxa)
    setb(IOMUXB, bit32.lshift(1, 15))

    -- ALPHA(7) | LCDC_STOP | LCDC_MCU | RGB24B; MCU_CTRL ALPHA_BASE(0x3f), bypass
    wr(LCDC_CTRL, bit32.bor(bit32.lshift(7, 9), 1, bit32.lshift(1, 7), RGB24B))
    wr(MCU_CTRL, bit32.bor(bit32.lshift(0x3f, 8), MCU_BYPASS))
    wr(VERT_PERIOD, LCD_TIMING)

    reg(R.RESET, 0x0001); hwstub.mdelay(100)
    reg(R.RESET, 0x0000); hwstub.mdelay(100)

    local seq = {
        { "IF_ENDIAN", 0x0000 }, { "DRIVER_OUT_CTRL", 0x0000 },
        { "WAVEFORM_CTRL", 0x0100 }, { "ENTRY_MODE", 0x1038 },
        { "SHAPENING_CTRL", 0x0000 }, { "DISPLAY_CTRL2", 0x0808 },
        { "LOW_PWR_CTRL1", 0x0001 }, { "LOW_PWR_CTRL2", 0x0010 },
        { "EXT_DISP_CTRL1", 0x0000 }, { "EXT_DISP_CTRL2", 0x0000 },
        { "BASE_IMG_SIZE", 0x3100 }, { "BASE_IMG_CTRL", 0x0001 },
        { "VSCROLL_CTRL", 0x0000 }, { "PART1_POS", 0x0000 },
        { "PART1_START", 0x0000 }, { "PART1_END", 0x018F },
        { "PART2_POS", 0x0000 }, { "PART2_START", 0x0000 },
        { "PART2_END", 0x0000 },
    }
    for _, s in ipairs(seq) do reg(R[s[1]], s[2]) end

    reg(R.PANEL_IF_CTRL1, 0x0011); udelay(100)
    reg(R.PANEL_IF_CTRL2, 0x0202)
    reg(R.PANEL_IF_CTRL3, 0x0300); udelay(100)
    reg(R.PANEL_IF_CTRL4, 0x021E)
    reg(R.PANEL_IF_CTRL5, 0x0202)
    reg(R.PANEL_IF_CTRL6, 0x0100)
    reg(R.FRAME_MKR_CTRL, 0x0000)
    reg(R.MDDI_CTRL,      0x0000)

    local gamma = { 0x0101, 0x0024, 0x1321, 0x2613, 0x2400, 0x0100, 0x1704,
                    0x0417, 0x0007, 0x0101, 0x0F05, 0x0F01, 0x010F, 0x050F,
                    0x0501, 0x0700 }
    for i, v in ipairs(gamma) do reg(0x300 + i - 1, v) end

    power_on()
    display_on()
    panel_sleep(false)
end

-- the backlight on at a mid level, as backlight_hw_init()/_on()
function YPLCD.backlight()
    clrb(CLKCFG, CLK_PWM)
    setb(GPIO_PDCON, bit32.lshift(1, 4))
    clrb(GPIO_PDDR, bit32.lshift(1, 4))
    wr(PWMT0_CTRL, bit32.lshift(1, 7))           -- reset
    wr(PWMT0_LRC, 50000)
    wr(PWMT0_HRC, 22045)                          -- lin_brightness[20]
    wr(PWMT0_CNTR, 0)
    wr(PWMT0_CTRL, bit32.bor(bit32.lshift(1, 3), 1))
    setb(IOMUXB, bit32.lshift(1, 11))             -- PWM0 pin
end

-- lcd_set_gram_area(), in landscape: the panel's horizontal is our
-- vertical
local function area(x0, y0, x1, y1)
    reg(R.WINDOW_H_START, y0)
    reg(R.WINDOW_H_END,   y1)
    reg(R.WINDOW_V_START, x0)
    reg(R.WINDOW_V_END,   x1)
    reg(R.GRAM_H_ADDR,    y0)
    reg(R.GRAM_V_ADDR,    x0)
    cmd(R.GRAM_WRITE)
end

-- a rectangle; colour(x, y) gives each pixel, RGB565
local function rect(x, y, w, h, colour)
    area(x, y, x + w - 1, y + h - 1)
    for j = 0, h - 1 do
        for i = 0, w - 1 do data(colour(i, j)) end
    end
end

local function solid(c) return function() return c end end

function YPLCD.pattern_a()
    print("pattern A: red, green, blue, white squares along the top")
    bypass()
    local S = 48
    rect(16,          16, S, S, solid(0xF800))
    rect(16 + 64,     16, S, S, solid(0x07E0))
    rect(16 + 128,    16, S, S, solid(0x001F))
    rect(16 + 192,    16, S, S, solid(0xFFFF))
end

function YPLCD.pattern_b()
    print("pattern B: checkerboard below the squares")
    bypass()
    rect(16, 96, 96, 96, function(i, j)
        return (math.floor(i / 12) + math.floor(j / 12)) % 2 == 0
               and 0xFFFF or 0x0000
    end)
end

-- lcd_enable(): the panel to sleep, then the LCDC's bus clock off - or
-- the clock on, VERT_PERIOD again, and the panel awake
function YPLCD.enable(on, opt)
    if on then
        clrb(CLKCFG, CLK_HLCDC)
        wr(VERT_PERIOD, LCD_TIMING)
    end
    bypass()
    panel_sleep(not on, opt)
    clrb(LCDC_CTRL, RGB24B)
    if not on then
        wait_idle()
        setb(CLKCFG, CLK_HLCDC)
    end
end

function YPLCD.test(opt)
    opt = opt or {}
    local off_ms = opt.off_ms or 5000

    YPLCD.backlight()
    if opt.init ~= false then YPLCD.init() end
    YPLCD.pattern_a()
    print("  look: four squares")
    hwstub.mdelay(3000)

    print(string.format("lcd_enable(false): panel asleep, LCDC clock off for %d ms", off_ms))
    YPLCD.enable(false)
    print(string.format("  CLKCFG %08x, HCLK_LCDC %s", rd(CLKCFG),
        bit32.btest(rd(CLKCFG), CLK_HLCDC) and "gated" or "CLOCKED"))
    print("  look: the picture gone, backlight on")
    hwstub.mdelay(off_ms)

    print("lcd_enable(true)")
    YPLCD.enable(true)
    print(string.format("  VERT_PERIOD %08x", rd(VERT_PERIOD)))
    print("  look: the four squares back, as before")
    hwstub.mdelay(3000)

    YPLCD.pattern_b()
    print("  look: checkerboard under the squares, edges straight")
end

-- ---- experiments ----

-- backlight_hw_off(): the PWM0 pin a GPIO, pulled low, the timer stopped
function YPLCD.backlight_off()
    clrb(IOMUXB, bit32.lshift(1, 11))
    clrb(PWMT0_CTRL, bit32.bor(bit32.lshift(1, 3), 1))
end

-- The white flash on wake, as in Rockbox: backlight and panel off, then
-- the panel woken, wait_ms, and the backlight on. Watch the moment it
-- lights. Pattern A must be on the screen (YPLCD.test() leaves it).
--   YPLCD.wake_test()                        -- what the firmware does now
--   YPLCD.wake_test{ settle_ms = 50 }
--   YPLCD.wake_test{ wait_ms = 200 }
--   YPLCD.wake_test{ power = true }          -- the whole power on again
--   YPLCD.wake_test{ n = 5 }                 -- five times, 2 s apart
function YPLCD.wake_test(opt)
    opt = opt or {}
    local wait_ms = opt.wait_ms or 40
    for i = 1, opt.n or 1 do
        print(string.format("wake %d: settle %d ms, %s, backlight %d ms after",
            i, opt.settle_ms or 10, opt.power and "power on again" or "sleep bit only",
            wait_ms))
        YPLCD.backlight_off()
        YPLCD.enable(false)
        hwstub.mdelay(1000)
        YPLCD.enable(true, opt)
        hwstub.mdelay(wait_ms)
        YPLCD.backlight()
        hwstub.mdelay(2000)
    end
end

-- A grey ramp across the screen, h rows from y: for the stripes, which
-- show on smooth gradients. Slow: 400 pixels a row.
function YPLCD.gradient(y, h)
    y = y or 200
    h = h or 32
    print(string.format("grey ramp, rows %d-%d", y, y + h - 1))
    bypass()
    rect(0, y, 400, h, function(i)
        local g = math.floor(i * 64 / 400)          -- 6-bit green
        local rb = bit32.rshift(g, 1)               -- 5-bit red and blue
        return bit32.bor(bit32.lshift(rb, 11), bit32.lshift(g, 5), rb)
    end)
end

-- A panel register, for trying values by hand (the panel's are 16-bit)
function YPLCD.reg(r, v)
    bypass()
    reg(r, v)
    cmd(R.GRAM_WRITE)
end

-- Steps panel register r from v0 to v1, ms apart, printing each value:
-- watch the ramp's stripes come and go. E.g. VCOM_HV1, the VCOMH level
-- (stripes at the reset value; none from 0x07 to 0x0C, init sets 0x0A):
--   YPLCD.sweep(0x281, 0x00, 0x3F, 1, 1500)
-- r is left at the last value; put it back with YPLCD.reg(), or init.
function YPLCD.sweep(r, v0, v1, step, ms)
    for v = v0, v1, step or 1 do
        print(string.format("  reg %03x = %04x", r, v))
        YPLCD.reg(r, v)
        hwstub.mdelay(ms or 1500)
    end
end
