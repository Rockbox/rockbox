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
    and_l(~0x00000010, &GPIO_OUT); /* Self powered */
    or_l(0x00800000, &GPIO_OUT);   /* RESET deasserted */
    or_l(0x00800010, &GPIO_FUNCTION);
    or_l(0x00800010, &GPIO_ENABLE);
    
    or_l(0x00800000, &GPIO1_FUNCTION);  /* USB detect */
}

bool usb_detect(void)
{
    return (GPIO1_READ & 0x00800000)?true:false;
}

void usb_enable(bool on)
{
    if(on) {
        or_l(0x00000008, &GPIO1_OUT);
    } else {
        and_l(~0x00000008, &GPIO1_OUT);
    }
}
