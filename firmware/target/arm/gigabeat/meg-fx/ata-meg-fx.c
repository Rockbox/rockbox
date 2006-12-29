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
#include "cpu.h"
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "pcf50606.h"

void ata_reset(void)
{
    GPGDAT &= ~(1 << 10);
    sleep(1); /* > 25us */
    GPGDAT |= (1 << 10);
    sleep(1); /* > 2ms */
}

void ata_enable(bool on)
{
    if(on)
        GPGDAT &= ~(1 << 12);
    else
        GPGDAT |= (1 << 12);
}

bool ata_is_coldstart(void)
{
    return true; /* TODO */
}

void ata_device_init(void)
{
}
