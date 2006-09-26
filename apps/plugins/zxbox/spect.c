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
#include "spperif.h"
#include "spscr.h"

#define SOUNDPORT 0x10

int SPNM(halfframe)(int firsttick, int numlines)
{

  int tc, linesleft;
  int feport, scline, border = 0;
  byte *scrptr;
  qbyte cmark = 0;

  scrptr = (byte *) SPNM(image);

  tc = firsttick - CHKTICK;

  for(linesleft = numlines; linesleft; linesleft--) {
    DANM(next_scri) = SPNM(scri)[SPNM(scline)];
    tc += CHKTICK;

    tc = PRNM(step)(tc);

    scline = SPNM(scline);

    /* store sound */
    SPNM(sound_buf)[scline] = DANM(sound_sam);
    feport = DANM(ula_outport);

    if(feport & SOUNDPORT) DANM(sound_sam) = 240;
    else DANM(sound_sam) = 16;

    /* change EAR bit, store MIC bit*/

    SPNM(fe_outport_time)[scline] = feport;
    if(DANM(imp_change)) DANM(ula_inport) ^= 0x40;
    DANM(imp_change) = SPNM(tape_impinfo)[scline];

    /* Check if updating screen */

    if(SPNM(updating)) {
#if LCD_WIDTH == 320 && ( LCD_HEIGHT == 240 || LCD_HEIGHT == 200 )
      border = SPNM(lastborder);
      if((feport & 0x07) != border) {
    SPNM(border_update) = 2;
    SPNM(lastborder) = feport & 0x07;
      }
#else
      SPNM(border_update) = 0;
#endif
      scrptr = update_screen_line(scrptr, SPNM(coli)[scline], DANM(next_scri),
                  border, &cmark);

    }
    SPNM(scline)++;
  }
  return tc;
}
