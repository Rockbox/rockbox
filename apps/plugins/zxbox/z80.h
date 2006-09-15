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

#ifndef Z80_H
#define Z80_H

#include "zxconfig.h"
#include "config.h"

#ifdef ROCKBOX_BIG_ENDIAN
#define WORDS_BIGENDIAN
#endif

#ifndef COMPARISON
#define PRNM(x) z80_ ## x
#else 
#define PRNM(x) z80x_ ## x
#endif

#define DANM(x) PRNM(proc).x

#include "z80_type.h"


#ifndef WORDS_BIGENDIAN
union dregp {
  struct { byte l, h, _b2, _b3; } s;
  struct { dbyte d, _d1; }        d;
  byte*                           p;
};
#else
union dregp {
  struct { byte _b3, _b2, h, l; } s;
  struct { dbyte _d1, d; }        d;
  byte*                           p;
};
#endif

#define NUMDREGS  9
#define BACKDREGS 4

#define PORTNUM 256

/* Do NOT change the order! */
typedef struct {
  union dregp nr[NUMDREGS];
  union dregp br[BACKDREGS];
  
  int haltstate;
  int it_mode;
  int iff1, iff2;
  
  byte *mem;

  int tc;
  int rl7;

#ifdef SPECT_MEM       /* WARNING: Do NOT change the order!!! */
  int next_scri;
  int inport_mask;
  int ula_inport;
  int ula_outport;
  int sound_sam;
  int sound_change;
  int imp_change;
#endif

#ifdef Z80C
  dbyte cbaddr;

#ifdef PROCP
  byte *incf_tbl;
  byte *decf_tbl;
  byte *addf_tbl;
  byte *subf_tbl;
  byte *orf_tbl;

  byte *inports;
  byte *outports;
#ifdef SPECT_MEM
  byte *fe_inport_high;
#endif
#endif
#endif

} Z80;


extern Z80 PRNM(proc);

extern byte PRNM(inports)[];
extern byte PRNM(outports)[];

#define ZI_BC 0
#define ZI_DE 1
#define ZI_HL 2
#define ZI_AF 3
#define ZI_IR 4
#define ZI_IX 5
#define ZI_IY 6
#define ZI_PC 7
#define ZI_SP 8


#define BC  (DANM(nr)[ZI_BC].d.d)
#define DE  (DANM(nr)[ZI_DE].d.d)
#define HL  (DANM(nr)[ZI_HL].d.d)
#define AF  (DANM(nr)[ZI_AF].d.d)
#define IR  (DANM(nr)[ZI_IR].d.d)
#define IX  (DANM(nr)[ZI_IX].d.d)
#define IY  (DANM(nr)[ZI_IY].d.d)
#define PC  (DANM(nr)[ZI_PC].d.d)
#define SP  (DANM(nr)[ZI_SP].d.d)

#define BCP (DANM(nr)[ZI_BC].p)
#define DEP (DANM(nr)[ZI_DE].p)
#define HLP (DANM(nr)[ZI_HL].p)
#define PCP (DANM(nr)[ZI_PC].p)
#define SPP (DANM(nr)[ZI_SP].p)
#define IXP (DANM(nr)[ZI_IX].p)
#define IYP (DANM(nr)[ZI_IY].p)


#define RB  (DANM(nr)[ZI_BC].s.h)
#define RC  (DANM(nr)[ZI_BC].s.l)
#define RD  (DANM(nr)[ZI_DE].s.h)
#define RE  (DANM(nr)[ZI_DE].s.l)
#define RH  (DANM(nr)[ZI_HL].s.h)
#define RL  (DANM(nr)[ZI_HL].s.l)
#define RA  (DANM(nr)[ZI_AF].s.h)
#define RF  (DANM(nr)[ZI_AF].s.l)
#define RI  (DANM(nr)[ZI_IR].s.h)
#define RR  (DANM(nr)[ZI_IR].s.l)
#define XH  (DANM(nr)[ZI_IX].s.h)
#define XL  (DANM(nr)[ZI_IX].s.l)
#define YH  (DANM(nr)[ZI_IY].s.h)
#define YL  (DANM(nr)[ZI_IY].s.l)
#define PCH (DANM(nr)[ZI_PC].s.h)
#define PCL (DANM(nr)[ZI_PC].s.l)
#define SPH (DANM(nr)[ZI_SP].s.h)
#define SPL (DANM(nr)[ZI_SP].s.l)

#define BCBK (DANM(br)[ZI_BC].d.d)
#define DEBK (DANM(br)[ZI_DE].d.d)
#define HLBK (DANM(br)[ZI_HL].d.d)
#define AFBK (DANM(br)[ZI_AF].d.d)

#define BBK  (DANM(br)[ZI_BC].s.h)
#define CBK  (DANM(br)[ZI_BC].s.l)
#define DBK  (DANM(br)[ZI_DE].s.h)
#define EBK  (DANM(br)[ZI_DE].s.l)
#define HBK  (DANM(br)[ZI_HL].s.h)
#define LBK  (DANM(br)[ZI_HL].s.l)
#define ABK  (DANM(br)[ZI_AF].s.h)
#define FBK  (DANM(br)[ZI_AF].s.l)

#ifdef __cplusplus
extern "C" {
#endif
 
extern void PRNM(init)(void);
extern int  PRNM(step)(int ticknum);

extern void PRNM(interrupt)(int data);
extern void PRNM(nmi)(void);
extern void PRNM(reset)(void);

extern void PRNM(pushpc)(void);
extern void PRNM(local_init)(void);

#ifdef __cplusplus
}
#endif


#endif /* Z80_H */
