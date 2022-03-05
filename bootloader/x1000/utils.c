/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2022 Aidan MacDonald
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

#include "x1000bootloader.h"
#include "storage.h"
#include "button.h"
#include "kernel.h"

/* this is both incorrect and incredibly racy... */
int check_disk(bool wait)
{
    if(storage_present(IF_MD(0)))
        return DISK_PRESENT;
    if(!wait)
        return DISK_ABSENT;

    while(!storage_present(IF_MD(0))) {
        splash2(0, "Insert SD card", "Press " BL_QUIT_NAME " to cancel");
        if(get_button(HZ/4) == BL_QUIT)
            return DISK_CANCELED;
    }

    /* a lie intended to give time for mounting the disk in the background */
    splash(HZ, "Scanning disk");

    return DISK_PRESENT;
}
