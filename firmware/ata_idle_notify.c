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
#include <stdbool.h>
#include "system.h"
#include "ata_idle_notify.h"
#include "logf.h"

#if USING_ATA_CALLBACK
static ata_idle_notify ata_idle_notify_funcs[MAX_ATA_CALLBACKS];
static int ata_callback_count = 0;
#endif

bool register_ata_idle_func(ata_idle_notify function)
{
#if USING_ATA_CALLBACK
    int i;
    for (i=0; i<MAX_ATA_CALLBACKS; i++)
    {
        if (ata_idle_notify_funcs[i] == NULL)
        {
            ata_idle_notify_funcs[i] = function;
            ata_callback_count++;
            return true;
        }
        else if (ata_idle_notify_funcs[i] == function)
            return true;
    }
    return false;
#else
    function(); /* just call the function now */
/* this _may_ cause problems later if the calling function
   sets a variable expecting the callback to unset it, because
   the callback will be run before this function exits, so before the var is set */
    return true;
#endif
}

#if USING_ATA_CALLBACK
void unregister_ata_idle_func(ata_idle_notify func)
{
    int i;
    for (i=0; i<MAX_ATA_CALLBACKS; i++)
    {
        if (ata_idle_notify_funcs[i] == func)
        {
            ata_idle_notify_funcs[i] = NULL;
            ata_callback_count--;
        }
    }
    return;
}

bool call_ata_idle_notifys(void)
{
    int i;
    ata_idle_notify function;
    if (ata_callback_count == 0)
        return false;
    ata_callback_count = 0; /* so we dont re-enter every time the callbacks read/write */
    for (i = 0; i < MAX_ATA_CALLBACKS; i++)
    {
        if (ata_idle_notify_funcs[i])
        {
            function = ata_idle_notify_funcs[i];
            ata_idle_notify_funcs[i] = NULL;
            function();
        }
    }
    return true;
}

void ata_idle_notify_init(void)
{
    int i;
    for (i=0; i<MAX_ATA_CALLBACKS; i++)
    {
        ata_idle_notify_funcs[i] = NULL;
    }
}
#endif
