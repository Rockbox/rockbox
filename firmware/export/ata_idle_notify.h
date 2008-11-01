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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __ATACALLBACK_H__
#define __ATACALLBACK_H__


#include <stdbool.h>
#include "events.h"

/*
        NOTE: storage_idle_notify usage notes..
        
1) The callbacks are called in the ata thread, not main/your thread.
2) Asynchronous callbacks (like the buffer refill) should be avoided.
    If you must use an async callback, remember to check storage_is_active() before
    accessing the disk, and do not call any functions between that check and the
    disk access which may cause a yield (lcd_update() does this!).
3) Do not call any yielding functions in the callback.
4) Do not call storage_sleep in the callbacks.
5) Don't Panic!

*/

enum {
    DISK_EVENT_SPINUP = (EVENT_CLASS_DISK|1),
};

#define USING_STORAGE_CALLBACK  !defined(SIMULATOR)             \
                            && ! ((CONFIG_STORAGE & STORAGE_NAND) \
                               && (CONFIG_NAND & NAND_IFP7XX)) \
                            && !defined(BOOTLOADER)

typedef bool (*storage_idle_notify)(void);

extern void register_storage_idle_func(storage_idle_notify function);
#if USING_STORAGE_CALLBACK
extern void unregister_storage_idle_func(storage_idle_notify function, bool run);
extern bool call_storage_idle_notifys(bool force);
#else
#define unregister_storage_idle_func(f,r)
#define call_storage_idle_notifys(f)
#define storage_idle_notify_init(s)
#endif

#endif /* __ATACALLBACK_H__ */
