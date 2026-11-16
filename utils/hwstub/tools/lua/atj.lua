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
require "atj/nand"
require "atj/charge"
require "atj/fm"
require "atj/udcprobe"
