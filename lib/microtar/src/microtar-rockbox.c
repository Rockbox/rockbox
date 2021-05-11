/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "file.h"
#include "microtar.h"
#include <stdint.h>

static int fd_write(mtar_t *tar, const void *data, unsigned size) {
  intptr_t fd = (intptr_t)tar->stream;
  ssize_t res = write(fd, data, size);
  if(res < 0 || ((unsigned)res != size))
    return MTAR_EWRITEFAIL;
  else
    return MTAR_ESUCCESS;
}

static int fd_read(mtar_t *tar, void *data, unsigned size) {
  intptr_t fd = (intptr_t)tar->stream;
  ssize_t res = read(fd, data, size);
  if(res < 0 || ((unsigned)res != size))
    return MTAR_EREADFAIL;
  else
    return MTAR_ESUCCESS;
}

static int fd_seek(mtar_t *tar, unsigned offset) {
  intptr_t fd = (intptr_t)tar->stream;
  off_t res = lseek(fd, offset, SEEK_SET);
  if(res < 0 || ((unsigned)res != offset))
    return MTAR_ESEEKFAIL;
  else
    return MTAR_ESUCCESS;
}

static int fd_close(mtar_t *tar) {
  intptr_t fd = (intptr_t)tar->stream;
  close(fd);
  return MTAR_ESUCCESS;
}


int mtar_open(mtar_t *tar, const char *filename, const char *mode) {
  int err;
  int openmode;
  int fd;

  /* Init tar struct and functions */
  memset(tar, 0, sizeof(*tar));
  tar->write = fd_write;
  tar->read = fd_read;
  tar->seek = fd_seek;
  tar->close = fd_close;

  /* Get correct mode flags */
  if ( strchr(mode, 'r') )
      openmode = O_RDONLY;
  else if ( strchr(mode, 'w') )
      openmode = O_CREAT|O_TRUNC|O_WRONLY;
  else if ( strchr(mode, 'a') )
      openmode = O_WRONLY|O_APPEND;
  else
      return MTAR_EFAILURE;

  /* Open file */
  fd = open(filename, openmode);
  if(fd < 0)
    return MTAR_EOPENFAIL;

  tar->stream = (void*)(intptr_t)fd;

  /* Read first header to check it is valid if mode is `r` */
  if ( openmode & O_RDONLY ) {
    err = mtar_read_header(tar, &tar->header);
    if (err != MTAR_ESUCCESS) {
      mtar_close(tar);
      return err;
    }
  }

  /* Return ok */
  return MTAR_ESUCCESS;
}
