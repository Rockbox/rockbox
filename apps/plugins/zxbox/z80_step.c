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

#include "z80_def.h"

byte z80c_incf_tbl[256];
byte z80c_decf_tbl[256];
byte z80c_addf_tbl[256];
byte z80c_subf_tbl[256];
byte z80c_orf_tbl[256];


void PRNM(pushpc)(void)
{
#ifdef PROCP
  Z80 *z80p;
  z80p = &PRNM(proc);
#endif

  SP--;
  PUTMEM(SP, SPP, PCH);
  SP--;
  PUTMEM(SP, SPP, PCL);
}

static int parity(int b)
{
  int i;
  int par;
  
  par = 0;
  for(i = 8; i; i--) par ^= (b & 1), b >>= 1;
  return par;
}

void PRNM(local_init)(void)
{
  int i;

#ifdef PROCP
  Z80 *z80p;
  z80p = &PRNM(proc);
#endif

  for(i = 0; i < 0x100; i++) {
    z80c_incf_tbl[i] = z80c_decf_tbl[i] = z80c_orf_tbl[i] = 0;

    z80c_orf_tbl[i] |= i & (SF | B3F | B5F);
    z80c_incf_tbl[i] |= i & (SF | B3F | B5F);
    z80c_decf_tbl[i] |= i & (SF | B3F | B5F);

    if(!parity(i)) z80c_orf_tbl[i] |= PVF;
  }

  z80c_incf_tbl[0] |= ZF;
  z80c_decf_tbl[0] |= ZF;
  z80c_orf_tbl[0] |= ZF;

  z80c_incf_tbl[0x80] |= PVF;
  z80c_decf_tbl[0x7F] |= PVF;

  for(i = 0; i < 0x100; i++) {
    int cr, c1, c2;
    int hr, h1, h2;
    int b5r;
    
    cr  = i & 0x80;
    c1  = i & 0x40;
    b5r = i & 0x20;
    c2  = i & 0x10;

    hr  = i & 0x08;
    h1  = i & 0x04;
    h2  = i & 0x01;

    z80c_addf_tbl[i] = 0;
    z80c_subf_tbl[i] = 0;
    if(cr) {
      z80c_addf_tbl[i] |= SF;
      z80c_subf_tbl[i] |= SF;
    }
    if(b5r) {
      z80c_addf_tbl[i] |= B5F;
      z80c_subf_tbl[i] |= B5F;
    }
    if(hr) {
      z80c_addf_tbl[i] |= B3F;
      z80c_subf_tbl[i] |= B3F;
    }
  
    if((c1 && c2) || (!cr && (c1 || c2))) z80c_addf_tbl[i] |= CF;
    if((h1 && h2) || (!hr && (h1 || h2))) z80c_addf_tbl[i] |= HF;

    if((!c1 && !c2 && cr) || (c1 && c2 && !cr)) z80c_addf_tbl[i] |= PVF;


    if((c2 && cr) || (!c1 && (c2 || cr))) z80c_subf_tbl[i] |= CF;
    if((h2 && hr) || (!h1 && (h2 || hr))) z80c_subf_tbl[i] |= HF;
    
    if((!c2 && !cr && c1) || (c2 && cr && !c1)) z80c_subf_tbl[i] |= PVF;

    
    z80c_subf_tbl[i] |= NF;
  }
  

#ifdef PROCP
  TAB(incf_tbl) = z80c_incf_tbl;
  TAB(decf_tbl) = z80c_decf_tbl;
  TAB(addf_tbl) = z80c_addf_tbl;
  TAB(subf_tbl) = z80c_subf_tbl;
  TAB(orf_tbl) = z80c_orf_tbl;

  PORT(inports) = PRNM(inports);
  PORT(outports) = PRNM(outports);
#ifdef SPECT_MEM
  SPECP(fe_inport_high) = SPNM(fe_inport_high);
#endif
#endif
}


int PRNM(step)(int tc)
{
#ifdef PROCP
  Z80 *z80p;
  z80p = &PRNM(proc);
#endif

  DANM(tc) = tc;
  DANM(rl7) = RR & 0x80;

  if(DANM(haltstate)) {
    register int nn;
    nn = (DANM(tc) - 1) / 4 + 1;
    
    DANM(tc) -= 4 * nn;
    RR += nn;
  }
  else do {
    register int nextop;

#ifdef DEBUG_Z80
    debug_z80();
#endif
    nextop = *PCP; 
    PC++;

#ifdef PROCP
    z80p = (*z80c_op_tab[nextop])(z80p);
#else
    (*z80c_op_tab[nextop])();
#endif
    RR++; 
  } while(DANM(tc) > 0);

  RR = (RR & 0x7F) | DANM(rl7);
  return DANM(tc);
}
