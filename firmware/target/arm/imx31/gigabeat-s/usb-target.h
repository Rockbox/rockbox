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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef USB_TARGET_H
#define USB_TARGET_H

#ifdef BOOTLOADER
#define USB_DRIVER_CLOSE
#endif

void usb_connect_event(void);
void usb_init_device(void);
/* Read the immediate state of the cable from the PMIC */
bool usb_plugged(void);

/** Sector read/write filters **/

/* Filter some things in the MBR - see usb-gigabeat-s.c */
void usb_fix_mbr(unsigned char *mbr);
#define USBSTOR_READ_SECTORS_FILTER() \
    ({ if (cur_cmd.sector == 0) \
            usb_fix_mbr(cur_cmd.data[cur_cmd.data_select]); \
    0; })

/* Disallow MBR writes entirely since it was "fixed" in usb_fix_mbr */
#define USBSTOR_WRITE_SECTORS_FILTER() \
    ({ cur_cmd.sector != 0 ? 0 : -1; })

#endif /* USB_TARGET */
