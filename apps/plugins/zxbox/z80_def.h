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

#include "z80.h"

#ifdef PROCP

typedef Z80 *z80t;

#define OPDEF(name, num) z80t z80op_ ## name (z80t z80p)
#define ENDOP() return z80p 
#undef DANM
#define DANM(x) z80p->x

#define TAB(t) DANM(t)
#define PORT(t) DANM(t)
#define SPECP(t) DANM(t)

#else

typedef void z80t;

#define OPDEF(name, num) void z80op_ ## name (z80t)
#define ENDOP() 

#define TAB(t) z80c_ ## t
#define PORT(t) PRNM(t)
#define SPECP(t) SPNM(t)

#endif


typedef z80t (*op_f)(z80t);
extern op_f z80c_op_tab[];
extern op_f z80c_op_tab_cb[];
extern op_f z80c_op_tab_dd[];
extern op_f z80c_op_tab_ed[];
extern op_f z80c_op_tab_fd[];

#define TIME(x) DANM(tc) -= (x)

#define ENTIME(x) { TIME(x); ENDOP(); }

#define READ(addr)  (DANM(mem)[addr])
#define WRITE(addr, val) PUTMEM(addr,  DANM(mem) + (dbyte) (addr), val)

#define DREAD(addr) (DANM(mem)[addr] | (DANM(mem)[(dbyte)(addr+1)] << 8))
#define DWRITE(addr, x) WRITE(addr, (byte) x); \
                        WRITE((dbyte) (addr+1), (byte) (x >> 8))

#define IPCS        ((sbyte) *PCP)


#define MODMEM(func) \
{                                        \
  register byte bdef;                    \
  bdef = *HLP;                           \
  func(bdef);                            \
  PUTMEM(HL, HLP, bdef);                 \
}

#define MODMEMADDR(func, addr) \
{                                        \
  register byte bdef;                    \
  bdef = READ(addr);                     \
  func(bdef);                            \
  WRITE(addr, bdef);                     \
}

#define IXDGET(ixy, addr) addr = ((dbyte) (ixy + IPCS)), PC++

#define FETCHD(x) \
{                                        \
  register dbyte ddef;                   \
  ddef = PC;                             \
  x = DREAD(ddef);                       \
  PC = ddef+2;                           \
}

#define POP(x) \
{                                        \
  register dbyte ddef;                   \
  ddef = SP;                             \
  x = DREAD(ddef);                       \
  SP = ddef+2;                           \
}

#define PUSH(x) \
{                                        \
  register dbyte ddef, dval;             \
  dval = x;                              \
  ddef = SP-2;                           \
  DWRITE(ddef, dval);                    \
  SP = ddef;                             \
}

#ifdef SPECT_MEM
#include "sp_def.h"
#else

#define PUTMEM(addr, ptr, val) *(ptr) = (val)
#define IN(porth, portl, dest) dest = PORT(inports)[portl]
#define OUT(porth, portl, source) PORT(outports)[portl] = (source)
#define DI_CHECK

#endif


#define SF  0x80
#define ZF  0x40
#define B5F 0x20
#define HF  0x10
#define B3F 0x08
#define PVF 0x04
#define NF  0x02
#define CF  0x01

#define ALLF (SF | ZF | HF | PVF | NF | CF)
#define BUTCF (SF | ZF | HF | PVF | NF)

#define AALLF 0xff
#define ABUTCF 0xfe

#define SETFLAGS(mask, val) (RF = (RF & ~(mask)) | (val))
#define SET_FL(x)  (RF |= (x))
#define CLR_FL(x)  (RF &= ~(x))


#define TESTZF        (RF & ZF)
#define TESTCF        (RF & CF)
#define TESTSF        (RF & SF)
#define TESTPF        (RF & PVF)
#define TESTHF        (RF & HF)
#define TESTNF        (RF & NF)


#define IDXCALC(v1, v2, res) \
  ((res & 0xA8) | ((v1 & 0x88) >> 1) | ((v2 & 0x88) >> 3))

#define DIDXCALC(v1, v2, res) \
  (((res & 0x8800) >> 8) | ((v1 & 0x8800) >> 9) | ((v2 & 0x8800) >> 11))


extern byte z80c_incf_tbl[];
extern byte z80c_decf_tbl[];
extern byte z80c_addf_tbl[];
extern byte z80c_subf_tbl[];
extern byte z80c_orf_tbl[];
