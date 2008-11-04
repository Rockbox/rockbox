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
#ifndef __ATA_H__
#define __ATA_H__

#include <stdbool.h>
#include "config.h" /* for HAVE_MULTIVOLUME or not */
#include "mv.h" /* for IF_MV() and friends */

struct storage_info;

void ata_enable(bool on);
void ata_spindown(int seconds);
void ata_sleep(void);
void ata_sleepnow(void);
/* NOTE: DO NOT use this to poll for disk activity.
         If you are waiting for the disk to become active before
         doing something use ata_idle_notify.h
 */
bool ata_disk_is_active(void);
int ata_soft_reset(void);
int ata_init(void);
void ata_close(void);
int ata_read_sectors(IF_MV2(int drive,) unsigned long start, int count, void* buf);
int ata_write_sectors(IF_MV2(int drive,) unsigned long start, int count, const void* buf);
void ata_spin(void);
#if (CONFIG_LED == LED_REAL)
void ata_set_led_enabled(bool enabled);
#endif
unsigned short* ata_get_identify(void);

#ifdef STORAGE_GET_INFO
void ata_get_info(IF_MV2(int drive,) struct storage_info *info);
#endif
#ifdef HAVE_HOTSWAP
bool ata_removable(IF_MV_NONVOID(int drive));
bool ata_present(IF_MV_NONVOID(int drive));
#endif



long ata_last_disk_activity(void);
int ata_spinup_time(void); /* ticks */


#endif
