/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * iPod driver based on code from the ipodlinux project - http://ipodlinux.org
 * Adapted for Rockbox in January 2006
 * Original file: podzilla/usb.c
 * Copyright (C) 2005 Adam Johnston
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "system.h"
#include "usb.h"
#include "button.h"
#include "ata.h"
#include "string.h"
#include "usb_core.h"
#include "usb_drv.h"

void usb_init_device(void)
{
    /* enable usb module */
    GPO32_ENABLE |= 0x200;  

    outl(inl(0x7000002C) | 0x3000000, 0x7000002C);  
    DEV_EN |= DEV_USB;

    DEV_RS |= DEV_USB; /* reset usb start */  
    DEV_RS &=~DEV_USB;/* reset usb end */  

    DEV_INIT2 |= INIT_USB;
    while ((inl(0x70000028) & 0x80) == 0);
    outl(inl(0x70000028) | 0x2, 0x70000028);
    udelay(0x186A0);

#if defined(IPOD_COLOR) || defined(IPOD_4G) \
 || defined(IPOD_MINI)  || defined(IPOD_MINI2G)
    /* GPIO C bit 1 is firewire detect */
    GPIOC_ENABLE    |=  0x02;
    GPIOC_OUTPUT_EN &= ~0x02;
#endif
}

void usb_enable(bool on)
{
    if (on)
        usb_core_init();
    else
        usb_core_exit();
}

int usb_detect(void)
{
#if defined(IPOD_COLOR) || defined(IPOD_4G) \
 || defined(IPOD_MINI)  || defined(IPOD_MINI2G)
    /* GPIO C bit 1 is firewire detect */
    if (!(GPIOC_INPUT_VAL & 0x02))
        return USB_INSERTED;
#elif defined(SANSA_C200)
    /* GPIO H bit 1 is usb detect */
    if (GPIOH_INPUT_VAL & 0x02)
        return USB_INSERTED;
#elif defined(SANSA_E200)
    /* GPIO B bit 4 is usb detect */
    if (GPIOB_INPUT_VAL & 0x10)
        return USB_INSERTED;
#elif defined(IRIVER_H10) || defined(IRIVER_H10_5GB)
    /* GPIO L bit 2 is usb detect */
    if (GPIOL_INPUT_VAL & 0x4)
        return USB_INSERTED;
#endif
    if (usb_drv_powered())
        return USB_INSERTED;

    return USB_EXTRACTED;
}
