-- bilgus 4/2021
require "buttons"
local BUTTON_NONE = 0

local function decode_ord_btn(btn)
    local btntxt = ""
    for k, v in pairs(rb.buttons) do
        if btn > BUTTON_NONE and v ~= BUTTON_NONE and bit.band(btn, v) == v then
            if #btntxt > 0 then
                btntxt = btntxt .. " | "
            end
            btntxt = btntxt .. k
        end
    end
    if btntxt == "" then return nil end
    return btntxt
end

local _, w, h = rb.font_getstringsize("W", rb.FONT_UI)
local max_lines = rb.LCD_HEIGHT / h - 1

button_text = {}
for k, v in pairs(rb.buttons) do
    button_text[v] = k
end

--Add the system button codes to the button_text table
for k, v in pairs(rb) do
    if string.find(k or "", "SYS_", 1, true) and type(v) == "number" then
        button_text[v] = k
    end
end

local s = {[1] = "Buttons Found:"}
for k, v in pairs(button_text) do
    table.insert(s, tostring(k) .. " : " .. tostring(v))
end
rb.splash_scroller(rb.HZ, table.concat(s, "\n"))

button_text[BUTTON_NONE] = " "

local lastbtn = BUTTON_NONE
local lastkey = BUTTON_NONE
local s_t = {"Press same button 3x to exit"}
local keyrpt = 0

repeat

    local btn
    if lastbtn == BUTTON_NONE then
        btn = rb.button_get(true)
    else
        btn = rb.button_get_w_tmo(rb.HZ)
    end

    if btn ~= lastkey then
        table.insert(s_t, 1, (button_text[btn] or decode_ord_btn(btn) or "unknown " .. tostring (btn)))
    end

    if btn == lastbtn then keyrpt = keyrpt + 1 end
    if button_text[btn] then lastbtn = btn end
    lastkey = btn

    rb.splash_scroller(-2, table.concat(s_t, "\n"))

    if #s_t > max_lines then
        table.remove(s_t)
    end

until keyrpt >= 2
