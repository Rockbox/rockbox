/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#ifndef __LOOPBACK_H__
#define __LOOPBACK_H__

#include <stdbool.h>
#include "mv.h" /* for HAVE_MULTIDRIVE or not */

struct storage_info;

void loopback_enable(bool on);
void loopback_spindown(int seconds);
void loopback_sleep(void);
bool loopback_disk_is_active(void);
int loopback_soft_reset(void);
int loopback_init(void);
void loopback_close(void);
int loopback_read_sectors(IF_MD2(int drive,) unsigned long start, int count, void* buf);
int loopback_write_sectors(IF_MD2(int drive,) unsigned long start, int count, const void* buf);
void loopback_spin(void);
void loopback_sleepnow(void);
int loopback_spinup_time(void);

#ifdef STORAGE_GET_INFO
void loopback_get_info(IF_MD2(int drive,) struct storage_info *info);
#endif

long loopback_last_disk_activity(void);

#ifdef CONFIG_STORAGE_MULTI
int loopback_num_drives(int first_drive);
#endif

int loopback_set_file(const char *newfile);
void loopback_clear_file(void);
#endif

#ifdef HAVE_HOTSWAP
bool loopback_removable(IF_MD_NONVOID(int drive));
bool loopback_present(IF_MD_NONVOID(int drive));
#endif

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: loopback.h 21933 2009-07-17 22:28:49Z gevaerts $
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
#ifndef __LOOPBACK_H__
#define __LOOPBACK_H__

#include <stdbool.h>
#include "mv.h" /* for HAVE_MULTIDRIVE or not */

struct storage_info;

void loopback_enable(bool on);
void loopback_spindown(int seconds);
void loopback_sleep(void);
bool loopback_disk_is_active(void);
int loopback_soft_reset(void);
int loopback_init(void);
void loopback_close(void);
int loopback_read_sectors(IF_MD2(int drive,) unsigned long start, int count, void* buf);
int loopback_write_sectors(IF_MD2(int drive,) unsigned long start, int count, const void* buf);
void loopback_spin(void);
void loopback_sleepnow(void);
int loopback_spinup_time(void);

#ifdef STORAGE_GET_INFO
void loopback_get_info(IF_MD2(int drive,) struct storage_info *info);
#endif

long loopback_last_disk_activity(void);

#ifdef CONFIG_STORAGE_MULTI
int loopback_num_drives(int first_drive);
#endif

int loopback_set_file(char *newfile);
void loopback_clear_file(void);
#endif

#ifdef HAVE_HOTSWAP
bool loopback_removable(IF_MD_NONVOID(int drive));
bool loopback_present(IF_MD_NONVOID(int drive));
#endif

