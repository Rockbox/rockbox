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
#include "system.h"
#include "kernel.h"
#include "ata.h"
#include "usb.h"

#define USB_RST_ASSERT   GPBDAT &= ~(1 << 4)
#define USB_RST_DEASSERT GPBDAT |=  (1 << 4)

#define USB_VBUS_PWR_ASSERT    GPBDAT |=  (1 << 6)
#define USB_VBUS_PWR_DEASSERT  GPBDAT &= ~(1 << 6)

#define USB_UNIT_IS_PRESENT  !(GPFDAT & 0x01)
#define USB_CRADLE_IS_PRESENT  ((GPFDAT &0x02)&&((GPGDAT&(3<<13))==(1<<13)))

#define USB_CRADLE_BUS_ENABLE GPHDAT |=  (1 << 8)
#define USB_CRADLE_BUS_DISABLE GPHDAT &= ~(1 << 8)

/* The usb detect is one pin to the cpu active low */
int usb_detect(void)
{
   if (USB_UNIT_IS_PRESENT | USB_CRADLE_IS_PRESENT)
       return USB_INSERTED;
   else
       return USB_EXTRACTED;
}

void usb_init_device(void)
{
    /* Setup USB Cradle Power control (output, disabled, no pullup) */
    GPHCON=( GPHCON&~(1<<17) ) | (1<<16);
    GPHUP|=1<<8;
    USB_CRADLE_BUS_DISABLE;

    /* Setup VBUS PWR (output, asserted, no pullup) */
    GPBCON=( GPBCON&~(1<<13) ) | (1 << 12);
    GPBUP|=1<<6;
    USB_VBUS_PWR_ASSERT;

    sleep(HZ/20);

    /* Setup USB reset (output, asserted, no pullup) */
    GPBCON = (GPBCON & ~0x200) | 0x100; /* Make sure reset line is an output */
    GPBUP|=1<<4;
    USB_RST_ASSERT;

    sleep(HZ/25);
    USB_RST_DEASSERT;

    /* needed to complete the reset */
    ata_enable(false);

    sleep(HZ/15);       /* 66ms */

    ata_enable(true);

    sleep(HZ/25);

    /* leave chip in low power mode */
    USB_VBUS_PWR_DEASSERT;

    sleep(HZ/25);
}

void usb_enable(bool on)
{    
    if (on)
    {
        USB_VBUS_PWR_ASSERT;
        if(USB_CRADLE_IS_PRESENT) USB_CRADLE_BUS_ENABLE;
    }
    else
    {
        if(USB_CRADLE_IS_PRESENT) USB_CRADLE_BUS_DISABLE;
        USB_VBUS_PWR_DEASSERT;
    }

    sleep(HZ/20); // > 50ms for detecting the enable state change
}
