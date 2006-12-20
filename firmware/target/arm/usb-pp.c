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

#include "usb-target.h"
 
void usb_init_device(void)
{
    int r0;
    outl(inl(0x70000084) | 0x200, 0x70000084);

    outl(inl(0x7000002C) | 0x3000000, 0x7000002C);
    DEV_EN |= DEV_USB;

    DEV_RS |= DEV_USB; /* reset usb start */
    DEV_RS &=~DEV_USB;/* reset usb end */

    DEV_INIT |= INIT_USB;
    while ((inl(0x70000028) & 0x80) == 0);

    UOG_PORTSC1 |= 0x100;
    while ((UOG_PORTSC1 & 0x100) != 0);

    UOG_OTGSC |= 0x5F000000;
    if( (UOG_OTGSC & 0x100) == 0) {
        UOG_USBMODE &=~ 0x3;
        UOG_USBMODE |= 0x2;
        outl(inl(0x70000028) | 0x4000, 0x70000028);
        outl(inl(0x70000028) | 0x2, 0x70000028);
    } else {
        UOG_USBMODE |= 0x2;
        outl(inl(0x70000028) &~0x4000, 0x70000028);
        outl(inl(0x70000028) | 0x2, 0x70000028);
    }
    UOG_USBCMD |= 0x2;
    while((UOG_USBCMD & 0x2) != 0);
    r0 = UOG_PORTSC1;

    /* Note from IPL source (referring to next 5 lines of code: 
       THIS NEEDS TO BE CHANGED ONCE THERE IS KERNEL USB */
    outl(inl(0x70000020) | 0x80000000, 0x70000020);
    DEV_EN |= DEV_USB;
    while ((inl(0x70000028) & 0x80) == 0);
    outl(inl(0x70000028) | 0x2, 0x70000028);

    udelay(0x186A0);
}

void usb_enable(bool on)
{
    /* This device specific code will eventually give way to proper USB
       handling, which should be the same for all PortalPlayer targets. */
    if (on)
    {
#if IPOD_ARCH || defined(IRIVER_H10) || defined (IRIVER_H10_5GB)
        /* For the H10 and iPod, we can only do one thing with USB mode - reboot
           into the flash-based disk-mode.  This does not return. */
       
        /* The following code is copied from ipodlinux */
#if defined(IPOD_COLOR) || defined(IPOD_3G) || \
    defined(IPOD_4G) || defined(IPOD_MINI)
        unsigned char* storage_ptr = (unsigned char *)0x40017F00;
#elif defined(IPOD_NANO) || defined(IPOD_VIDEO) || defined(IPOD_MINI2G)
        unsigned char* storage_ptr = (unsigned char *)0x4001FF00;
#endif

#if defined(IRIVER_H10) || defined (IRIVER_H10_5GB)
        if(button_status()==BUTTON_RIGHT)
#endif
        {
            ata_sleepnow(); /* Immediately spindown the disk. */
            sleep(HZ*2);
#ifdef IPOD_ARCH
            memcpy(storage_ptr, "diskmode\0\0hotstuff\0\0\1", 21);
#endif
            system_reboot(); /* Reboot */
        }
#endif
    }
}

bool usb_detect(void)
{
    bool current_status;

    /* UOG_ID should have the bit format:
        [31:24] = 0x0
        [23:16] = 0x22 (Revision number)
        [15:14] = 0x3 (Reserved)
        [13:8]  = 0x3a (NID - 1's compliment of ID)
        [7:6]   = 0x0 (Reserved)
        [5:0]   = 0x05 (ID) */
    if (UOG_ID != 0x22FA05) {
        return false;
    }
    current_status = (UOG_OTGSC & 0x800)?true:false;
    
    return current_status;
}
