/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include <stdbool.h>
#include "cpu.h"
#include "system.h"

void usb_init_device(void)
{
}

bool usb_detect(void)
{
    return (GPFDAT & 1) ? false : true;
}

void usb_enable(bool on)
{
    if(on) {
        int i;

        GPBDAT &= 0x7EF;
        GPBCON |= 1<<8;

        GPGDAT &= 0xE7FF;
        GPGDAT |= 1<<11;

        for (i = 0; i < 10000000; i++) {continue;}

        GPBCON &= 0x2FFCFF;
        GPBDAT |= 1<<5; 
        GPBDAT |= 1<<6;
    } else {
        /* TODO how turn USB mode back off again? */
    }
}
