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
    if (on) {
        /* until we have native mass-storage mode, we want to reboot on
           usb host connect */
#if defined(IRIVER_H10) || defined (IRIVER_H10_5GB)
        if (button_status()==BUTTON_RIGHT)
#endif /* defined(IRIVER_H10) || defined (IRIVER_H10_5GB) */
        {
#ifndef HAVE_FLASH_STORAGE
            ata_sleepnow(); /* Immediately spindown the disk. */
            sleep(HZ*2);
#endif

#ifdef IPOD_ARCH  /* The following code is based on ipodlinux */
#if CONFIG_CPU == PP5020
            memcpy((void *)0x40017f00, "diskmode\0\0hotstuff\0\0\1", 21);
#elif CONFIG_CPU == PP5022
            memcpy((void *)0x4001ff00, "diskmode\0\0hotstuff\0\0\1", 21);
#endif /* CONFIG_CPU */
#endif /* IPOD_ARCH */

            system_reboot(); /* Reboot */
        }
    }
    else
        usb_core_exit();
}

bool usb_pin_detect(void)
{
#if defined(IPOD_4G) || defined(IPOD_COLOR) \
 || defined(IPOD_MINI) || defined(IPOD_MINI2G)
    /* GPIO D bit 3 is usb detect */
    if (GPIOD_INPUT_VAL & 0x08)
        return true;

#elif defined(IPOD_NANO) || defined(IPOD_VIDEO)
    /* GPIO L bit 4 is usb detect */
    if (GPIOL_INPUT_VAL & 0x10)
        return true;

#elif defined(SANSA_C200)
    /* GPIO H bit 1 is usb detect */
    if (GPIOH_INPUT_VAL & 0x02)
        return true;

#elif defined(SANSA_E200)
    /* GPIO B bit 4 is usb detect */
    if (GPIOB_INPUT_VAL & 0x10)
        return true;

#elif defined(IRIVER_H10) || defined(IRIVER_H10_5GB)
    /* GPIO L bit 2 is usb detect */
    if (GPIOL_INPUT_VAL & 0x4)
        return true;
#endif
    return false;
}

/* detect host or charger (INSERTED or POWERED) */
int usb_detect(void)
{
    static int countdown = 0;
    static int status = USB_EXTRACTED;
    static bool prev_usbstatus1 = false;
    bool usbstatus1, usbstatus2;

#if defined(IPOD_COLOR) || defined(IPOD_4G) \
 || defined(IPOD_MINI)  || defined(IPOD_MINI2G)
    /* GPIO C bit 1 is firewire detect */
    if (!(GPIOC_INPUT_VAL & 0x02))
        /* no charger detection needed for firewire */
        return USB_INSERTED;
#endif

    if (countdown > 0)
    {
        countdown--;

        usbstatus2 = usb_core_data_connection();
        if ((countdown == 0) || usbstatus2)
        {
            /* We now know that we have been connected to either a charger
               or a computer */
            countdown = 0;
            status = usbstatus2 ? USB_INSERTED : USB_POWERED;
        }
        return status;
    }

    usbstatus1 = usb_pin_detect();

    if (usbstatus1 == prev_usbstatus1)
    {
        /* Nothing has changed, so just return previous status */
        return status;
    }
    prev_usbstatus1 = usbstatus1;

    if (!usbstatus1)
    {   /* We have just been disconnected */
        status = USB_EXTRACTED;
        return status;
    }

    /* Run the USB stack to request full bus power */
    usb_core_init();

    if((button_status() & ~USBPOWER_BTN_IGNORE) == USBPOWER_BUTTON)
    {   
        /* The user wants to charge, so it doesn't matter what we are
           connected to. */
        status = USB_POWERED;
        return status;
    }

    /* Wait up to 50 ticks (500ms) before deciding there is no computer
       attached. */
    countdown = 50;

    return status;
}
