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

#include "spperif.h"

#define MARK_SCR(addr) SPNM(scr_mark)[(addr) >> 5] |= BIT_N((addr) & 0x1F)

#define PUTMEM(addr, ptr, val)  \
{                                                \
  register byte addrhi;                          \
  addrhi = (dbyte) (addr) >> 8;                  \
  if(addrhi >= 0x5B) *(ptr) = (val);             \
  else if(addrhi & 0x40) {                       \
    *(ptr) = (val);                              \
    MARK_SCR((dbyte) (addr));                    \
    if(DANM(next_scri) >= 0 && DANM(tc) > 86)    \
       DANM(tc) -= 2;                            \
  }                                              \
}

#define SOUNDPORT 0x10

/* TODO: attribute or pixel byte is present on unused ports? */


#define IN(porth, portl, dest) \
{                                                                \
    if(!((portl) & DANM(inport_mask))) {                         \
       dest = PORT(inports)[portl];                              \
    }                                                            \
    else if(!((portl) & 1)) {                                    \
       if(DANM(imp_change) > DANM(tc)) {                         \
          DANM(imp_change) = 0;                                  \
          DANM(ula_inport) ^= 0x40;                              \
       }                                                         \
       dest = SPECP(fe_inport_high)[porth] & DANM(ula_inport);   \
       DANM(tc) -= 1;                                            \
    }                                                            \
    else {                                                       \
       register int scri;                                        \
       scri = DANM(next_scri);                                   \
       dest = (scri < 0 || DANM(tc) <= 96)                       \
     ? 0xFF : DANM(mem)[(scri<<5)+((224-DANM(tc))>>2)];      \
    }                                                            \
}


#define OUT(porth, portl, source) \
{                                                                \
  if(!((portl) & 1)) {                                           \
     if((DANM(ula_outport) ^ (source)) & SOUNDPORT) {            \
       DANM(sound_change) = 1;                                   \
       if((source) & SOUNDPORT) DANM(sound_sam) += DANM(tc);     \
       else                     DANM(sound_sam) -= DANM(tc);     \
     }                                                           \
     DANM(ula_outport) = (source);                               \
     DANM(tc) -= 1;                                              \
  }                                                              \
  PORT(outports)[portl] = (source);                              \
}

#define DI_CHECK \
  if(PC == LOAD_DI+1 && SPNM(quick_load)) \
     SPNM(load_trapped) = 1,            \
     DANM(haltstate) = 1,               \
     DANM(tc) = 0;

