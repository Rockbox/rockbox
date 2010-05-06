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

#ifndef ZXMISC_H
#define ZXMISC_H

#include <string.h> /* size_t */

extern char *get_base_name(char *fname);
extern int   check_ext(const char *filename, const char *ext);
extern void  add_extension(char *filename, const char *ext);
extern int   file_exist(const char *filename);
extern int   try_extension(char *filename, const char *ext);
extern char *spif_get_tape_fileinfo(int *startp, int *nump);
extern void *malloc_err(size_t size);
extern char *make_string(char *ostr, const char *nstr);
extern void  free_string(char *ostr);

extern int mis_strcasecmp(const char *s1, const char *s2);

#endif /* ZXMISC_H */
