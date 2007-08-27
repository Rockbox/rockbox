/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Christian Gmeiner
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "usbstack/core.h"

/* register serial driver in usb stack */
void usb_serial_driver_init(void);

void usb_serial_driver_bind(void* controller_ops);
int usb_serial_driver_request(struct usb_ctrlrequest* req);
void usb_serial_driver_speed(enum usb_device_speed speed);

#endif /*_SERIAL_H_*/
