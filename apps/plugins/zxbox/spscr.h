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

#ifndef SPSCR_H
#define SPSCR_H

#include "z80_type.h"

extern volatile int screen_visible;

extern void init_spect_scr(void);
extern void destroy_spect_scr(void);
extern void update_screen(void);
extern void flash_change(void);

extern byte *update_screen_line(byte *scrp, int coli, int scri, int border,
                qbyte *cmarkp);

extern void spscr_refresh_colors(void);

#endif /* SPSCR_H */
