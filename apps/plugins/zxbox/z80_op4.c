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
#include "z80_op4.h"
#include "z80_ari.h"
#endif

#define RET_CC(ccn, cc, n) \
OPDEF(ret_ ## ccn, 0xC0+n*8)       \
{                                  \
  if(!(cc)) {                      \
    ENTIME(5);                     \
  }                                \
  else {                           \
    POP(PC);                       \
    ENTIME(11);                    \
  }                                \
}

RET_CC(nz, !TESTZF, 0)
RET_CC(z,  TESTZF,  1)
RET_CC(nc, !TESTCF, 2)
RET_CC(c,  TESTCF,  3)
RET_CC(po, !TESTPF, 4)
RET_CC(pe, TESTPF,  5)
RET_CC(p,  !TESTSF, 6)
RET_CC(m,  TESTSF,  7)

#define POP_RR(rrn, rr, n) \
OPDEF(pop_ ## rrn, 0xC1+n*0x10)    \
{                                  \
  POP(rr);                         \
  ENTIME(10);                      \
}

POP_RR(bc, BC, 0)
POP_RR(de, DE, 1)
POP_RR(hl, HL, 2)
POP_RR(af, AF, 3)

OPDEF(ret, 0xC9)
{
  POP(PC);
  ENTIME(10);
}

OPDEF(exx, 0xD9)
{
  register dbyte dtmp;
  
  dtmp = BCBK, BCBK = BC, BC = dtmp;
  dtmp = DEBK, DEBK = DE, DE = dtmp;
  dtmp = HLBK, HLBK = HL, HL = dtmp;
  
  ENTIME(4);
}  
  
#define JP_RR(rrn, rr) \
OPDEF(jp_ ## rrn, 0xE9)            \
{                                  \
  PC = rr;                         \
  ENTIME(4);                       \
}

JP_RR(hl, HL)

#define LD_SP_RR(rrn, rr) \
OPDEF(ld_sp_ ## rrn, 0xF9)         \
{                                  \
  SP = rr;                         \
  ENTIME(6);                       \
}

LD_SP_RR(hl, HL)

#define JP_CC(ccn, cc, n) \
OPDEF(jp_ ## ccn ## _nn, 0xC2+n*8) \
{                                  \
  if(!(cc)) {                      \
    PC+=2;                         \
    ENTIME(10);                    \
  }                                \
  else {                           \
    PC = DREAD(PC);                \
    ENTIME(10);                    \
  }                                \
}

JP_CC(nz, !TESTZF, 0)
JP_CC(z,  TESTZF,  1)
JP_CC(nc, !TESTCF, 2)
JP_CC(c,  TESTCF,  3)
JP_CC(po, !TESTPF, 4)
JP_CC(pe, TESTPF,  5)
JP_CC(p,  !TESTSF, 6)
JP_CC(m,  TESTSF,  7)

OPDEF(jp_nn, 0xC3)
{
  PC = DREAD(PC);
  ENTIME(10);
}


OPDEF(out_in_a, 0xD3)
{
  OUT(RA, *PCP, RA);
  PC++;
  ENTIME(11);
}

OPDEF(in_a_in, 0xDB)
{
  IN(RA, *PCP, RA);
  PC++;
  ENTIME(11);
}

#define EX_ISP_RR(rrn, rr) \
OPDEF(ex_isp_ ## rrn, 0xE3)        \
{                                  \
  register dbyte dtmp;             \
  dtmp = DREAD(SP);                \
  DWRITE(SP, rr);                  \
  rr = dtmp;                       \
  ENTIME(19);                      \
}

EX_ISP_RR(hl, HL)

OPDEF(ex_de_hl, 0xEB)
{
  register dbyte dtmp;
  dtmp = DE;
  DE = HL;
  HL = dtmp;
  ENTIME(4);
}

OPDEF(di, 0xF3)
{
  DANM(iff1) = 0;
  DANM(iff2) = 0;
  DI_CHECK
  ENTIME(4);
}

OPDEF(ei, 0xFB)
{
  DANM(iff1) = 1;
  DANM(iff2) = 1;
  ENTIME(4);
}



#define CALL_CC(ccn, cc, n) \
OPDEF(call_ ## ccn ## _nn, 0xC4+n*8) \
{                                  \
  if(!(cc)) {                      \
    PC+=2;                         \
    ENTIME(10);                    \
  }                                \
  else {                           \
    register dbyte dtmp;           \
    dtmp = PC;                     \
    PC = DREAD(dtmp);              \
    dtmp += 2;                     \
    PUSH(dtmp);                    \
    ENTIME(17);                    \
  }                                \
}

CALL_CC(nz, !TESTZF, 0)
CALL_CC(z,  TESTZF,  1)
CALL_CC(nc, !TESTCF, 2)
CALL_CC(c,  TESTCF,  3)
CALL_CC(po, !TESTPF, 4)
CALL_CC(pe, TESTPF,  5)
CALL_CC(p,  !TESTSF, 6)
CALL_CC(m,  TESTSF,  7)



#define PUSH_RR(rrn, rr, n) \
OPDEF(push_ ## rrn, 0xC5+n*0x10)   \
{                                  \
  PUSH(rr);                        \
  ENTIME(11);                      \
}

PUSH_RR(bc, BC, 0)
PUSH_RR(de, DE, 1)
PUSH_RR(hl, HL, 2)
PUSH_RR(af, AF, 3)


OPDEF(call_nn, 0xCD)
{
  register dbyte dtmp;
  dtmp = PC;
  PC = DREAD(dtmp);
  dtmp += 2;
  PUSH(dtmp);
  ENTIME(17);
}


OPDEF(add_a_n, 0xC6)
{
  ADD(*PCP);
  PC++;
  ENTIME(7);
}


OPDEF(adc_a_n, 0xCE)
{
  ADC(*PCP);
  PC++;
  ENTIME(7);
}

OPDEF(sub_n, 0xD6)
{
  SUB(*PCP);
  PC++;
  ENTIME(7);
}


OPDEF(sbc_a_n, 0xDE)
{
  SBC(*PCP);
  PC++;
  ENTIME(7);
}

OPDEF(and_n, 0xE6)
{
  AND(*PCP);
  PC++;
  ENTIME(7);
}


OPDEF(xor_n, 0xEE)
{
  XOR(*PCP);
  PC++;
  ENTIME(7);
}

OPDEF(or_n, 0xF6)
{
  OR(*PCP);
  PC++;
  ENTIME(7);
}


OPDEF(cp_n, 0xFE)
{
  CP(*PCP);
  PC++;
  ENTIME(7);
}

#define RST_NN(nnn, n) \
OPDEF(rst_ ## nnn, 0xC7+n*8)       \
{                                  \
  PUSH(PC);                        \
  PC = n*8;                        \
  ENTIME(11);                      \
}

RST_NN(00, 0)
RST_NN(08, 1)
RST_NN(10, 2)
RST_NN(18, 3)
RST_NN(20, 4)
RST_NN(28, 5)
RST_NN(30, 6)
RST_NN(38, 7)

#include "z80_op4x.c"
