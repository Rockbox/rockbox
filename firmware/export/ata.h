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
#include "config.h" /* for HAVE_MULTIVOLUME or not */

/* FixMe: These macros are a bit nasty and perhaps misplaced here.
   We'll get rid of them once decided on how to proceed with multivolume. */
#ifdef HAVE_MULTIVOLUME
#define IF_MV(x) x /* optional volume/drive parameter */
#define IF_MV2(x,y) x,y /* same, for a list of arguments */
#define IF_MV_NONVOID(x) x /* for prototype with sole volume parameter */
#define NUM_VOLUMES 2
#else /* empty definitions if no multi-volume */
#define IF_MV(x)
#define IF_MV2(x,y)
#define IF_MV_NONVOID(x) void
#define NUM_VOLUMES 1
#endif

extern void ata_enable(bool on);
extern void ata_spindown(int seconds);
extern void ata_sleep(void);
extern void ata_sleepnow(void);
/* NOTE: DO NOT use this to poll for disk activity.
         If you are waiting for the disk to become active before
         doing something use ata_idle_notify.h
 */
extern bool ata_disk_is_active(void);
extern int ata_hard_reset(void);
extern int ata_soft_reset(void);
extern int ata_init(void);
extern int ata_read_sectors(IF_MV2(int drive,) unsigned long start, int count, void* buf);
extern int ata_write_sectors(IF_MV2(int drive,) unsigned long start, int count, const void* buf);
extern void ata_spin(void);
#if (CONFIG_LED == LED_REAL)
extern void ata_set_led_enabled(bool enabled);
#endif
extern unsigned short* ata_get_identify(void);

extern long last_disk_activity;
extern int ata_spinup_time; /* ticks */

#endif
