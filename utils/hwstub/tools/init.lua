-- init code for hwstub_tools

--
-- INFO
--

function hwstub.info()
--- if not hwstub.options.quiet then
    print("information")
    print("  hwstub")
    print("    version: " .. string.format("%d.%d", hwstub.host.version.major,
        hwstub.host.version.minor))
    print("  device")
    print("    version: " .. string.format("%d.%d.%d", hwstub.dev.version.major,
        hwstub.dev.version.minor, hwstub.dev.version.revision))
    print("    target")
    local id_str = string.char(bit32.extract(hwstub.dev.target.id, 0, 8),
        bit32.extract(hwstub.dev.target.id, 8, 8),
        bit32.extract(hwstub.dev.target.id, 16, 8),
        bit32.extract(hwstub.dev.target.id, 24, 8))
    print("      id: " .. string.format("%#x (%s)", hwstub.dev.target.id, id_str))
    print("      name: " .. hwstub.dev.target.name)
    print("    layout")
    print("      code: " .. string.format("%#x bytes @ %#x",
        hwstub.dev.layout.code.size, hwstub.dev.layout.code.start))
    print("      stack: " .. string.format("%#x bytes @ %#x",
        hwstub.dev.layout.stack.size, hwstub.dev.layout.stack.start))
    print("      buffer: " .. string.format("%#x bytes @ %#x",
        hwstub.dev.layout.buffer.size, hwstub.dev.layout.buffer.start))
    if hwstub.dev.target.id == hwstub.dev.target.STMP then
        print("    stmp")
        print("      chipid: " .. string.format("%x", hwstub.dev.stmp.chipid))
        print("      rev: " .. string.format("%d", hwstub.dev.stmp.rev))
        print("      package: " .. string.format("%d", hwstub.dev.stmp.package))
    elseif hwstub.dev.target.id == hwstub.dev.target.PP then
        print("    pp")
        print("      chipid: " .. string.format("%x", hwstub.dev.pp.chipid))
        print("      rev: " .. string.format("%d", hwstub.dev.pp.rev))
    elseif hwstub.dev.target.id == hwstub.dev.target.JZ then
        print("    jz")
        print("      chipid: " .. string.format("%x", hwstub.dev.jz.chipid))
        print("      revision: " .. string.format("%c", hwstub.dev.jz.rev))
    end
end

--
-- SOC
--
function hwstub.soc:select(soc)
    if self[soc] == nil then return false end
    print("Selecting soc " .. soc .. ". Redirecting HW to hwstub.soc." .. soc)
    HW = self[soc]
    return true
end

--
-- DEV
--
DEV = hwstub.dev

--
-- Misc
--
function hwstub.mdelay(msec)
    hwstub.udelay(msec * 1000)
end

require "lua/help"
require "lua/load"

function init()
    LOAD.init()
end

-- first time init
init()
