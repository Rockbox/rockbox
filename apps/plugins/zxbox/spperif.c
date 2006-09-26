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
#include "z80.h"

extern unsigned char rom_imag[];

int SPNM(quick_load) = 1;
int SPNM(load_trapped);

qbyte SPNM(scr_mark)[SCRMARK_SIZE];
byte SPNM(fe_inport_high)[256];

int SPNM(scline);


unsigned char SPNM(colors)[16];

int SPNM(flash_state);

qbyte *SPNM(scr_f0_table);
qbyte *SPNM(scr_f1_table);

byte SPNM(tape_impinfo)[PORT_TIME_NUM];
byte SPNM(fe_inport_default);
byte SPNM(fe_outport_time)[PORT_TIME_NUM];
byte SPNM(sound_buf)[PORT_TIME_NUM];
signed char SPNM(tape_sound)[TMNUM];
int SPNM(scri)[PORT_TIME_NUM];
int SPNM(coli)[PORT_TIME_NUM];

int SPNM(playing_tape) = 0;

char *SPNM(image);
int SPNM(updating);
qbyte SPNM(imag_mark)[192];
qbyte SPNM(imag_horiz);
qbyte SPNM(imag_vert);
int SPNM(border_update);
int SPNM(lastborder);

void SPNM(init_screen_mark)(void)
{
  int i;

  for(i = 0x200; i < 0x2D8; i++) SPNM(scr_mark)[i] = ~((qbyte) 0);

  SPNM(border_update) = 1;
}

void SPNM(init)(void)
{
  int i;

  PRNM(init)();
  PRNM(reset)();

  SPNM(load_trapped) = 0;

  for(i = 0; i < PORTNUM; i++) PRNM(inports)[i] = 0x00;

  DANM(inport_mask) = 0x20;
/* TODO: Kempston is always present, this is not nice */

  SPNM(fe_inport_default) = 0xBF; /* Issue 3 */

  DANM(ula_outport) = 0xFF;

  for(i = 0; i < 256; i++) SPNM(fe_inport_high)[i] = 0xFF;
  
  for(i = 0; i < PORT_TIME_NUM; i++) SPNM(tape_impinfo)[i] = 0;
  DANM(imp_change) = 0;
  DANM(ula_inport) = SPNM(fe_inport_default);

  SPNM(scline) = 0;
  
  for(i = 0; i < 0x4000; i++) PRNM(proc).mem[i] = rom_imag[i];

  SPNM(init_screen_mark)();
}
