
local rbac_is_loaded = (package.loaded.actions ~= nil)
require("actions")   -- Contains rb.actions & rb.contexts

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

if not rbac_is_loaded then
    rb.actions = nil
    rb.contexts = nil
    package.loaded.actionss = nil
end

return button_t
