/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "ata.h"
#include "disk.h"
#include "fat.h"
#include "lcd.h"
#include "debug.h"
#include "led.h"
#include "kernel.h"
#include "button.h"
#include "tree.h"

int init(void)
{
    debug_init();
    kernel_init();
    set_irq_level(0);

    if(ata_init()) {
        DEBUGF("*** Warning! The disk is uninitialized\n");
    }
    DEBUGF("ATA initialized\n");

    if (disk_init()) {
        DEBUGF("*** Failed reading partitions\n");
        return -1;
    }

    if(fat_mount(part[0].start)) {
        DEBUGF("*** Failed mounting fat\n");
    }

    button_init();

    return 0;
}

int main(void)
{
    init();

    browse_root();

    while(1) {
        led(true); sleep(HZ/10);
        led(false); sleep(HZ/10);
    }
    return 0;
}
