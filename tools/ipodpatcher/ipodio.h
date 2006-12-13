/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __IPODIO_H
#define __IPODIO_H

#ifdef __WIN32__
#include <windows.h>
#else
#define HANDLE int
#define O_BINARY 0
#endif

void print_error(char* msg);
int ipod_open(HANDLE* dh, char* diskname, int* sector_size);
int ipod_reopen_rw(HANDLE* dh, char* diskname);
int ipod_close(HANDLE dh);
int ipod_seek(HANDLE dh, unsigned long pos);
int ipod_read(HANDLE dh, unsigned char* buf, int nbytes);
int ipod_write(HANDLE dh, unsigned char* buf, int nbytes);
int ipod_alloc_buffer(unsigned char** sectorbuf, int bufsize);

#endif
