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
#include <stdbool.h>
#include "system.h"
#include "ata.h"
#include "ata_idle_notify.h"
#include "kernel.h"
#include "string.h"

void register_ata_idle_func(ata_idle_notify function)
{
#if USING_ATA_CALLBACK
    add_event(DISK_EVENT_SPINUP, true, function);
#else
    function(); /* just call the function now */
/* this _may_ cause problems later if the calling function
   sets a variable expecting the callback to unset it, because
   the callback will be run before this function exits, so before the var is set */
#endif
}

#if USING_ATA_CALLBACK
void unregister_ata_idle_func(ata_idle_notify func, bool run)
{
    remove_event(DISK_EVENT_SPINUP, func);
    
    if (run)
        func();
}

bool call_ata_idle_notifys(bool force)
{
    static int lock_until = 0;

    if (!force)
    {
        if (TIME_BEFORE(current_tick,lock_until) )
            return false;
    }
    lock_until = current_tick + 30*HZ;

    send_event(DISK_EVENT_SPINUP, NULL);
    
    return true;
}
#endif
