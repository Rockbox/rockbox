--[[ Lua Blit Operations
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 William Wilgus
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
]]

--[[
copy(dst, src, [dx, dy, sx, sy, offset_x, offset_y, clip, _blit.OP, clr/customfunct])
 blit allows you to copy a [portion of a] source image to a dest image applying
 a transformation operation to the pixels as they are copied
 offsets are auto calculated if left empty or out of range
 blit will default to copy if operation is empty or out of range

it is slightly faster to use the number directly and you don't really
    need to define all (any) of these if you don't use them but I put them
    here for easier use of the blit function
]]
if not rb.lcd_framebuffer then rb.splash(rb.HZ, "No Support!") return nil end

local _blit ={} do

    _blit.CUSTOM = nil  --user defined blit function func(dst_val, x, y, src_val, x, y)
    _blit.BCOPY        = 0x0  --copy (use :copy() instead it is slightly faster
    _blit.BOR          = 0x1  --OR source and dest pixels
    _blit.BXOR         = 0x2  --XOR source and dest pixels
    _blit.BNOR         = 0x3  --(NOT) (source OR dest pixels)
    _blit.BSNOR        = 0x4  --(NOT source) OR dest pixels
    _blit.BAND         = 0x5  --AND source and dest pixels
    _blit.BNAND        = 0x6  --(NOT) AND source and dest pixels
    _blit.BNOT         = 0x7  --NOT source and dest pixels
    --blit functions for masks
    _blit.BSAND        = 0x8 --copy color to dest if source pixel <> 0
    _blit.BSNOT        = 0x9 --copy color to dest if source pixel == 0
    --blit functions for masks with colors
    _blit.BSORC        = 0xA --copy source pixel or color
    _blit.BSXORC       = 0xB --copy source pixel xor color
    _blit.BNSORC       = 0xC --copy ~(src_val | clr)
    _blit.BSORNC       = 0xD --copy src_val | (~clr)
    _blit.BSANDC       = 0xE --copy src_val & clr;
    _blit.BNSANDC      = 0xF --copy (~src_val) & clr
    _blit.BDORNSORC    = 0x10 --copy dst | (~src_val) | clr
    _blit.BXORSADXORC  = 0x11 --copy dst ^ (src_val & (dst_val ^ clr))

    _blit.BSNEC        = 0x12  --copy source pixel if source <> color
    _blit.BSEQC        = 0x13 --copy source pixel if source == color
    _blit.BSGTC        = 0x14 --copy source pixel if source > color
    _blit.BSLTC        = 0x15 --copy source pixel if source < color
    _blit.BDNEC        = 0x16 --copy source pixel if dest <> color
    _blit.BDEQC        = 0x17 --copy source pixel if dest == color
    _blit.BDGTC        = 0x18 --copy source pixel if dest > color
    _blit.BDLTC        = 0x19 --copy source pixel if dest < color
    _blit.BDNES        = 0x1A --copy color to dest if dest <> source pixel
    _blit.BDEQS        = 0x1B --copy color to dest if dest == source pixel
    _blit.BDGTS        = 0x1C --copy color to dest if dest > source pixel
    _blit.BDLTS        = 0x1D --copy color to dest if dest < source pixel
    --Source unused for these blits
    _blit.BCOPYC       = 0x1E --copy color
    _blit.BORC         = 0x1F --OR dest and color
    _blit.BXORC        = 0x20 --XOR dest and color
    _blit.BNDORC       = 0x21 --~(dst_val | clr)
    _blit.BDORNC       = 0x22 --dst_val | (~clr)
    _blit.BANDC        = 0x23 --AND dest and color
    _blit.BNDANDC      = 0x24 --copy (~dst_val) & clr
    _blit.BDLTS        = 0x25 --dest NOT color
end -- _blit operations

return _blit

