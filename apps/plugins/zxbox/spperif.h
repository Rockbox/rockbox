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

#ifndef SPPERIF_H
#define SPPERIF_H

#ifndef COMPARISON
#define SPNM(x) sp_ ## x
#else
#define SPNM(x) spx_ ## x
#endif

#include "z80_type.h"

#define ST 140000
#define CHKTICK 224

/* #define TMNUM (ST / CHKTICK) */

#if 0
#  define TMNUM 625
#  define EVENHF 313
#  define ODDHF  312
#else
#  define TMNUM 624
#  define EVENHF 312
#  define ODDHF  312
#endif


#define LOAD_DI    0x0559
#define SA_LD_RET  0x053F

extern int SPNM(quick_load);
extern int SPNM(load_trapped);

#define SCRMARK_SIZE 2048
extern qbyte SPNM(scr_mark)[];
extern byte SPNM(fe_inport_high)[];
extern int SPNM(playing_tape);

extern int SPNM(scline);

extern unsigned char SPNM(colors)[];

extern int SPNM(flash_state);

extern qbyte *SPNM(scr_f0_table);
extern qbyte *SPNM(scr_f1_table);

extern int SPNM(scri)[];
extern int SPNM(coli)[];

#define PORT_TIME_NUM 1024
extern byte SPNM(tape_impinfo)[];
extern byte SPNM(fe_inport_default);
extern byte SPNM(fe_outport_time)[];
extern signed char SPNM(tape_sound)[];
extern byte SPNM(sound_buf)[];


extern char *SPNM(image);
extern int SPNM(updating);
extern qbyte SPNM(imag_mark)[];
extern qbyte SPNM(imag_horiz);
extern qbyte SPNM(imag_vert);
extern int SPNM(border_update);
extern int SPNM(lastborder);

extern void SPNM(init_screen_mark)(void);
extern void SPNM(init)(void);
extern int SPNM(halfframe)(int firsttick, int numlines);

#endif /* SPPERIF_H */
