/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "debug.h"
#include "ata.h"
#include "fat.h"
#include "disk.h"
#include "panic.h"
#include "lcd.h"
#include "adc.h"
#include "usb.h"
#include "button.h"
#include "sprintf.h"
#include "string.h"
#include "hwcompat.h"
#include "pp5020.h"

void usb_init_device(void)
{
    /* USB is initialized by bootloader */
}

bool usb_detect(void)
{
    return (GPIOL_INPUT_VAL & 0x04)?true:false;
}

void usb_enable(bool on)
{
    /* For the H10, we reboot if BUTTON_RIGHT is held so that the iriver
     * bootloader can start up in UMS mode. This does not return. */
    if (on && (button_status()==BUTTON_RIGHT))
    {
        ata_sleepnow(); /* Immediately spindown the disk. */
        sleep(HZ*2);
        system_reboot();
    }
}
