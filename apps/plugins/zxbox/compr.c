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

#include "compr.h"

void compr(void) 
{
  int j, c, lc, lled, rep, num;

  rep = 0;
  
  c = compr_read_byte();
  lc = 0;
  num = 0;

  while(c >= 0) {
    if(lc == 0xED) lled = 1;
    else lled = 0;

    lc = c;
    c = compr_read_byte();
    if(c == lc && num != 255 && (!lled || rep)) {
      if(!rep) {
    num = 1;
    rep = 1;
      }
      num++;
    }
    else {
      if(rep) {
    if(num < 5 && lc != 0xED) for(j = 0; j < num; j++) compr_put_byte(lc);
    else{
      compr_put_byte(0xED);
      compr_put_byte(0xED);
      compr_put_byte(num);
      compr_put_byte(lc);
      num = 0;
    }
    rep = 0;
      }
      else compr_put_byte(lc);
    }
  }

  compr_put_byte(0x00);
  compr_put_byte(0xED);
  compr_put_byte(0xED);
  compr_put_byte(0x00);
}

