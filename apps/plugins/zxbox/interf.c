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

#include "zxconfig.h"
#include "interf.h"

#include <stdio.h>
#include <ctype.h>

#define MAXMSGLEN   2048

char filenamebuf[MAXFILENAME];
char msgbuf[MAXMSGLEN];

char *spif_get_filename(void)
{
    char *name=NULL;
#if 0
/* should be implemented when adding ability */
/* to open snapshots from within zxbox */
  char *name, *s;

  s = get_filename_line();
  for(; *s && isspace((int) *s); s++);
  name = s;
  for(; *s && isgraph((int) *s); s++);
  *s = '\0';

  if(name == s) {
    printf("Canceled!\n");
    return NULL;
  }
#endif
  return name;
}

char *spif_get_tape_fileinfo(int *startp, int *nump)
{
    *startp=*nump=0;
     char *name=NULL;
#if 0
/* should be implemented when adding ability */
/* to tapes snapshots from within zxbox */

  char *name, *s;
  int res;
  
  s = get_filename_line();
  for(; *s && isspace((int) *s); s++);
  name = s;
  for(; *s && isgraph((int) *s); s++);
  
  if(name != s) res = 1;
  else res = 0;

  if(*s) {
    *s = '\0';
    s++;
    if(*s) {
      int r1;

      r1 = sscanf(s, "%d %d", startp, nump);
      if(r1 > 0) res += r1;
    }
  }

  if(res < 1) {
    printf("Canceled!\n");
    return NULL;
  }

  if(res < 2) *startp = -1;
  if(res < 3) *nump = -1;
#endif
  return name;
}

void put_msg(const char *msg)
{
#ifndef USE_GREY
    rb->splash (HZ/2, msg );
#else
	LOGF(msg);
    (void)msg;
#endif
}


void put_tmp_msg(const char *msg)
{
#ifndef USE_GREY
    rb->splash (HZ/10, msg );
#else
    LOGF(msg);
	(void)msg;
#endif
}

