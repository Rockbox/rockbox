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

LD_RR_NN(ix, IX, 2)

ADD_RR_RR(ix, IX, bc, BC, 0)
ADD_RR_RR(ix, IX, de, DE, 1)
ADD_RR_RR(ix, IX, ix, IX, 2)
ADD_RR_RR(ix, IX, sp, SP, 3)

INC_RR(ix, IX, 2)

DEC_RR(ix, IX, 2)

LD_INN_RR(ix, IX)

LD_RR_INN(ix, IX)

INC_R(ixh, XH, 4)
INC_R(ixl, XL, 5)

OPDEF(inc_iixd, 0x34)
{
  register dbyte addr;
  IXDGET(IX, addr);
  MODMEMADDR(INC, addr);
  ENTIME(19);
}

DEC_R(ixh, XH, 4)
DEC_R(ixl, XL, 5)

OPDEF(dec_iixd, 0x35)
{
  register dbyte addr;
  IXDGET(IX, addr);
  MODMEMADDR(DEC, addr);
  ENTIME(19);
}


LD_R_N(ixh, XH, 4)
LD_R_N(ixl, XL, 5)

OPDEF(ld_iixd_n, 0x36)
{
  register dbyte addr;
  IXDGET(IX, addr);
  WRITE(addr, READ(PC)); 
  PC++;
  ENTIME(15);
}


/* IY */

LD_RR_NN(iy, IY, 2)

ADD_RR_RR(iy, IY, bc, BC, 0)
ADD_RR_RR(iy, IY, de, DE, 1)
ADD_RR_RR(iy, IY, iy, IY, 2)
ADD_RR_RR(iy, IY, sp, SP, 3)

INC_RR(iy, IY, 2)

DEC_RR(iy, IY, 2)

LD_INN_RR(iy, IY)

LD_RR_INN(iy, IY)

INC_R(iyh, YH, 4)
INC_R(iyl, YL, 5)

OPDEF(inc_iiyd, 0x34)
{
  register dbyte addr;
  IXDGET(IY, addr);
  MODMEMADDR(INC, addr);
  ENTIME(19);
}

DEC_R(iyh, YH, 4)
DEC_R(iyl, YL, 5)

OPDEF(dec_iiyd, 0x35)
{
  register dbyte addr;
  IXDGET(IY, addr);
  MODMEMADDR(DEC, addr);
  ENTIME(19);
}


LD_R_N(iyh, YH, 4)
LD_R_N(iyl, YL, 5)

OPDEF(ld_iiyd_n, 0x36)
{
  register dbyte addr;
  IXDGET(IY, addr);
  WRITE(addr, READ(PC)); 
  PC++;
  ENTIME(15);
}

