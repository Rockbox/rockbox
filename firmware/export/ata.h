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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __ATA_H__
#define __ATA_H__

#include <stdbool.h>

/*
  ata_spindown() time values:
   -1     Immediate spindown
   0      Timeout disabled
   1-240  (time * 5) seconds
   241-251((time - 240) * 30) minutes
   252    21 minutes
   253    Period between 8 and 12 hrs
   254    Reserved
   255    21 min 15 s
*/
extern void ata_enable(bool on);
extern void ata_spindown(int seconds);
extern void ata_poweroff(bool enable);
extern int ata_sleep(void);
extern int ata_standby(int time);
extern bool ata_disk_is_active(void);
extern int ata_hard_reset(void);
extern int ata_soft_reset(void);
extern int ata_init(void);
extern int ata_read_sectors(unsigned long start, int count, void* buf);
extern int ata_write_sectors(unsigned long start, int count, const void* buf);
extern void ata_delayed_write(unsigned long sector, const void* buf);
extern void ata_flush(void);
extern void ata_spin(void);
extern unsigned short* ata_get_identify(void);

extern long last_disk_activity;
extern int ata_spinup_time; /* ticks */

#endif
