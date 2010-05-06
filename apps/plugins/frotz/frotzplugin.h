/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Torne Wuff
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
#ifndef _FROTZPLUGIN_H_
#define _FROTZPLUGIN_H_

#include "plugin.h"

/*
 * pretend stdio.h is implemented. references to FILE * still have to be
 * changed to int, and references to NULL into -1, but there are less of those
 */
#define fread(ptr, size, nmemb, stream) rb->read(stream, ptr, size*nmemb)
#define fwrite(ptr, size, nmemb, stream) rb->write(stream, ptr, size*nmemb)
#define fclose(stream) rb->close(stream)
#define fseek(stream, offset, whence) rb->lseek(stream, offset, whence)
#define ftell(stream) rb->lseek(stream, 0, SEEK_CUR)
#define ferror(stream) 0

/*
 * we need functions for character io
 */
extern int frotz_ungetc(int c, int f);
#define ungetc frotz_ungetc
extern int frotz_fgetc(int f);
#define fgetc frotz_fgetc
extern int frotz_fputc(int c, int f);
#define fputc frotz_fputc

/*
 * this is used instead of os_read_key for more prompts and the like
 * since the menu can't be used there.
 */
extern void wait_for_key(void);

/*
 * wrappers
 */
#define strchr(s, c) rb->strchr(s, c)
#define strcpy(dest, src) rb->strcpy(dest, src)

#endif /* _FROTZPLUGIN_H_ */
