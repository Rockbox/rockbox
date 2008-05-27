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

#ifndef SPSCR_P_H
#define SPSCR_P_H

#define COLORNUM 16

struct rgb {
  int r, g, b;
};

#define SCRSIZE 6912
#define COLORBEG 6144

extern void spscr_init_mask_color(void);
extern void spscr_init_line_pointers(int lines);

#endif /* SPSCR_P_H */
