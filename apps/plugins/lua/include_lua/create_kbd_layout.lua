--create keyboard layout
--BILGUS 4/2021
local bitAND = bit.band
local bitOR  = bit.bor
local RSHIFT = bit.rshift
local LSHIFT = bit.lshift
local CHR = string.char

local function encode_short(n)
    return CHR(bitAND(0x00FF, n), RSHIFT(bitAND(0xFF00, n), 8))
end

function utf8decode(str)
    local t = {}
    local function check_char(c)
        c = string.byte(c)
        local code = 0xfffd;
        local tail = false

        if (c <= 0x7f) or (c >= 0xc2) then
            -- Start of new character
            if (c < 0x80) then        -- U-00000000 - U-0000007F, 1 byte
                code = c;
            elseif (c < 0xe0) then   -- U-00000080 - U-000007FF, 2 bytes
                tail = 1;
                code = bitAND(c, 0x1f)
            elseif (c < 0xf0) then   -- U-00000800 - U-0000FFFF, 3 bytes
                tail = 2;
                code = bitAND(c, 0x0f)
            elseif (c < 0xf5) then -- U-00010000 - U-001FFFFF, 4 bytes
                tail = 3;
                code = bitAND(c, 0x07)
            else
                -- Invalid size
                code = 0xfffd;
            end

            while tail and c ~= 0 do
                tail = tail - 1
                if bitAND(c, 0xc0) == 0x80 then
                    -- Valid continuation character
                    code = bitOR(LSHIFT(code, 6),bitAND(c, 0x3f))
                else 
                    -- Invalid continuation char
                    code = 0xfffd;
                    break;
                end
            end
        else
            -- Invalid UTF-8 char 
            code = 0xfffd;
        end
        -- currently we don't support chars above U-FFFF
        code = (code < 0x10000) and code or 0xfffd;
        t[#t + 1 ] = encode_short(code)
    end
    str:gsub(".", check_char) -- run check function for every char
    return table.concat(t)
end

function create_keyboard_layout(s_layout)
    local insert = table.insert
    lines = {}

    local t={}
    for str in string.gmatch(s_layout, "([^\n]+)") do
        local len = string.len(str)
        lines[#lines + 1] =
        table.concat({encode_short(len), utf8decode(str)})
    end
    lines[#lines + 1] = encode_short(0xFEFF)

    return table.concat(lines)
end

--[[
local name = "Test_KBD_LAYOUT_" .. tostring(1)
local test = create_keyboard_layout("ABCDEFGHIJKLM\nNOPQRSTUVWXYZ\n0123456789")
local file = io.open('/' .. name, "w+") -- overwrite, rb ignores the 'b' flag
file:write(test)-- write the header to the file now
file:close()

if not file then
    rb.splash(rb.HZ, "Error opening /" .. name)
    return
end
rb.kbd_input(name, test)
]]
