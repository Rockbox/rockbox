---
--- Chip Identification
---

RK27XX = {}

function RK27XX.init()
    hwstub.soc:select("rk27xx")
end

require 'rk27xx/lradc'
