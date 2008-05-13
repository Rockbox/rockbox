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
#include "zxmisc.h"
#include "spconf_p.h"
#include "interf.h"
#include "spscr_p.h"
#include "spkey.h"

#include "snapshot.h"   /* for SN_Z80  and SN_SNA  */
#include "tapefile.h"   /* for TAP_TAP and TAP_TZX */
#include "zxconfig.h"
#include "stdio.h"
#include "string.h"
#include "sys/types.h"
#include "helpers.h"
#include "ctype.h"


extern const char *spcf_keynames_ascii[];
extern const char *spcf_keynames_misc[];

char *spcf_init_snapshot = NULL;
int   spcf_init_snapshot_type;
char *spcf_init_tapefile = NULL;
int   spcf_init_tapefile_type;


static int file_type = -1;
static int file_subtype;

struct ext_type {
  const char *ext;
  int type;
  int subtype;
};

static struct ext_type extensions[] = {
  {"z80", FT_SNAPSHOT, SN_Z80},
  {"sna", FT_SNAPSHOT, SN_SNA},
  {"tzx", FT_TAPEFILE, TAP_TZX},
  {"tap", FT_TAPEFILE, TAP_TAP},

  {NULL, 0, 0}
};

int spcf_find_file_type(char *filename, int *ftp, int *ftsubp)
{
  int i;
  int found;

  if(*ftp >= 0 && *ftsubp >= 0) return 1;

  found = 0;
  
  for(i = 0; extensions[i].ext != NULL; i++) 
    if((*ftp < 0 || *ftp == extensions[i].type) &&
       (*ftsubp < 0 || *ftsubp == extensions[i].subtype) &&
       check_ext(filename, extensions[i].ext)) {
      found = 1;
      *ftp = extensions[i].type;
      *ftsubp = extensions[i].subtype;
      break;
    }

  if(!found) for(i = 0; extensions[i].ext != NULL; i++) 
    if((*ftp < 0 || *ftp == extensions[i].type) &&
       (*ftsubp < 0 || *ftsubp == extensions[i].subtype) &&
       try_extension(filename, extensions[i].ext)) {
      found = 1;
      *ftp = extensions[i].type;
      *ftsubp = extensions[i].subtype;
      break;
    }
  
  return found;
}

static int find_extension(const char *ext)
{
  int i;
  for(i = 0; extensions[i].ext != NULL; i++)
    if(rb->strcasecmp(extensions[i].ext, ext) == 0) return i;

  return -1;
}


/* now actually a snapshot/tape loader*/
void spcf_read_command_line(const void* parameter)
{
  int ix;

  ix = find_extension( parameter - 3 + rb->strlen (parameter) );
      
  file_type = extensions[ix].type;
  file_subtype = extensions[ix].subtype;
  rb->strncpy(filenamebuf, parameter, MAXFILENAME - 10);
  filenamebuf[MAXFILENAME-10] = '\0';
  if(file_type < 0) file_subtype = -1;
  if(!spcf_find_file_type(filenamebuf, &file_type, &file_subtype))
    return;

  if(file_type == FT_SNAPSHOT) {
    spcf_init_snapshot = make_string(spcf_init_snapshot, filenamebuf);
    spcf_init_snapshot_type = file_subtype;
  }
  else if(file_type == FT_TAPEFILE) {
    spcf_init_tapefile = make_string(spcf_init_tapefile, filenamebuf);
    spcf_init_tapefile_type = file_subtype;
  }
}

