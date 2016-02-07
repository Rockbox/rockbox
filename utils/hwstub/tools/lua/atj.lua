---
--- Chip Identification
---

ATJ = {}

function ATJ.init()
    hwstub.soc:select("atj213x")
end

require "atj/gpio"
require "atj/lcm"
require "atj/dsp"
