/* 
 * Copyright (C) 1996-1998 Szeredi Miklos
 * Email: mszeredi@inf.bme.hu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the file COPYING. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* IX */

ADD_A_R(ixh, XH, 4)
ADD_A_R(ixl, XL, 5)

ADC_A_R(ixh, XH, 4)
ADC_A_R(ixl, XL, 5)

SUB_R(ixh, XH, 4)
SUB_R(ixl, XL, 5)

SBC_A_R(ixh, XH, 4)
SBC_A_R(ixl, XL, 5)

AND_R(ixh, XH, 4)
AND_R(ixl, XL, 5)

XOR_R(ixh, XH, 4)
XOR_R(ixl, XL, 5)

OR_R(ixh, XH, 4)
OR_R(ixl, XL, 5)

CP_R(ixh, XH, 4)
CP_R(ixl, XL, 5)

ARIID(add_a, ADD, 0, ix, IX)
ARIID(adc_a, ADC, 1, ix, IX)
ARIID(sub,   SUB, 2, ix, IX)
ARIID(sbc_a, SBC, 3, ix, IX)
ARIID(and,   AND, 4, ix, IX)
ARIID(xor,   XOR, 5, ix, IX)
ARIID(or,    OR,  6, ix, IX)
ARIID(cp,    CP,  7, ix, IX)

/* IY */

ADD_A_R(iyh, YH, 4)
ADD_A_R(iyl, YL, 5)

ADC_A_R(iyh, YH, 4)
ADC_A_R(iyl, YL, 5)

SUB_R(iyh, YH, 4)
SUB_R(iyl, YL, 5)

SBC_A_R(iyh, YH, 4)
SBC_A_R(iyl, YL, 5)

AND_R(iyh, YH, 4)
AND_R(iyl, YL, 5)

XOR_R(iyh, YH, 4)
XOR_R(iyl, YL, 5)

OR_R(iyh, YH, 4)
OR_R(iyl, YL, 5)

CP_R(iyh, YH, 4)
CP_R(iyl, YL, 5)

ARIID(add_a, ADD, 0, iy, IY)
ARIID(adc_a, ADC, 1, iy, IY)
ARIID(sub,   SUB, 2, iy, IY)
ARIID(sbc_a, SBC, 3, iy, IY)
ARIID(and,   AND, 4, iy, IY)
ARIID(xor,   XOR, 5, iy, IY)
ARIID(or,    OR,  6, iy, IY)
ARIID(cp,    CP,  7, iy, IY)
