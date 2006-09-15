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

#define ADDSUB(r, op, tbl) \
{                                  \
  register byte res;               \
  register int idx;                \
  register int flag;               \
  res = RA op;                     \
  idx = IDXCALC(RA, r, res);       \
  RA = res;                        \
  flag = (RF & ~(AALLF)) |         \
         TAB(tbl)[idx];            \
  if(!res) flag |= ZF;             \
  RF = flag;               \
}  


#define ADD(r) ADDSUB(r, + r, addf_tbl)
#define ADC(r) ADDSUB(r, + r + (RF & CF), addf_tbl) 
#define SUB(r) ADDSUB(r, - r, subf_tbl)
#define SBC(r) ADDSUB(r, - r - (RF & CF), subf_tbl)

#define AND(r) \
{                                  \
  register byte res;               \
  register byte flag;              \
  res = RA & r;                    \
  RA = res;                        \
  flag = (RF & ~(AALLF)) |          \
         TAB(orf_tbl)[res] | HF;   \
  RF = flag;                       \
}


#define XOR(r) \
{                                  \
  register byte res;               \
  register byte flag;              \
  res = RA ^ r;                    \
  RA = res;                        \
  flag = (RF & ~(AALLF)) |          \
         TAB(orf_tbl)[res];        \
  RF = flag;                       \
}


#define OR(r) \
{                                  \
  register byte res;               \
  register byte flag;              \
  res = RA | r;                    \
  RA = res;                        \
  flag = (RF & ~(AALLF)) |          \
         TAB(orf_tbl)[res];        \
  RF = flag;                       \
}


#define CP(r) \
{                                  \
  register byte res;               \
  register int idx;                \
  register int flag;               \
  res = RA - r;                    \
  idx = IDXCALC(RA, r, res);       \
  flag = (RF & ~(AALLF)) |          \
         TAB(subf_tbl)[idx];       \
  if(!res) flag |= ZF;             \
  RF = flag;               \
}  
