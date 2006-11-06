/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __ATACALLBACK_H__
#define __ATACALLBACK_H__

#include <stdbool.h>
#define USING_ATA_CALLBACK  !defined(SIMULATOR)             \
                            && !defined(HAVE_FLASH_DISK)    \
                            && !defined(HAVE_MMC)
                            
#define MAX_ATA_CALLBACKS 5
typedef bool (*ata_idle_notify)(void);

extern bool register_ata_idle_func(ata_idle_notify function);
#if USING_ATA_CALLBACK
extern void ata_idle_notify_init(void);
extern void unregister_ata_idle_func(ata_idle_notify function);
extern bool call_ata_idle_notifys(void);
#else
#define unregister_ata_idle_func(f)
#define call_ata_idle_notifys()
#define ata_idle_notify_init()
#endif

#endif /* __ATACALLBACK_H__ */
