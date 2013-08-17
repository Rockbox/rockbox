/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 * Copyright (C) 2008 by Frank Gevaerts
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
#ifndef __NAND_H__
#define __NAND_H__

#include <stdbool.h>
#include "config.h"
#include "mv.h" /* for HAVE_MULTIDRIVE or not */

struct storage_info;

void nand_enable(bool on);
void nand_spindown(int seconds);
void nand_sleep(void);
void nand_sleepnow(void);
bool nand_disk_is_active(void);
int  nand_soft_reset(void);
int  nand_init(void) STORAGE_INIT_ATTR;
void nand_close(void);
int  nand_read_sectors(IF_MD(int drive,) unsigned long start, int count, void* buf);
int  nand_write_sectors(IF_MD(int drive,) unsigned long start, int count, const void* buf);
#ifdef HAVE_STORAGE_FLUSH
int  nand_flush(void);
#endif
void nand_spin(void);
int  nand_spinup_time(void); /* ticks */

#ifdef STORAGE_GET_INFO
void nand_get_info(IF_MD(int drive,) struct storage_info *info);
#endif

long nand_last_disk_activity(void);

#ifdef CONFIG_STORAGE_MULTI
int nand_num_drives(int first_drive);
#endif

#endif
