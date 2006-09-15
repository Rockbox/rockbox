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

#ifndef NO_OPDEF
#include "z80_def.h"
#include "z80_op2.h"
#endif

OPDEF(halt, 0x76)
{
  register int nn;

  DANM(haltstate) = 1;
  nn = (DANM(tc) - 1) / 4 + 1;
  
  DANM(tc) -= 4 * nn;
  RR += nn-1;
  ENDOP();
}

#define LD_R_R(rdn, rsn, rd, rs, n1, n2) \
OPDEF(ld_ ## rdn ## _ ## rsn, 0x40 + n1 * 8 + n2)          \
{                                                          \
  rd = rs;                                                 \
  ENTIME(4);                                               \
}

#if 0

#define LD_NOOP(rsd, n) \
OPDEF(ld_ ## rsd ## _ ## rsd, 0x40 + n * 8 + n)            \
{                                                          \
  ENTIME(4);                                               \
}

LD_NOOP(b, 0)
LD_NOOP(c, 1)
LD_NOOP(d, 2)
LD_NOOP(e, 3)
LD_NOOP(h, 4)
LD_NOOP(l, 5)
LD_NOOP(a, 7)

#endif

LD_R_R(b, c, RB, RC, 0, 1)
LD_R_R(b, d, RB, RD, 0, 2)
LD_R_R(b, e, RB, RE, 0, 3)
LD_R_R(b, h, RB, RH, 0, 4)
LD_R_R(b, l, RB, RL, 0, 5)
LD_R_R(b, a, RB, RA, 0, 7)

LD_R_R(c, b, RC, RB, 1, 0)
LD_R_R(c, d, RC, RD, 1, 2)
LD_R_R(c, e, RC, RE, 1, 3)
LD_R_R(c, h, RC, RH, 1, 4)
LD_R_R(c, l, RC, RL, 1, 5)
LD_R_R(c, a, RC, RA, 1, 7)

LD_R_R(d, b, RD, RB, 2, 0)
LD_R_R(d, c, RD, RC, 2, 1)
LD_R_R(d, e, RD, RE, 2, 3)
LD_R_R(d, h, RD, RH, 2, 4)
LD_R_R(d, l, RD, RL, 2, 5)
LD_R_R(d, a, RD, RA, 2, 7)


LD_R_R(e, b, RE, RB, 3, 0)
LD_R_R(e, c, RE, RC, 3, 1)
LD_R_R(e, d, RE, RD, 3, 2)
LD_R_R(e, h, RE, RH, 3, 4)
LD_R_R(e, l, RE, RL, 3, 5)
LD_R_R(e, a, RE, RA, 3, 7)

LD_R_R(h, b, RH, RB, 4, 0)
LD_R_R(h, c, RH, RC, 4, 1)
LD_R_R(h, d, RH, RD, 4, 2)
LD_R_R(h, e, RH, RE, 4, 3)
LD_R_R(h, l, RH, RL, 4, 5)
LD_R_R(h, a, RH, RA, 4, 7)

LD_R_R(l, b, RL, RB, 5, 0)
LD_R_R(l, c, RL, RC, 5, 1)
LD_R_R(l, d, RL, RD, 5, 2)
LD_R_R(l, e, RL, RE, 5, 3)
LD_R_R(l, h, RL, RH, 5, 4)
LD_R_R(l, a, RL, RA, 5, 7)

LD_R_R(a, b, RA, RB, 7, 0)
LD_R_R(a, c, RA, RC, 7, 1)
LD_R_R(a, d, RA, RD, 7, 2)
LD_R_R(a, e, RA, RE, 7, 3)
LD_R_R(a, h, RA, RH, 7, 4)
LD_R_R(a, l, RA, RL, 7, 5)


#define LD_R_IHL(rdn, rd, n) \
OPDEF(ld_ ## rdn ## _ihl, 0x46+n*8)          \
{                                            \
  rd = *HLP;                                 \
  ENTIME(7);                                 \
}

#define LD_R_ID(ixyn, ixy, rsn, rs, n) \
OPDEF(ld_ ## rsn ## _i ## ixyn ## d, 0x46+n*8) \
{                                  \
  register dbyte addr;             \
  IXDGET(ixy, addr);               \
  rs = READ(addr);                 \
  ENTIME(15);                      \
}


LD_R_IHL(b, RB, 0)
LD_R_IHL(c, RC, 1)
LD_R_IHL(d, RD, 2)
LD_R_IHL(e, RE, 3)
LD_R_IHL(h, RH, 4)
LD_R_IHL(l, RL, 5)
LD_R_IHL(a, RA, 7)

#define LD_IHL_R(rsn, rs, n) \
OPDEF(ld_ihl_ ## rsn, 0x70+n)      \
{                                  \
  PUTMEM(HL, HLP, rs);             \
  ENTIME(7);                       \
}

#define LD_ID_R(ixyn, ixy, rsn, rs, n) \
OPDEF(ld_i ## ixyn ## d_ ## rsn, 0x70+n) \
{                                  \
  register dbyte addr;             \
  IXDGET(ixy, addr);               \
  WRITE(addr, rs);                 \
  ENTIME(15);                      \
}



LD_IHL_R(b, RB, 0)
LD_IHL_R(c, RC, 1)
LD_IHL_R(d, RD, 2)
LD_IHL_R(e, RE, 3)
LD_IHL_R(h, RH, 4)
LD_IHL_R(l, RL, 5)
LD_IHL_R(a, RA, 7)

#include "z80_op2x.c"
