---
--- ROM
---
STMP.rom = {}

-- if path is nil, create a generic path that depends on stmp version and rom version
function STMP.rom.dump(path)
    local name = path
    if name == nil then
        name = string.format("stmp%04x_ta%d.bin", hwstub.dev.stmp.chipid, hwstub.dev.stmp.rev + 1)
    end
    local file = io.open(name, "wb")
    file:write(DEV.read(0xffff0000, 0x10000))
    file:close()
    print("Dumping ROM to " .. name)
end
