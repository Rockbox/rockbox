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
#include "kernel.h"

#define USB_RST_ASSERT   GPBDAT &= ~(1 << 4)
#define USB_RST_DEASSERT GPBDAT |=  (1 << 4)

#define USB_ATA_ENABLE   GPBDAT |=  (1 << 5)
#define USB_ATA_DISABLE  GPBDAT &= ~(1 << 5)

#define USB_VPLUS_PWR_ASSERT    GPBDAT |=  (1 << 6)
#define USB_VPLUS_PWR_DEASSERT  GPBDAT &= ~(1 << 6)

#define USB_UNIT_IS_PRESENT  !(GPFDAT & 0x01)
#define USB_CRADLE_IS_PRESENT  ((GPFDAT &0x02)&&!(GPGDAT&0x00004000))

#define USB_CRADLE_BUS_ENABLE GPHDAT |=  (1 << 8)
#define USB_CRADLE_BUS_DISABLE GPHDAT &= ~(1 << 8)

/* The usb detect is one pin to the cpu active low */
inline bool usb_detect(void)
{
    return USB_UNIT_IS_PRESENT | USB_CRADLE_IS_PRESENT;
}

void usb_init_device(void)
{
    USB_VPLUS_PWR_ASSERT;
    sleep(HZ/20);
    
    /* Reset the usb port */
    /* Make sure the cpu pin for reset line is set to output */
    GPBCON = (GPBCON & ~0x300) | 0x100;
    USB_RST_ASSERT;
    sleep(HZ/25);
    USB_RST_DEASSERT;
    
    /* needed to complete the reset */
    USB_ATA_ENABLE;
    
    sleep(HZ/15);       /* 66ms */
    
    USB_ATA_DISABLE;
    
    sleep(HZ/25);
    
    /* leave chip in low power mode */
    USB_VPLUS_PWR_DEASSERT;
    
    sleep(HZ/25);
}

void usb_enable(bool on)
{
    if (on)
    {
        /* make sure ata_en is high */
        USB_VPLUS_PWR_ASSERT;
        USB_ATA_ENABLE;
        if(USB_CRADLE_IS_PRESENT) USB_CRADLE_BUS_ENABLE;
    }
    else
    {
        /* make sure ata_en is low */
        if(USB_CRADLE_IS_PRESENT) USB_CRADLE_BUS_DISABLE;
        USB_ATA_DISABLE;
        USB_VPLUS_PWR_DEASSERT;
    }

    sleep(HZ/20); // > 50ms for detecting the enable state change
}
