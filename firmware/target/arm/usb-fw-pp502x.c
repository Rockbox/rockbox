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
#include "arcotg_udc.h"

#ifdef HAVE_USBSTACK
#include "usbstack.h"
#endif

void usb_init_device(void)
{

#if defined(IPOD_COLOR) || defined(IPOD_4G) \
 || defined(IPOD_MINI)  || defined(IPOD_MINI2G)
    /* GPIO C bit 1 is firewire detect */
    GPIOC_ENABLE    |=  0x02;
    GPIOC_OUTPUT_EN &= ~0x02;
#endif
}

void usb_enable(bool on)
{
#ifndef HAVE_USBSTACK
    /* This device specific code will eventually give way to proper USB
       handling, which should be the same for all PP502x targets. */
    if (on)
    {
#if defined(IPOD_ARCH) || defined(IRIVER_H10) || defined (IRIVER_H10_5GB)
        /* For the H10 and iPod, we can only do one thing with USB mode - reboot
           into the flash-based disk-mode.  This does not return. */

#if defined(IRIVER_H10) || defined (IRIVER_H10_5GB)
        if(button_status()==BUTTON_RIGHT)
#endif
        {
            ata_sleepnow(); /* Immediately spindown the disk. */
            sleep(HZ*2);

#ifdef IPOD_ARCH  /* The following code is based on ipodlinux */
#if CONFIG_CPU == PP5020
            memcpy((void *)0x40017f00, "diskmode\0\0hotstuff\0\0\1", 21);
#elif CONFIG_CPU == PP5022
            memcpy((void *)0x4001ff00, "diskmode\0\0hotstuff\0\0\1", 21);
#endif
#endif

            system_reboot(); /* Reboot */
        }
#endif
    }
#endif
}

bool usb_detect(void)
{
    static bool prev_usbstatus1 = false;
    bool usbstatus1,usbstatus2;

#if defined(IPOD_COLOR) || defined(IPOD_4G) \
 || defined(IPOD_MINI)  || defined(IPOD_MINI2G)
    /* GPIO C bit 1 is firewire detect */
    if (!(GPIOC_INPUT_VAL & 0x02))
        return true;
#endif

    /* UDC_ID should have the bit format:
        [31:24] = 0x0
        [23:16] = 0x22 (Revision number)
        [15:14] = 0x3 (Reserved)
        [13:8]  = 0x3a (NID - 1's compliment of ID)
        [7:6]   = 0x0 (Reserved)
        [5:0]   = 0x05 (ID) */
    if (UDC_ID != 0x22FA05) {
        return false;
    }

    usbstatus1 = (UDC_OTGSC & 0x800) ? true : false;
#ifdef HAVE_USBSTACK
    if ((usbstatus1 == true) && (prev_usbstatus1 == false)) {
        usb_stack_start();
    } else if ((usbstatus1 == false) && (prev_usbstatus1 == true)) {
    	usb_stack_stop();
    }
#endif
    prev_usbstatus1 = usbstatus1;
    usbstatus2 = (UDC_PORTSC1 & PORTSCX_CURRENT_CONNECT_STATUS) ? true : false;

    if (usbstatus1 && usbstatus2) {
        return true;
    } else {
        return false;
    }
}
