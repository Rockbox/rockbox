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
#ifndef USB_STORAGE_H
#define USB_STORAGE_H

#include "usb_ch9.h"

int usb_storage_get_config_descriptor(unsigned char *dest,int max_packet_size,
                                      int interface_number,int endpoint);
void usb_storage_init_connection(int interface,int endpoint);
void usb_storage_init(void);
void usb_storage_transfer_complete(bool in,int state,int length);
bool usb_storage_control_request(struct usb_ctrlrequest* req);
#ifdef HAVE_HOTSWAP
void usb_storage_notify_hotswap(int volume,bool inserted);
#endif

void usb_storage_reconnect(void);

#endif

