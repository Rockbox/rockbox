-- Bilgus 4/2021
local oldrb = rb
local tmploader = require("temploader")

rb = {} --replace the rb table so we can keep the defines out of the namespace
--require("actions")   -- Contains rb.actions & rb.contexts

local actions, err = tmploader("actions")
if err then
    error(err)
end

-- Menu Button definitions --
local button_t = {
    CANCEL = rb.actions.PLA_CANCEL,
    DOWN = rb.actions.PLA_DOWN,
    DOWNR = rb.actions.PLA_DOWN_REPEAT,
    EXIT = rb.actions.PLA_EXIT,
    LEFT = rb.actions.PLA_LEFT,
    LEFTR = rb.actions.PLA_LEFT_REPEAT,
    RIGHT = rb.actions.PLA_RIGHT,
    RIGHTR = rb.actions.PLA_RIGHT_REPEAT,
    SEL = rb.actions.PLA_SELECT,
    SELREL = rb.actions.PLA_SELECT_REL,
    SELR = rb.actions.PLA_SELECT_REPEAT,
    UP = rb.actions.PLA_UP,
    UPR = rb.actions.PLA_UP_REPEAT,
}

rb = oldrb
return button_t
