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

#ifndef SPCONF_H
#define SPCONF_H

#define FT_SNAPSHOT 0
#define FT_TAPEFILE 1

extern char *spcf_init_snapshot;
extern int   spcf_init_snapshot_type;
extern char *spcf_init_tapefile;
extern int   spcf_init_tapefile_type;


/*extern int  spcf_read_conf_file(const char *filename);*/
extern void spcf_read_command_line(const void* parameter);
extern void spcf_read_xresources(void);
extern int  spcf_find_file_type(char *filename, int *ftp, int *ftsubp);


#endif /* SPCONF_H */
