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

#if 0
                NOTE: ata_idle_nofity usage notes..
                
        1) the callbacks are called in the ata thread, not main/your thread.
        2) Asyncronous callbacks (like the buffer refill) should be avoided.
            If you must use an async callback, remember to check ata_is_active() before
            accessing the disk, and nonot call any functions between that check and the
            disk access which may cause a yield (lcd_update() does this!)
        3) Do not call cany yielding functions in the callback
        4) Do not call ata_sleep in the callbacks
        5) Dont Panic!
#endif

#define USING_ATA_CALLBACK  !defined(SIMULATOR)             \
                            && !defined(HAVE_FLASH_DISK)    \
                            && !defined(HAVE_MMC)

#define MAX_ATA_CALLBACKS 5
typedef bool (*ata_idle_notify)(void);

extern bool register_ata_idle_func(ata_idle_notify function);
#if USING_ATA_CALLBACK
extern void ata_idle_notify_init(void);
extern void unregister_ata_idle_func(ata_idle_notify function, bool run);
extern bool call_ata_idle_notifys(bool sleep_after);
#else
#define unregister_ata_idle_func(f,r)
#define call_ata_idle_notifys()
#define ata_idle_notify_init(s)
#endif

#endif /* __ATACALLBACK_H__ */
