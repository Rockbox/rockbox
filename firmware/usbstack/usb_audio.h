/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:  $
 *
 * Copyright (C) 2014 by Amaury Pouly
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef USB_AUDIO_H
#define USB_AUDIO_H

#include "usb_ch9.h"

int usb_audio_request_endpoints(struct usb_class_driver *);
int usb_audio_set_first_interface(int interface);
int usb_audio_get_config_descriptor(unsigned char *dest,int max_packet_size);
void usb_audio_init_connection(void);
void usb_audio_init(void);
void usb_audio_disconnect(void);
void usb_audio_transfer_complete(int ep,int dir, int status, int length);
bool usb_audio_fast_transfer_complete(int ep,int dir, int status, int length);
bool usb_audio_control_request(struct usb_ctrlrequest* req, unsigned char *dest);
int usb_audio_set_interface(int intf, int alt);
int usb_audio_get_interface(int intf);

#endif
