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

#ifndef TAPEFILE_H
#define TAPEFILE_H

#define DAT_ERR        -2
#define DAT_END        -1

#define SEG_ERROR      -1
#define SEG_END         0   /* ^--- End of tape */

#define SEG_STOP        1
#define SEG_SKIP        2
#define SEG_GRP_BEG     3
#define SEG_GRP_END     4

#define SEG_DATA       10
#define SEG_DATA_TURBO 11
#define SEG_DATA_PURE  12

#define SEG_PAUSE      20
#define SEG_OTHER      21

#define SEG_VIRTUAL    30

#define TAP_TAP      0
#define TAP_TZX      1

#define MACHINE_48  0
#define MACHINE_128 1

struct tape_options {
  int blanknoise;
  int stoppause;
  int machine;
};

#define INITTAPEOPT(to) \
  (to).blanknoise=0,       \
  (to).stoppause=0,        \
  (to).machine=MACHINE_48

extern char seg_desc[];

#ifdef __cplusplus
extern "C" {
#endif

extern int  open_tapefile(char *name, int type);
extern void close_tapefile(void);
extern int  goto_segment(int at_seg);
extern int  next_segment(void);
extern int  next_byte(void);
extern int  next_imps(unsigned short *impbuf, int buflen, long timelen);

extern void set_tapefile_options(struct tape_options *to);
extern int get_level(void);

extern long get_seglen(void);
extern long get_segpos(void);
extern unsigned segment_pos(void);

#ifdef __cplusplus
}
#endif

#endif /* TAPEFILE_H */
