/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================

   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.

   See the file COPYING for details.

   $Id: files.c,v 1.18 1997/02/23 20:39:29 ahornby Exp ahornby $
******************************************************************************/

/*
   Used to load cartridge images into memory.
 */

#include "rbconfig.h"
#include <stdio.h>
/*#include <string.h>*/
#include "options.h"
#include "vmachine.h"

#include "memory.h"

/* Return the size of the file pointed to by fp */
/* Avoids need for UNIX type stat, leaves file */
/* pointer untouched. */
long
filesize (int fd)
{
  long curpos, length;

  curpos = rb->lseek(fd, 0, SEEK_CUR);
  rb -> lseek (fd, 0L, SEEK_END);
  length = rb->lseek(fd, 0, SEEK_CUR);
  rb -> lseek (fd, curpos, SEEK_SET);
  return length;
}


/* Load a raw binary image  */
/* fp: file stream to load */
/* stat_data: unix stat structure for file pointed to by fp */
/* returns: size of file loaded */
int
loadRaw (int fd, long flen)
{

  int size = flen;
#if 0
  if (Verbose)
    rb -> splash (HZ, "Raw loading %d bytes\n");
#endif

  // TODO: use my newly defined max size constants in memory.h
  if (size > 16384)
    size = 16384; // TODO: warn!
  rb -> read ( fd , theCart , size );
  rom_size = size;
  if (size == 2048)
    {
      rb->memcpy (&theCart[2048], &theCart[0], 2048);
      rom_size = 4096;
    }
  else if (size < 2048)
    {
      theCart[0x0ffc] = 0x00;
      theCart[0x0ffd] = 0xf0;
      rom_size = 4096;
    }

  DEBUGF("Loading ROM. Size = %d Bytes\n", size);
  if (size < 2048 || size > 16384)
      DEBUGF("**WARNING** incompatible size!\n");
  if (size != 2048 && size != 4096 && size != 8192 && size != 16384)
      DEBUGF("**WARNING** odd rom size\n");
  return size;
}


// --------------------------------------------------------------

const char *bsw_fileextension[] = { BSW_FILE_EXTENSIONS };

/*
 * Helper function:
 * look for file extension
 * fname: filename
 * ext: the extension to look for
 * right: index of right end of string; strlen for first extension, the index of '.'
 * if ''right' is 0, strlen is calculated.
 * for secondary extension
 * returns index of matching char if file extension found, 0 if not equal
 */
static size_t is_file_extension(char *fname, const char *ext, size_t right)
{
    if (!right)
        right = rb->strlen(fname);
    size_t l = rb->strlen(ext);
    if (right < l)
        return 0;

    right -= l;
    if (!rb->strncasecmp(&fname[right], ext, l))
        return right;

    return 0;
}

int check_file_extension(char *fname)
{
    size_t index;

    // first ignore "a26" if present
    // TODO: define macro for standard extensions. Should also ignore "bin"
    // extension (if used with "open with...")
    index = is_file_extension(fname, ".a26", 0);
    if (index) {
        DEBUGF("   found file extension '.a26'\n");
    }
    else {
        index = rb->strlen(fname);
    }

    for (int i = 1; i < BSW_END_OF_LIST; ++i) {
        if (is_file_extension(fname, bsw_fileextension[i], index)) {
            DEBUGF("   found file extension %s\n", bsw_fileextension[i]);
            base_opts.bank = i;
            return 1;
        }
    }

    DEBUGF("   no extra BSW extension recognized\n");

    return 0;
}


// --------------------------------------------------------------

/* Load a cartridge image */
/* name: filename to load */
/* returns: -1 on error 0 otherwise  */
int
loadCart (char *name)
{
  int fd;
  int flen;

  fd = rb -> open (name, O_RDONLY);
  if(fd < 0) {
      return -1;
  }

  DEBUGF("Loading cart: %s\n", name);
#if 0
  if (Verbose)
    rb -> splash  (HZ , "Loading cart: %s\n");
#endif
  flen = filesize (fd);

  DEBUGF("Cart Size: %d Bytes\n", flen);

  check_file_extension(name);

      int size=loadRaw (fd, flen);
      /* try and correct for idiots */
      // TODO: move to memory.c and maybe improve auto detection
      if(base_opts.bank==BSW_NONE) {
        switch(size) {
        case 8192:
          base_opts.bank=BSW_F8;
          break;
        case 16384:
          base_opts.bank=BSW_F6;
          break;
        default:
          break;
          /* Do nothing */
        }
      }


  rb -> close (fd);

  return 0;
}
