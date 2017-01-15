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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "zxconfig.h"
#include "helpers.h"
Z80 PRNM(proc) IBSS_ATTR;

byte PRNM(inports)[PORTNUM] IBSS_ATTR;
byte PRNM(outports)[PORTNUM] IBSS_ATTR;

#ifdef SPECT_MEM
#define NUM64KSEGS 3
#endif

#ifndef NUM64KSEGS
#define NUM64KSEGS 1
#endif

static byte *a64kmalloc(int num64ksegs)
{
  byte *bigmem;
  
  bigmem = (byte *) my_malloc((unsigned) (0x10000 * (num64ksegs + 1)));
  if(bigmem == NULL) {
    /*fprintf(stderr, "Out of memory!\n");*/
    /*exit(1);*/
  }

  return (byte *) (( (intptr_t) bigmem & ~((intptr_t) 0xFFFF)) + 0x10000);
}



void PRNM(init)(void) 
{
  qbyte i;

  DANM(mem) = a64kmalloc(NUM64KSEGS);

  rb->srand((unsigned int)( rb->get_time()->tm_sec+rb->get_time()->tm_min*60 + rb->get_time()->tm_hour*3600));
  for(i = 0; i < 0x10000; i++) DANM(mem)[i] = (byte) rb->rand();

  for(i = 0; i < NUMDREGS; i++) {
    DANM(nr)[i].p = DANM(mem);
    DANM(nr)[i].d.d = (dbyte) rb->rand();
  }

  for(i = 0; i < BACKDREGS; i++) {
    DANM(br)[i].p = DANM(mem);
    DANM(br)[i].d.d = (dbyte) rb->rand();
  }

  for(i = 0; i < PORTNUM; i++) PRNM(inports)[i] = PRNM(outports)[i] = 0;

  PRNM(local_init)();

  return;
}

/* TODO: no interrupt immediately afer EI (not important for spectrum) */

void PRNM(nmi)(void)
{
  DANM(iff2) = DANM(iff1);
  DANM(iff1) = 0;

  DANM(haltstate) = 0;
  PRNM(pushpc)();

  PC = 0x0066;
}

/* TODO: IM 0 emulation */

void PRNM(interrupt)(int data)
{
  if(DANM(iff1)) {

    DANM(haltstate) = 0;
    DANM(iff1) = DANM(iff2) = 0;

    switch(DANM(it_mode)) {
    case 0:
      PRNM(pushpc)();
      PC = 0x0038;
      break;
    case 1:
      PRNM(pushpc)();
      PC = 0x0038;
      break;
    case 2:
      PRNM(pushpc)();
      PCL = DANM(mem)[(dbyte) (((int) RI << 8) + (data & 0xFF))];
      PCH = DANM(mem)[(dbyte) (((int) RI << 8) + (data & 0xFF) + 1)];
      break;
    }
  }
}


void PRNM(reset)(void)
{
  DANM(haltstate) = 0;
  DANM(iff1) = DANM(iff2) = 0;
  DANM(it_mode) = 0;
  RI = 0;
  RR = 0;
  PC = 0;
}
