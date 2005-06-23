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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _USB_H_
#define _USB_H_

#include "kernel.h"

void usb_init(void);
void usb_start_monitoring(void);
void usb_acknowledge(long id);
void usb_wait_for_disconnect(struct event_queue *q);
int usb_wait_for_disconnect_w_tmo(struct event_queue *q, int ticks);
bool usb_inserted(void); /* return the official value, what's been reported to the threads */
bool usb_detect(void); /* return the raw hardware value */
#ifdef HAVE_USB_POWER
bool usb_powered(void);
#endif

#endif
