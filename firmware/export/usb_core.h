/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:  $
 *
 * Copyright (C) 2007 by Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef USB_CORE_H
#define USB_CORE_H

#ifndef BOOTLOADER

#define USB_SERIAL
#define USB_STORAGE
#define USB_CHARGING_ONLY
#else /* BOOTLOADER */
#define USB_CHARGING_ONLY
#endif /* BOOTLOADER */

#include "usb_ch9.h"
#include "usb.h"

/* endpoints */
#define EP_CONTROL 0
#define NUM_ENDPOINTS 3

extern int usb_max_pkt_size;

void usb_core_init(void);
void usb_core_exit(void);
void usb_core_control_request(struct usb_ctrlrequest* req);
void usb_core_transfer_complete(int endpoint, bool in, int status, int length);
void usb_core_bus_reset(void);
bool usb_core_exclusive_connection(void);
void usb_core_enable_driver(int driver,bool enabled);
bool usb_core_driver_enabled (int driver);
void usb_core_handle_transfer_completion(
                             struct usb_transfer_completion_event_data* event);
#endif

