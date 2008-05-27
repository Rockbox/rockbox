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

#include "spscr_p.h"
#include "spscr.h"

#include "spperif.h"
#include "z80.h"


#include <stdlib.h>
#include <stdio.h>

#define TABOFFS 2

volatile int screen_visible = 1;
volatile int accept_keys = 1;


byte *update_screen_line(byte *scrp, int coli, int scri, int border,
             qbyte *cmarkp)
{
  qbyte *scrptr;
#if LCD_WIDTH == 320 && ( LCD_HEIGHT == 240 || LCD_HEIGHT == 200 )
  qbyte brd_color;
#else
  (void) border;
#endif
  int i;
  qbyte *tmptr, *mptr;
  qbyte mark, cmark;
  
  cmark = *cmarkp;
  scrptr = (qbyte *) scrp;
  if(scri >= 0) {          /* normal line */
#if LCD_WIDTH == 320 && ( LCD_HEIGHT == 240 || LCD_HEIGHT == 200 )
    if(SPNM(border_update)) {
      brd_color = SPNM(colors)[border];
      brd_color |= brd_color << 8;
      brd_color |= brd_color << 16;
      for(i = 8; i; i--) *scrptr++ = brd_color;
      scrptr += 0x40;
      for(i = 8; i; i--) *scrptr++ = brd_color;
      scrptr -= 0x48;
    }
    else
#endif
        scrptr += 0x08;

    tmptr = SPNM(scr_mark) + 0x2C0 + (coli >> 3);
    mark = *tmptr;
    if(!(coli & 0x07)) {
      cmark = mark;
      *tmptr = 0;
    }
    else cmark |= mark;
    mptr = SPNM(scr_mark) + scri;
    
    mark = *mptr | cmark;
    if(mark) {
      byte *spmp, *spcp;
      qbyte *scr_tab;
      
      *mptr = 0;
      SPNM(imag_mark)[coli] |= mark;
      SPNM(imag_horiz) |= mark;
      coli >>= 3;
      SPNM(imag_vert) |= (1 << coli);
      
      spmp = PRNM(proc).mem + (scri << 5);
      spcp = PRNM(proc).mem + 0x5800 + (coli << 5);

      if(!SPNM(flash_state)) scr_tab = SPNM(scr_f0_table);
      else scr_tab = SPNM(scr_f1_table);
      for(i = 32; i; i--) {
    register dbyte spcx, spmx;
    spcx = (*spcp++) << 6;
    spmx = *spmp++;
    *scrptr++ = scr_tab[spcx|((spmx & 0xf0) >> 4)];
    *scrptr++ = scr_tab[spcx|((spmx & 0x0f))];
      }
      scrptr +=0x08;
    }
    else scrptr += 0x48;
  }
  else if(scri == -1) {  /* only border */
#if LCD_WIDTH == 320 && ( LCD_HEIGHT == 240 || LCD_HEIGHT == 200 )
    if(SPNM(border_update)) {
      brd_color = SPNM(colors)[border];
      brd_color |= brd_color << 8;
      brd_color |= brd_color << 16;
      for(i = 0x50; i; i--) *scrptr++ = brd_color;
    }
    else
#endif
        scrptr += 0x50;

  }

  *cmarkp = cmark;
  return (byte *) scrptr;
}

void spscr_init_mask_color(void)
{
  int clb;
  int bwb;
  int hb;
  int ix, j;
  int bc, fc;
  byte *tab_f0, *tab_f1;

  sp_scr_f0_table = (qbyte *) (PRNM(proc).mem + 0x10000);
  sp_scr_f1_table = (qbyte *) (PRNM(proc).mem + 0x20000);

  sp_colors[8] = sp_colors[0];

  for(clb = 0; clb < 256; clb++) 
    for(hb = 0; hb < 16; hb++) {

      bc = (clb & 0x38) >> 3;
      fc = clb & 0x07;
      
      if(clb & 0x40) {
    fc |= 0x08;
    bc |= 0x08;
      }
      bwb = hb;

      ix = (clb << 8) + (hb << TABOFFS);
      tab_f0 = ((byte *) sp_scr_f0_table) + ix + 3;
      tab_f1 = ((byte *) sp_scr_f1_table) + ix + 3;

      for(j = 4; j; bwb >>= 1, j--)  {
    *tab_f0-- = (byte) sp_colors[(bwb & 0x01) ? fc : bc];
    *tab_f1-- = (byte) sp_colors[(clb & 0x80) ? 
                    ((bwb & 0x01) ? bc : fc) :
                    ((bwb & 0x01) ? fc : bc)];
      }
    }
}

void flash_change(void)
{
  int i,j;
  byte *scp;
  qbyte *mcp;
  register unsigned int val;

  mcp = sp_scr_mark + 0x2C0;
  scp = z80_proc.mem + 0x5800;

  for(i = 24; i; mcp++, i--) {
    val = 0;
    for(j = 32; j; scp++, j--) {
      val >>= 1;
      if(*scp & 0x80) val |= (1 << 31);
    }
    *mcp |= val;
  }
}

void spscr_init_line_pointers(int lines)
{
  int i;
  int bs;
  int y;
  int scline;
  
  bs = (lines - 192) / 2;

  for(i = 0; i < PORT_TIME_NUM; i++) {

    sp_scri[i] = -2;

    if(i < ODDHF) scline = i;
    else scline = i - ODDHF;
    
    if(scline >= 64-bs && scline < 256+bs) {
      if(scline >= 64 && scline < 256) {
    y = scline - 64;
    sp_coli[i] = y;
    sp_scri[i] = 0x200 +
      (y & 0xC0) + ((y & 0x07) << 3) + ((y & 0x38) >> 3);
      }
      else sp_scri[i] = -1;
    }
  }
}
