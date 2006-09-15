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
#include "z80_op5.h"
#endif

OPDEF(ill_ed, 0x00)
{
#ifdef DEBUG_Z80
  printf("ILL_ED: %04X - %02X\n", (dbyte) (PC-1), DANM(mem)[(dbyte) (PC-1)]);
#endif

  ENTIME(8);
}

#define IN_R_IC(rn, r, n) \
OPDEF(in_ ## rn ## _ic, 0x40+8*n)  \
{                                  \
  register byte res, flag;         \
  IN(RB, RC, res);                 \
  r = res;                         \
  flag = (RF & ~(ABUTCF)) |        \
         TAB(orf_tbl)[res];        \
  RF = flag;                       \
  ENTIME(12);                      \
}

IN_R_IC(b, RB, 0)
IN_R_IC(c, RC, 1)
IN_R_IC(d, RD, 2)
IN_R_IC(e, RE, 3)
IN_R_IC(h, RH, 4)
IN_R_IC(l, RL, 5)
IN_R_IC(f, res, 6)
IN_R_IC(a, RA, 7)


#define OUT_IC_R(rn, r, n) \
OPDEF(out_ic_ ## rn, 0x41+8*n)     \
{                                  \
  OUT(RB, RC, r);                  \
  ENTIME(12);                      \
}

OUT_IC_R(b, RB, 0)
OUT_IC_R(c, RC, 1)
OUT_IC_R(d, RD, 2)
OUT_IC_R(e, RE, 3)
OUT_IC_R(h, RH, 4)
OUT_IC_R(l, RL, 5)
OUT_IC_R(0, 0,  6)
OUT_IC_R(a, RA, 7)



#define SBC_HL_RR(rrn, rr, n) \
OPDEF(sbc_hl_ ## rrn, 0x42+0x10*n) \
{                                  \
  register dbyte res;              \
  register int idx, flag;          \
  flag = RF;                       \
  res = HL - rr - (flag & CF);     \
  idx = DIDXCALC(HL, rr, res);     \
  HL = res;                        \
  flag = (RF & ~(ALLF)) |          \
      (TAB(subf_tbl)[idx] & ALLF); \
  if(!res) flag |= ZF;             \
  RF = flag;                       \
  ENTIME(15);                      \
}

SBC_HL_RR(bc, BC, 0)
SBC_HL_RR(de, DE, 1)
SBC_HL_RR(hl, HL, 2)
SBC_HL_RR(sp, SP, 3)

#define ADC_HL_RR(rrn, rr, n) \
OPDEF(adc_hl_ ## rrn, 0x4A+0x10*n) \
{                                  \
  register dbyte res;              \
  register int idx, flag;          \
  flag = RF;                       \
  res = HL + rr + (flag & CF);     \
  idx = DIDXCALC(HL, rr, res);     \
  HL = res;                        \
  flag = (RF & ~(ALLF)) |          \
      (TAB(addf_tbl)[idx] & ALLF); \
  if(!res) flag |= ZF;             \
  RF = flag;                       \
  ENTIME(15);                      \
}

ADC_HL_RR(bc, BC, 0)
ADC_HL_RR(de, DE, 1)
ADC_HL_RR(hl, HL, 2)
ADC_HL_RR(sp, SP, 3)



#define LD_INN_RR(rrn, pf, rr, n) \
OPDEF(ld_inn_ ## rrn ## pf, 0x43+0x10*n) \
{                                  \
  register dbyte dtmp;             \
  FETCHD(dtmp);                    \
  DWRITE(dtmp, rr);                \
  ENTIME(20);                      \
}


LD_INN_RR(bc,    , BC, 0)
LD_INN_RR(de,    , DE, 1)
LD_INN_RR(hl, _ed, HL, 2)
LD_INN_RR(sp,    , SP, 3)


#define LD_RR_INN(rrn, pf, rr, n) \
OPDEF(ld_## rrn ## _inn ## pf, 0x4B+0x10*n) \
{                                  \
  register dbyte dtmp;             \
  FETCHD(dtmp);                    \
  rr = DREAD(dtmp);                \
  ENTIME(20);                      \
}


LD_RR_INN(bc,    , BC, 0)
LD_RR_INN(de,    , DE, 1)
LD_RR_INN(hl, _ed, HL, 2)
LD_RR_INN(sp,    , SP, 3)

OPDEF(neg, 0x44 0x4C 0x54 0x5C 0x64 0x6C 0x74 0x7C)
{
  register byte res;
  register int idx;
  register int flag;
  
  res = 0 - RA;
  idx = IDXCALC(0, RA, res);
  RA = res;
  flag = (RF & ~(AALLF)) | TAB(subf_tbl)[idx];
  if(!res) flag |= ZF;
  RF = flag;
  ENTIME(8);
}

OPDEF(retn, 0x45 0x55 0x5D 0x65 0x6D 0x75 0x7D)
{
  DANM(iff1) = DANM(iff2);
  POP(PC);
  ENTIME(14);
}


OPDEF(reti, 0x4D)
{
  POP(PC);
  ENTIME(14);
}

OPDEF(im_0, 0x46 0x4E 0x56 0x5E)
{
  DANM(it_mode) = 0;
  ENTIME(8);
}


OPDEF(im_1, 0x56 0x76)
{
  DANM(it_mode) = 1;
  ENTIME(8);
}


OPDEF(im_2, 0x5E 0x7E)
{
  DANM(it_mode) = 2;
  ENTIME(8);
}

OPDEF(ld_i_a, 0x47)
{
  RI = RA;
  ENTIME(9);
}

OPDEF(ld_r_a, 0x4F)
{
  DANM(rl7) = RA & 0x80;
  RR = RA;
  ENTIME(9);
}

OPDEF(ld_a_i, 0x57)
{
  register int flag;

  RA = RI;
  flag = (RF & ~(BUTCF)) | (RA & SF);
  if(!RA) flag |= ZF;
  if(DANM(iff2)) flag |= PVF;
  RF = flag;
  
  ENTIME(9);
}

OPDEF(ld_a_r, 0x5F)
{
  register int flag;

  RA = (RR & 0x7F) | DANM(rl7);
  flag = (RF & ~(BUTCF)) | (RA & SF);
  if(!RA) flag |= ZF;
  if(DANM(iff2)) flag |= PVF;
  RF = flag;
  
  ENTIME(9);
}

OPDEF(rrd, 0x67)
{
  register dbyte dtmp;

  dtmp = *HLP | (RA << 8);
  RA = (RA & 0xF0) | (dtmp & 0x0F);
  SETFLAGS(ABUTCF, TAB(orf_tbl)[RA]);
  dtmp >>= 4;
  PUTMEM(HL, HLP, (byte) dtmp);
  
  ENTIME(18);
}

OPDEF(rld, 0x6F)
{
  register dbyte dtmp;
  
  dtmp = (*HLP << 4) | (RA & 0x0F);
  RA = (RA & 0xF0) | ((dtmp >> 8) & 0x0F);
  SETFLAGS(ABUTCF, TAB(orf_tbl)[RA]);
  PUTMEM(HL, HLP, (byte) dtmp);

  ENTIME(18);
}

#define NOREPEAT() \
  if(BC) RF |= PVF;                \
  ENTIME(16)

#define LDREPEAT() \
  if(BC) {                         \
    RF |= PVF;                     \
    PC-=2;                         \
    ENTIME(21);                    \
  }                                \
  else {                           \
    ENTIME(16);                    \
  }


#define LDID(dir) \
  register byte res;            \
  res = *HLP;                   \
  PUTMEM(DE, DEP, res);         \
  DE dir, HL dir;               \
  RF = RF & ~(HF | PVF | NF);   \
  BC--

OPDEF(ldi, 0xA0)
{
  LDID(++);
  NOREPEAT();
}

OPDEF(ldd, 0xA8)
{
  LDID(--);
  NOREPEAT();
}

OPDEF(ldir, 0xB0)
{
  LDID(++);
  LDREPEAT();
}


OPDEF(lddr, 0xB8)
{
  LDID(--);
  LDREPEAT();
}

#define CPREPEAT() \
  if(res && BC) {                  \
    RF |= PVF;                     \
    PC-=2;                         \
    ENTIME(21);                    \
  }                                \
  else {                           \
    ENTIME(16);                    \
  }

#define CPID(dir) \
  register byte res;                   \
  register int idx;                    \
  res = RA - *HLP;                     \
  idx = IDXCALC(RA, *HLP, res);        \
  RF = (RF & ~BUTCF) |                 \
         (TAB(subf_tbl)[idx] &         \
         (SF | HF | NF | B3F | B5F));  \
  if(!res) RF |= ZF;                   \
  HL dir;                              \
  BC--


OPDEF(cpi, 0xA1)
{
  CPID(++);
  NOREPEAT();
}


OPDEF(cpd, 0xA9)
{
  CPID(--);
  NOREPEAT();
}


OPDEF(cpir, 0xB1)
{
  CPID(++);
  CPREPEAT();
}

OPDEF(cpdr, 0xB9)
{
  CPID(--);
  CPREPEAT();
}

#define IOREPEAT() \
  if(RB) {                         \
    PC-=2;                         \
    ENTIME(21);                    \
  }                                \
  else {                           \
    ENTIME(16);                    \
  }


#define INID(dir) \
  register byte res;               \
  register int idx;                \
  res = RB - 1;                    \
  idx = IDXCALC(RB, 1, res);       \
  RF = (RF & ~BUTCF) |             \
       (TAB(subf_tbl)[idx] &       \
        (SF | HF | PVF | NF | B3F | B5F));    \
  if(!res) RF |= ZF;               \
  RB = res;                        \
  IN(RB, RC, res);                 \
  PUTMEM(HL, HLP, res);            \
  HL dir

 
OPDEF(ini, 0xA2)
{
  INID(++);
  ENTIME(16);
}

OPDEF(ind, 0xAA)
{
  INID(--);
  ENTIME(16);
}

OPDEF(inir, 0xB2)
{
  INID(++);
  IOREPEAT();
}

OPDEF(indr, 0xBA)
{
  INID(--);
  IOREPEAT();
}



#define OUTID(dir) \
  register byte res;               \
  register int idx;                \
  res = RB - 1;                    \
  idx = IDXCALC(RB, 1, res);       \
  RF = (RF & ~BUTCF) |             \
       (TAB(subf_tbl)[idx] &       \
        (SF | HF | PVF | NF ));    \
  if(!res) RF |= ZF;               \
  RB = res;                        \
  OUT(RB, RC, *HLP);               \
  HL dir



OPDEF(outi, 0xA3)
{
  OUTID(++);
  ENTIME(16);
}

OPDEF(outd, 0xAB)
{
  OUTID(--);
  ENTIME(16);
}


OPDEF(otir, 0xB3)
{
  OUTID(++);
  IOREPEAT();
}

OPDEF(otdr, 0xBB)
{
  OUTID(--);
  IOREPEAT();
}
