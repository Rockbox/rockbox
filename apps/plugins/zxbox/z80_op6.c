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
#include "z80_op6.h"
#endif

#define GHL DANM(cbaddr)

#define B7(r) (((r) & 0x80) >> 7)
#define B0(r) ((r) & 0x01)

#define SHIFTROTL(r, mod) \
{                                  \
  register int carry;              \
  carry = B7(r);                   \
  r = mod;                         \
  RF = (RF & ~(AALLF)) | carry |    \
       TAB(orf_tbl)[(byte) r];     \
}


#define SHIFTROTR(r, mod) \
{                                  \
  register int carry;              \
  carry = B0(r);                   \
  r = mod;                         \
  RF = (RF & ~(AALLF)) | carry |    \
       TAB(orf_tbl)[(byte) r];     \
}


#define RLC(r) SHIFTROTL(r, (r << 1) | carry)
#define RRC(r) SHIFTROTR(r, (r >> 1) | (carry << 7))
#define RLN(r) SHIFTROTL(r, (r << 1) | (RF & CF))
#define RRN(r) SHIFTROTR(r, (r >> 1) | ((RF & CF) << 7))
#define SLA(r) SHIFTROTL(r, r << 1)
#define SRA(r) SHIFTROTR(r, (byte) ((sbyte) r >> 1))
#define SLL(r) SHIFTROTL(r, (r << 1) | 0x01)
#define SRL(r) SHIFTROTR(r, r >> 1)


#define SHRR(shrn, func, an, rn, r, n) \
OPDEF(shrn ## _ ## rn, 0x00+an*8+n)      \
{                                        \
  func(r);                               \
  ENTIME(8);                             \
}

#define SHRIHL(shrn, func, an) \
OPDEF(shrn ## _ihl, 0x06+an*8)           \
{                                        \
  register byte btmp;                    \
  btmp = READ(GHL);                      \
  func(btmp);                            \
  WRITE(GHL, btmp);                      \
  ENTIME(15);                            \
}

#define RLC_R(rn, r, n) SHRR(rlc, RLC, 0, rn, r, n)
#define RRC_R(rn, r, n) SHRR(rrc, RRC, 1, rn, r, n)
#define RL_R(rn, r, n)  SHRR(rl,  RLN,  2, rn, r, n)
#define RR_R(rn, r, n)  SHRR(rr,  RRN,  3, rn, r, n)
#define SLA_R(rn, r, n) SHRR(sla, SLA, 4, rn, r, n)
#define SRA_R(rn, r, n) SHRR(sra, SRA, 5, rn, r, n)
#define SLL_R(rn, r, n) SHRR(sll, SLL, 6, rn, r, n)
#define SRL_R(rn, r, n) SHRR(srl, SRL, 7, rn, r, n)

RLC_R(b, RB, 0)
RLC_R(c, RC, 1)
RLC_R(d, RD, 2)
RLC_R(e, RE, 3)
RLC_R(h, RH, 4)
RLC_R(l, RL, 5)
RLC_R(a, RA, 7)

RRC_R(b, RB, 0)
RRC_R(c, RC, 1)
RRC_R(d, RD, 2)
RRC_R(e, RE, 3)
RRC_R(h, RH, 4)
RRC_R(l, RL, 5)
RRC_R(a, RA, 7)

RL_R(b, RB, 0)
RL_R(c, RC, 1)
RL_R(d, RD, 2)
RL_R(e, RE, 3)
RL_R(h, RH, 4)
RL_R(l, RL, 5)
RL_R(a, RA, 7)

RR_R(b, RB, 0)
RR_R(c, RC, 1)
RR_R(d, RD, 2)
RR_R(e, RE, 3)
RR_R(h, RH, 4)
RR_R(l, RL, 5)
RR_R(a, RA, 7)

SLA_R(b, RB, 0)
SLA_R(c, RC, 1)
SLA_R(d, RD, 2)
SLA_R(e, RE, 3)
SLA_R(h, RH, 4)
SLA_R(l, RL, 5)
SLA_R(a, RA, 7)

SRA_R(b, RB, 0)
SRA_R(c, RC, 1)
SRA_R(d, RD, 2)
SRA_R(e, RE, 3)
SRA_R(h, RH, 4)
SRA_R(l, RL, 5)
SRA_R(a, RA, 7)

SLL_R(b, RB, 0)
SLL_R(c, RC, 1)
SLL_R(d, RD, 2)
SLL_R(e, RE, 3)
SLL_R(h, RH, 4)
SLL_R(l, RL, 5)
SLL_R(a, RA, 7)

SRL_R(b, RB, 0)
SRL_R(c, RC, 1)
SRL_R(d, RD, 2)
SRL_R(e, RE, 3)
SRL_R(h, RH, 4)
SRL_R(l, RL, 5)
SRL_R(a, RA, 7)

SHRIHL(rlc, RLC, 0)
SHRIHL(rrc, RRC, 1)
SHRIHL(rl,  RLN, 2)
SHRIHL(rr,  RRN, 3)
SHRIHL(sla, SLA, 4)
SHRIHL(sra, SRA, 5)
SHRIHL(sll, SLL, 6)
SHRIHL(srl, SRL, 7)

#define BIT(r, n) \
  RF = (RF & ~(SF | ZF | NF)) | (r & SF) | (((~r >> n) & 0x01) << 6)

#define BIT_N_R(bn, rn, r, n) \
OPDEF(bit_ ## bn ## _ ## rn, 0x40+bn*8+n) \
{                                         \
  BIT(r, bn);                             \
  ENTIME(8);                              \
}

#define BIT_N_IHL(bn) \
OPDEF(bit_ ## bn ## _ihl, 0x46+bn*8) \
{                                         \
  register byte btmp;                     \
  btmp = READ(GHL);                       \
  BIT(btmp, bn);                          \
  ENTIME(12);                             \
}

BIT_N_R(0, b, RB, 0)
BIT_N_R(0, c, RC, 1)
BIT_N_R(0, d, RD, 2)
BIT_N_R(0, e, RE, 3)
BIT_N_R(0, h, RH, 4)
BIT_N_R(0, l, RL, 5)
BIT_N_R(0, a, RA, 7)

BIT_N_R(1, b, RB, 0)
BIT_N_R(1, c, RC, 1)
BIT_N_R(1, d, RD, 2)
BIT_N_R(1, e, RE, 3)
BIT_N_R(1, h, RH, 4)
BIT_N_R(1, l, RL, 5)
BIT_N_R(1, a, RA, 7)

BIT_N_R(2, b, RB, 0)
BIT_N_R(2, c, RC, 1)
BIT_N_R(2, d, RD, 2)
BIT_N_R(2, e, RE, 3)
BIT_N_R(2, h, RH, 4)
BIT_N_R(2, l, RL, 5)
BIT_N_R(2, a, RA, 7)

BIT_N_R(3, b, RB, 0)
BIT_N_R(3, c, RC, 1)
BIT_N_R(3, d, RD, 2)
BIT_N_R(3, e, RE, 3)
BIT_N_R(3, h, RH, 4)
BIT_N_R(3, l, RL, 5)
BIT_N_R(3, a, RA, 7)

BIT_N_R(4, b, RB, 0)
BIT_N_R(4, c, RC, 1)
BIT_N_R(4, d, RD, 2)
BIT_N_R(4, e, RE, 3)
BIT_N_R(4, h, RH, 4)
BIT_N_R(4, l, RL, 5)
BIT_N_R(4, a, RA, 7)

BIT_N_R(5, b, RB, 0)
BIT_N_R(5, c, RC, 1)
BIT_N_R(5, d, RD, 2)
BIT_N_R(5, e, RE, 3)
BIT_N_R(5, h, RH, 4)
BIT_N_R(5, l, RL, 5)
BIT_N_R(5, a, RA, 7)

BIT_N_R(6, b, RB, 0)
BIT_N_R(6, c, RC, 1)
BIT_N_R(6, d, RD, 2)
BIT_N_R(6, e, RE, 3)
BIT_N_R(6, h, RH, 4)
BIT_N_R(6, l, RL, 5)
BIT_N_R(6, a, RA, 7)

BIT_N_R(7, b, RB, 0)
BIT_N_R(7, c, RC, 1)
BIT_N_R(7, d, RD, 2)
BIT_N_R(7, e, RE, 3)
BIT_N_R(7, h, RH, 4)
BIT_N_R(7, l, RL, 5)
BIT_N_R(7, a, RA, 7)

BIT_N_IHL(0)
BIT_N_IHL(1)
BIT_N_IHL(2)
BIT_N_IHL(3)
BIT_N_IHL(4)
BIT_N_IHL(5)
BIT_N_IHL(6)
BIT_N_IHL(7)

#define RES(r, n) r &= ~(1 << n)

#define RES_N_R(bn, rn, r, n) \
OPDEF(res_ ## bn ## _ ## rn, 0x80+bn*8+n) \
{                                         \
  RES(r, bn);                             \
  ENTIME(8);                              \
}

#define RES_N_IHL(bn) \
OPDEF(res_ ## bn ## _ihl, 0x86+bn*8) \
{                                         \
  register byte btmp;                     \
  btmp = READ(GHL);                       \
  RES(btmp, bn);                          \
  WRITE(GHL, btmp);                       \
  ENTIME(15);                             \
}


RES_N_R(0, b, RB, 0)
RES_N_R(0, c, RC, 1)
RES_N_R(0, d, RD, 2)
RES_N_R(0, e, RE, 3)
RES_N_R(0, h, RH, 4)
RES_N_R(0, l, RL, 5)
RES_N_R(0, a, RA, 7)

RES_N_R(1, b, RB, 0)
RES_N_R(1, c, RC, 1)
RES_N_R(1, d, RD, 2)
RES_N_R(1, e, RE, 3)
RES_N_R(1, h, RH, 4)
RES_N_R(1, l, RL, 5)
RES_N_R(1, a, RA, 7)

RES_N_R(2, b, RB, 0)
RES_N_R(2, c, RC, 1)
RES_N_R(2, d, RD, 2)
RES_N_R(2, e, RE, 3)
RES_N_R(2, h, RH, 4)
RES_N_R(2, l, RL, 5)
RES_N_R(2, a, RA, 7)

RES_N_R(3, b, RB, 0)
RES_N_R(3, c, RC, 1)
RES_N_R(3, d, RD, 2)
RES_N_R(3, e, RE, 3)
RES_N_R(3, h, RH, 4)
RES_N_R(3, l, RL, 5)
RES_N_R(3, a, RA, 7)

RES_N_R(4, b, RB, 0)
RES_N_R(4, c, RC, 1)
RES_N_R(4, d, RD, 2)
RES_N_R(4, e, RE, 3)
RES_N_R(4, h, RH, 4)
RES_N_R(4, l, RL, 5)
RES_N_R(4, a, RA, 7)

RES_N_R(5, b, RB, 0)
RES_N_R(5, c, RC, 1)
RES_N_R(5, d, RD, 2)
RES_N_R(5, e, RE, 3)
RES_N_R(5, h, RH, 4)
RES_N_R(5, l, RL, 5)
RES_N_R(5, a, RA, 7)

RES_N_R(6, b, RB, 0)
RES_N_R(6, c, RC, 1)
RES_N_R(6, d, RD, 2)
RES_N_R(6, e, RE, 3)
RES_N_R(6, h, RH, 4)
RES_N_R(6, l, RL, 5)
RES_N_R(6, a, RA, 7)

RES_N_R(7, b, RB, 0)
RES_N_R(7, c, RC, 1)
RES_N_R(7, d, RD, 2)
RES_N_R(7, e, RE, 3)
RES_N_R(7, h, RH, 4)
RES_N_R(7, l, RL, 5)
RES_N_R(7, a, RA, 7)

RES_N_IHL(0)
RES_N_IHL(1)
RES_N_IHL(2)
RES_N_IHL(3)
RES_N_IHL(4)
RES_N_IHL(5)
RES_N_IHL(6)
RES_N_IHL(7)


#define SET(r, n) r |= (1 << n)

#define SET_N_R(bn, rn, r, n) \
OPDEF(set_ ## bn ## _ ## rn, 0xC0+bn*8+n) \
{                                         \
  SET(r, bn);                             \
  ENTIME(8);                              \
}

#define SET_N_IHL(bn) \
OPDEF(set_ ## bn ## _ihl, 0x86+bn*8) \
{                                         \
  register byte btmp;                     \
  btmp = READ(GHL);                       \
  SET(btmp, bn);                          \
  WRITE(GHL, btmp);                       \
  ENTIME(15);                             \
}


SET_N_R(0, b, RB, 0)
SET_N_R(0, c, RC, 1)
SET_N_R(0, d, RD, 2)
SET_N_R(0, e, RE, 3)
SET_N_R(0, h, RH, 4)
SET_N_R(0, l, RL, 5)
SET_N_R(0, a, RA, 7)

SET_N_R(1, b, RB, 0)
SET_N_R(1, c, RC, 1)
SET_N_R(1, d, RD, 2)
SET_N_R(1, e, RE, 3)
SET_N_R(1, h, RH, 4)
SET_N_R(1, l, RL, 5)
SET_N_R(1, a, RA, 7)

SET_N_R(2, b, RB, 0)
SET_N_R(2, c, RC, 1)
SET_N_R(2, d, RD, 2)
SET_N_R(2, e, RE, 3)
SET_N_R(2, h, RH, 4)
SET_N_R(2, l, RL, 5)
SET_N_R(2, a, RA, 7)

SET_N_R(3, b, RB, 0)
SET_N_R(3, c, RC, 1)
SET_N_R(3, d, RD, 2)
SET_N_R(3, e, RE, 3)
SET_N_R(3, h, RH, 4)
SET_N_R(3, l, RL, 5)
SET_N_R(3, a, RA, 7)

SET_N_R(4, b, RB, 0)
SET_N_R(4, c, RC, 1)
SET_N_R(4, d, RD, 2)
SET_N_R(4, e, RE, 3)
SET_N_R(4, h, RH, 4)
SET_N_R(4, l, RL, 5)
SET_N_R(4, a, RA, 7)

SET_N_R(5, b, RB, 0)
SET_N_R(5, c, RC, 1)
SET_N_R(5, d, RD, 2)
SET_N_R(5, e, RE, 3)
SET_N_R(5, h, RH, 4)
SET_N_R(5, l, RL, 5)
SET_N_R(5, a, RA, 7)

SET_N_R(6, b, RB, 0)
SET_N_R(6, c, RC, 1)
SET_N_R(6, d, RD, 2)
SET_N_R(6, e, RE, 3)
SET_N_R(6, h, RH, 4)
SET_N_R(6, l, RL, 5)
SET_N_R(6, a, RA, 7)

SET_N_R(7, b, RB, 0)
SET_N_R(7, c, RC, 1)
SET_N_R(7, d, RD, 2)
SET_N_R(7, e, RE, 3)
SET_N_R(7, h, RH, 4)
SET_N_R(7, l, RL, 5)
SET_N_R(7, a, RA, 7)

SET_N_IHL(0)
SET_N_IHL(1)
SET_N_IHL(2)
SET_N_IHL(3)
SET_N_IHL(4)
SET_N_IHL(5)
SET_N_IHL(6)
SET_N_IHL(7)
