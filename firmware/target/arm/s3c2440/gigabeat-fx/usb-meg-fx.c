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
#include "ata.h"

#define USB_RST_ASSERT   GPBDAT &= ~(1 << 4)
#define USB_RST_DEASSERT GPBDAT |=  (1 << 4)

#define USB_VPLUS_PWR_ASSERT    GPBDAT |=  (1 << 6)
#define USB_VPLUS_PWR_DEASSERT  GPBDAT &= ~(1 << 6)

#define USB_UNIT_IS_PRESENT  !(GPFDAT & 0x01)
#define USB_CRADLE_IS_PRESENT  ((GPFDAT &0x02)&&!(GPGDAT&1<<14))

#define USB_CRADLE_BUS_ENABLE GPHDAT |=  (1 << 8)
#define USB_CRADLE_BUS_DISABLE GPHDAT &= ~(1 << 8)

/* The usb detect is one pin to the cpu active low */
inline bool usb_detect(void)
{
    return USB_UNIT_IS_PRESENT | USB_CRADLE_IS_PRESENT;
}

void usb_init_device(void)
{
    /* Input is the default configuration, only pullups need to be disabled */
/*    GPFUP|=0x02;  */

    USB_VPLUS_PWR_ASSERT;
    GPBCON=( GPBCON&~(1<<13) ) | (1 << 12);

    sleep(HZ/20);

    /* Reset the usb port */
    USB_RST_ASSERT;
    GPBCON = (GPBCON & ~0x200) | 0x100; /* Make sure reset line is an output */

    sleep(HZ/25);
    USB_RST_DEASSERT;

    /* needed to complete the reset */
    ata_enable(false);

    sleep(HZ/15);       /* 66ms */

    ata_enable(true);

    sleep(HZ/25);

    /* leave chip in low power mode */
    USB_VPLUS_PWR_DEASSERT;

    sleep(HZ/25);
}

void usb_enable(bool on)
{
    if (on)
    {
        USB_VPLUS_PWR_ASSERT;
        if(USB_CRADLE_IS_PRESENT) USB_CRADLE_BUS_ENABLE;
    }
    else
    {
        if(USB_CRADLE_IS_PRESENT) USB_CRADLE_BUS_DISABLE;
        USB_VPLUS_PWR_DEASSERT;
    }

    /* Make sure USB_CRADLE_BUS pin is an output */
    GPHCON=( GPHCON&~(1<<17) ) | (1<<16); /* Make the pin an output */
    GPHUP|=1<<8;  /* Disable pullup in SOC as we are now driving */

    sleep(HZ/20); // > 50ms for detecting the enable state change
}
