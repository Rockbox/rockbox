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

#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#define SN_SNA 0
#define SN_Z80 1

extern void save_snapshot_file(char *snsh_name);
extern void load_snapshot_file_type(char *snsh_name, int type);
extern void snsh_z80_load_intern(unsigned char *p, unsigned len);

extern void save_snapshot(void);
extern void load_snapshot(void);

extern void save_quick_snapshot(void);
extern void load_quick_snapshot(void);

#endif /* SNAPSHOT_H */
