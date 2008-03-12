/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "cpu.h"
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "power.h"

void ata_reset(void)
{
    and_l(~0x00000040, &GPIO1_OUT);
    sleep(1); /* > 25us */
    or_l(0x00000040, &GPIO1_OUT);
    sleep(1); /* > 2ms */
}

void ata_enable(bool on)
{
    if(on)
        and_l(~0x01000000, &GPIO1_OUT);
    else
        or_l(0x01000000, &GPIO1_OUT);
}

bool ata_is_coldstart(void)
{
    return true; /* TODO */
}

void ata_device_init(void)
{
    /* ATA enable & reset */
    or_l(0x01000040, &GPIO1_OUT);
    or_l(0x01000040, &GPIO1_ENABLE);
    or_l(0x01000040, &GPIO1_FUNCTION);

    /* USB enable */
    and_l(~0x10000000, &GPIO_OUT);
    or_l(0x10000000, &GPIO_ENABLE);
    or_l(0x10000000, &GPIO_FUNCTION);
}
