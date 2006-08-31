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

/* Code from the iPod port but commented out. USB detection custom made based
   on GPIO analysis */

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
#ifdef HAVE_MMC
#include "ata_mmc.h"
#endif

void usb_init_device(void)
{
#if 0
    int r0;
    outl(inl(0x70000084) | 0x200, 0x70000084);

    outl(inl(0x7000002C) | 0x3000000, 0x7000002C);
    outl(inl(0x6000600C) | 0x400000, 0x6000600C);

    outl(inl(0x60006004) | 0x400000, 0x60006004);   /* reset usb start */
    outl(inl(0x60006004) & ~0x400000, 0x60006004);  /* reset usb end */

    outl(inl(0x70000020) | 0x80000000, 0x70000020);
    while ((inl(0x70000028) & 0x80) == 0);

    outl(inl(0xc5000184) | 0x100, 0xc5000184);
    while ((inl(0xc5000184) & 0x100) != 0);

    outl(inl(0xc50001A4) | 0x5F000000, 0xc50001A4);
    if ((inl(0xc50001A4) & 0x100) == 0) {
        outl(inl(0xc50001A8) & ~0x3, 0xc50001A8);
        outl(inl(0xc50001A8) | 0x2, 0xc50001A8);
        outl(inl(0x70000028) | 0x4000, 0x70000028);
        outl(inl(0x70000028) | 0x2, 0x70000028);
    } else {
        outl(inl(0xc50001A8) | 0x3, 0xc50001A8);
        outl(inl(0x70000028) &~0x4000, 0x70000028);
        outl(inl(0x70000028) | 0x2, 0x70000028);
    }
    outl(inl(0xc5000140) | 0x2, 0xc5000140);
    while((inl(0xc5000140) & 0x2) != 0);
    r0 = inl(0xc5000184);

    /* Note from IPL source (referring to next 5 lines of code: 
       THIS NEEDS TO BE CHANGED ONCE THERE IS KERNEL USB */
    outl(inl(0x70000020) | 0x80000000, 0x70000020);
    outl(inl(0x6000600C) | 0x400000, 0x6000600C);
    while ((inl(0x70000028) & 0x80) == 0);
    outl(inl(0x70000028) | 0x2, 0x70000028);

    udelay(0x186A0);
#endif
}

bool usb_detect(void)
{
    return false;
}

void usb_enable(bool on)
{
    (void)on;
#if 0
    /* For the ipod, we can only do one thing with USB mode - reboot
       into Apple's flash-based disk-mode.  This does not return. */
    if (on)
    {
      /* The following code is copied from ipodlinux */
        unsigned char* storage_ptr = (unsigned char *)0x40017F00;
        memcpy(storage_ptr, "diskmode\0\0hotstuff\0\0\1", 21);
        DEV_RS |= 4; /* Reboot */
    }
#endif
}
