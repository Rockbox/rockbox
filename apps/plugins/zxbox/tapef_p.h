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

#ifndef TAPEF_P_H
#define TAPEF_P_H

typedef unsigned char  byte;
typedef unsigned short dbyte;
typedef unsigned long  qbyte;

#define BYTE(arr, i)  (arr[i])
#define DBYTE(arr, i) (arr[i] + (arr[(i)+1] << 8))

struct tapeinfo {
  int type;
  int tzxminver, tzxmajver;
};

struct seginfo {
  int type;
  int segtype;

  dbyte pulse;
  dbyte num;  
  dbyte sync1p;
  dbyte sync2p;
  dbyte zerop;
  dbyte onep;

  dbyte pause;

  byte  bused;

  long len;
  long ptr;
};

extern struct tapeinfo tf_tpi;
extern struct seginfo  tf_cseg;
extern long tf_segoffs;

extern byte *tf_get_block(int i);

#endif /* TAPEF_P_H */
